// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <algorithm>
#include <map>
#include <utility>
#include <list>
#include <vector>
#include <set>
#include <limits>

#include "init.h"
#include "base58.h"
#include "walletdb.h"
#include "wallet.h"
#include "rpcmain.h"
#include "ntp.h"

#include <boost/assign/list_of.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;


int64 nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;

std::string HelpRequiringPassphrase()
{
    return pwalletMain->IsCrypted()
        ? "\nrequires wallet passphrase to be set with walletpassphrase first"
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

void WalletTxToJSON(const CWalletTx& wtx, Object& entry)
{
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsCoinBase())
        entry.push_back(Pair("generated", true));
    if (confirms)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
        entry.push_back(Pair("blocktime", (boost::int64_t)(mapBlockIndex[wtx.hashBlock]->nTime)));
    }
    entry.push_back(Pair("txid", wtx.GetHash().GetHex()));
    entry.push_back(Pair("time", (boost::int64_t)wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (boost::int64_t)wtx.nTimeReceived));
    BOOST_FOREACH(const PAIRTYPE(string,string)& item, wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

string AccountFromValue(const Value& value)
{
    string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}


Value getinfo(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getinfo\n"
          "Returns an object containing various state info";
        throw(runtime_error(msg));
    }

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    obj.push_back(Pair("version",       (int)CLIENT_VERSION));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    obj.push_back(Pair("blocks",        (int)nBestHeight));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("systemtime",    (boost::int64_t)GetTime()));
    obj.push_back(Pair("adjustedtime",  (boost::int64_t)GetAdjustedTime()));
    obj.push_back(Pair("ntpoffset",     (boost::int64_t)nNtpOffset != INT64_MAX ? (boost::int64_t)nNtpOffset : Value::null));
    obj.push_back(Pair("p2poffset",     (boost::int64_t)nPeersOffset != INT64_MAX ? (boost::int64_t)nPeersOffset : Value::null));
    obj.push_back(Pair("testnet",       fTestNet));
    obj.push_back(Pair("keypoololdest", (boost::int64_t)pwalletMain->GetOldestKeyPoolTime()));
    obj.push_back(Pair("keypoolsize",   pwalletMain->GetKeyPoolSize()));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
    obj.push_back(Pair("mininput",      ValueFromAmount(nMinimumInputValue)));
    if (pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", (boost::int64_t)nWalletUnlockTime / 1000));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}


Value getnewaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "getnewaddress [account]\n"
          "Picks up a new address from the key pool.\n"
          "If [account] is specified (recommended), it is added to the address book\n"
          "so payments received with the address will be credited to [account].";
        throw(runtime_error(msg));
    }

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return(CCoinAddress(keyID).ToString());
}


CCoinAddress GetAccountAddress(string strAccount, bool bForceNew = false) {
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid())
    {
        CScript scriptPubKey;
        scriptPubKey.SetDestination(account.vchPubKey.GetID());
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it)
        {
            const CWalletTx& wtx = (*it).second;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed)
    {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey, false))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBookName(account.vchPubKey.GetID(), strAccount);
        walletdb.WriteAccount(strAccount, account);
    }

    return(CCoinAddress(account.vchPubKey.GetID()));
}

Value getaccountaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "getaccountaddress <account>\n"
          "Returns the current address for receiving payments to this account";
        throw(runtime_error(msg));
    }

    // Parse the account first so we don't generate a key if there's an error
    string strAccount = AccountFromValue(params[0]);

    Value ret;

    ret = GetAccountAddress(strAccount).ToString();

    return ret;
}


Value setaccount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "setaccount <address> <account>\n"
          "Sets the account associated with the given address";
        throw(runtime_error(msg));
    }

    CCoinAddress address(params[0].get_str());
    if(!address.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));


    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Detect when changing the account of an address that is the 'unused current key' of another account:
    if (pwalletMain->mapAddressBook.count(address.Get()))
    {
        string strOldAccount = pwalletMain->mapAddressBook[address.Get()];
        if (address == GetAccountAddress(strOldAccount))
            GetAccountAddress(strOldAccount, true);
    }

    pwalletMain->SetAddressBookName(address.Get(), strAccount);

    return Value::null;
}


Value getaccount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "getaccount <address>\n"
          "Returns the account associated with the given address";
        throw(runtime_error(msg));
    }

    CCoinAddress address(params[0].get_str());
    if(!address.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));

    string strAccount;
    map<CTxDestination, string>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.empty())
        strAccount = (*mi).second;
    return strAccount;
}


Value getaddressesbyaccount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "getaddressesbyaccount <account>\n"
          "Returns the list of addresses for the given account";
        throw(runtime_error(msg));
    }

    string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    Array ret;
    BOOST_FOREACH(const PAIRTYPE(CCoinAddress, string)& item, pwalletMain->mapAddressBook) {
        const CCoinAddress &address = item.first;
        const string &strName = item.second;
        if(strName == strAccount) ret.push_back(address.ToString());
    }
    return(ret);
}


Value sendtoaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 2) || (params.size() > 4)) {
        string msg = "sendtoaddress <address> <amount> [comment] [comment-to]\n"
          "<amount> is a real and is rounded to the nearest 0.00000001"
          + HelpRequiringPassphrase();
        throw(runtime_error(msg));
    }

    CCoinAddress address(params[0].get_str());
    if(!address.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));

    // Amount
    int64 nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["to"]      = params[3].get_str();

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (!strError.empty())
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}


Value listaddressgroupings(const Array &params, bool fHelp) {

    if(fHelp) {
        string msg = "listaddressgroupings\n"
          "Lists groups of addresses which have had their common ownership\n"
          "made public by common use as inputs or as the resulting change\n"
          "in past transactions";
        throw(runtime_error(msg));
    }

    Array jsonGroupings;
    map<CTxDestination, int64> balances = pwalletMain->GetAddressBalances();
    BOOST_FOREACH(set<CTxDestination> grouping, pwalletMain->GetAddressGroupings())
    {
        Array jsonGrouping;
        BOOST_FOREACH(CTxDestination address, grouping)
        {
            Array addressInfo;
            addressInfo.push_back(CCoinAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                LOCK(pwalletMain->cs_wallet);
                if(pwalletMain->mapAddressBook.find(CCoinAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                  addressInfo.push_back(pwalletMain->mapAddressBook.find(CCoinAddress(address).Get())->second);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}


Value signmessage(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 2)) {
        string msg = "signmessage <address> <message>\n"
          "Signs a message with the private key of an address";
        throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CCoinAddress addr(strAddress);
    if(!addr.IsValid())
      throw(JSONRPCError(RPC_TYPE_ERROR, "Invalid address"));

    CKeyID keyID;
    if(!addr.GetKeyID(keyID))
      throw(JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key"));

    CKey key;
    if(!pwalletMain->GetKey(keyID, key))
      throw(JSONRPCError(RPC_WALLET_ERROR, "Private key not available"));

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}


Value verifymessage(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 3)) {
        string msg = "verifymessage <address> <signature> <message>\n"
          "Verifies a signed message";
        throw(runtime_error(msg));
    }

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CCoinAddress addr(strAddress);
    if(!addr.IsValid())
      throw(JSONRPCError(RPC_TYPE_ERROR, "Invalid address"));

    CKeyID keyID;
    if(!addr.GetKeyID(keyID))
      throw(JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key"));

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CKey key;
    if (!key.SetCompactSignature(Hash(ss.begin(), ss.end()), vchSig))
        return false;

    return (key.GetPubKey().GetID() == keyID);
}


Value getreceivedbyaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "getreceivedbyaddress <address> [minconf=1]\n"
          "Returns the total amount received by <address> in transactions\n"
          "with at least [minconf] confirmations";
        throw(runtime_error(msg));
    }

    /* Address */
    CCoinAddress address = CCoinAddress(params[0].get_str());
    CScript scriptPubKey;
    if(!address.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));
    scriptPubKey.SetDestination(address.Get());
    if(!IsMine(*pwalletMain, scriptPubKey)) return((double)0.0);

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64 nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return  ValueFromAmount(nAmount);
}


void GetAccountAddresses(string strAccount, set<CTxDestination>& setAddress)
{
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& item, pwalletMain->mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second;
        if (strName == strAccount)
            setAddress.insert(address);
    }
}

Value getreceivedbyaccount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "getreceivedbyaccount <account> [minconf=1]\n"
          "Returns the total amount received by addresses\n"
          "with <account> in transactions with at least [minconf] confirmations";
        throw(runtime_error(msg));
    }

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    string strAccount = AccountFromValue(params[0]);
    set<CTxDestination> setAddress;
    GetAccountAddresses(strAccount, setAddress);

    // Tally
    int64 nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}


int64 GetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64 nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!wtx.IsFinal())
            continue;

        int64 nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

int64 GetAccountBalance(const string& strAccount, int nMinDepth)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth);
}

Value getbalance(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 2)) {
        string msg = "getbalance [account] [minconf=1]\n"
          "If [account] is not specified, returns the server's total available balance.\n"
          "If [account] is specified, returns the balance in the account.";
        throw(runtime_error(msg));
    }

    if (params.size() == 0)
        return  ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    if (params[0].get_str() == "*") {
        // Calculate total balance a different way from GetBalance()
        // (GetBalance() sums up all unspent TxOuts)
        // getbalance and getbalance '*' should always return the same number.
        int64 nBalance = 0;
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (!wtx.IsFinal())
                continue;

            int64 allFee;
            string strSentAccount;
            list<pair<CTxDestination, int64> > listReceived;
            list<pair<CTxDestination, int64> > listSent;
            wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount);
            if (wtx.GetDepthInMainChain() >= nMinDepth)
            {
                BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64)& r, listReceived)
                    nBalance += r.second;
            }
            BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64)& r, listSent)
                nBalance -= r.second;
            nBalance -= allFee;
        }
        return  ValueFromAmount(nBalance);
    }

    string strAccount = AccountFromValue(params[0]);

    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);

    return ValueFromAmount(nBalance);
}


Value movecmd(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 3) || (params.size() > 5)) {
        string msg = "move <from-account> <to-account> <amount> [minconf=1] [comment]\n"
          "Moves from one account in your wallet to another.";
        throw(runtime_error(msg));
    }

    string strFrom = AccountFromValue(params[0]);
    string strTo = AccountFromValue(params[1]);
    int64 nAmount = AmountFromValue(params[2]);
    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    int64 nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    walletdb.WriteAccountingEntry(debit);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    walletdb.WriteAccountingEntry(credit);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}


Value sendfrom(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 3) || (params.size() > 6)) {
        string msg = "sendfrom <from-account> <to-address> <amount> [minconf=1] [comment] [comment-to]\n"
          "<amount> is a real and is rounded to the nearest 0.00000001"
          + HelpRequiringPassphrase();
        throw(runtime_error(msg));
    }

    string strAccount = AccountFromValue(params[0]);
    CCoinAddress address(params[1].get_str());
    if(!address.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));
    int64 nAmount = AmountFromValue(params[2]);
    int nMinDepth = 1;
    if(params.size() > 3)
      nMinDepth = params[3].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();

    EnsureWalletIsUnlocked();

    // Check funds
    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (!strError.empty())
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}


Value sendmany(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 2) || (params.size() > 4)) {
        string msg = "sendmany <from-account> {address:amount,...} [minconf=1] [comment]\n"
          "Amounts are double precision floating point numbers"
          + HelpRequiringPassphrase();
        throw(runtime_error(msg));
    }

    string strAccount = AccountFromValue(params[0]);
    Object sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

    set<CCoinAddress> setAddress;
    vector<pair<CScript, int64> > vecSend;

    int64 totalAmount = 0;
    BOOST_FOREACH(const Pair &s, sendTo) {
        CCoinAddress address(s.name_);
        if(!address.IsValid()) {
            throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
              string("Invalid Phoenixcoin address: ") + s.name_));
        }

        if(setAddress.count(address)) {
            throw(JSONRPCError(RPC_INVALID_PARAMETER,
              string("Invalid parameter, duplicated address: ") + s.name_));
        }
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64 nAmount = AmountFromValue(s.value_);
        totalAmount += nAmount;

        vecSend.push_back(make_pair(scriptPubKey, nAmount));
    }

    EnsureWalletIsUnlocked();

    // Check funds
    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    int64 nFeeRequired = 0;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);
    if (!fCreated)
    {
        if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}


Value addmultisigaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 2) || (params.size() > 3)) {
        string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]\n"
          "Add a nrequired-to-sign multisignature address to the wallet\"\n"
          "each key is an address or hex encoded public key.\n"
          "If [account] is specified, assign address to [account].";
        throw(runtime_error(msg));
    }

    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();
    string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %" PRIszu" keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();

        // Case 1: Bitcoin address and we have full public key:
        CCoinAddress address(ks);
        if(address.IsValid()) {
            CKeyID keyID;
            if(!address.GetKeyID(keyID))
              throw(runtime_error(strprintf("%s does not refer to a key", ks.c_str())));
            CPubKey vchPubKey;
            if(!pwalletMain->GetPubKey(keyID, vchPubKey))
              throw(runtime_error(strprintf("no full public key for address %s", ks.c_str())));
            if(!vchPubKey.IsValid() || !pubkeys[i].SetPubKey(vchPubKey))
              throw(runtime_error(" Invalid public key: " + ks));
        }

        // Case 2: hex public key
        else if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsValid() || !pubkeys[i].SetPubKey(vchPubKey))
                throw runtime_error(" Invalid public key: "+ks);
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner;
    inner.SetMultisig(nRequired, pubkeys);
    CScriptID innerID = inner.GetID();
    pwalletMain->AddCScript(inner);

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return(CCoinAddress(innerID).ToString());
}


struct tallyitem
{
    int64 nAmount;
    int nConf;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
    }
};

Value ListReceived(const Array& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    // Tally
    map<CCoinAddress, tallyitem> mapTally;
    for(map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
      it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx &wtx = (*it).second;

        if (wtx.IsCoinBase() || !wtx.IsFinal())
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < nMinDepth)
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address) || !IsMine(*pwalletMain, address))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = min(item.nConf, nDepth);
        }
    }

    // Reply
    Array ret;
    map<string, tallyitem> mapAccountTally;
    BOOST_FOREACH(const PAIRTYPE(CCoinAddress, string) &item, pwalletMain->mapAddressBook) {
        const CCoinAddress &address = item.first;
        const string &strAccount = item.second;
        map<CCoinAddress, tallyitem>::iterator it = mapTally.find(address);

        if((it == mapTally.end()) && !fIncludeEmpty)
          continue;

        int64 nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        if (it != mapTally.end())
        {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
        }

        if (fByAccounts)
        {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = min(item.nConf, nConf);
        }
        else
        {
            Object obj;
            obj.push_back(Pair("address",       address.ToString()));
            obj.push_back(Pair("account",       strAccount));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    if (fByAccounts)
    {
        for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
        {
            int64 nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            Object obj;
            obj.push_back(Pair("account",       (*it).first));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}


Value listreceivedbyaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 2)) {
        string msg = "listreceivedbyaddress [minconf=1] [include-empty=false]\n"
          "[minconf] is the minimum number of confirmations before payments are included.\n"
          "[include-empty] whether to include addresses that haven't received any payments.\n"
          "Returns an array of objects containing:\n"
          "  \"address\" : receiving address\n"
          "  \"account\" : the account of the receiving address\n"
          "  \"amount\" : total amount received by the address\n"
          "  \"confirmations\" : number of confirmations of the most recent transaction included";
        throw(runtime_error(msg));
    }

    return(ListReceived(params, false));
}


Value listreceivedbyaccount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 2)) {
        string msg = "listreceivedbyaccount [minconf=1] [include-empty=false]\n"
          "[minconf] is the minimum number of confirmations before payments are included.\n"
          "[include-empty] whether to include accounts that haven't received any payments.\n"
          "Returns an array of objects containing:\n"
          "  \"account\" : the account of the receiving addresses\n"
          "  \"amount\" : total amount received by addresses with this account\n"
          "  \"confirmations\" : number of confirmations of the most recent transaction included";
        throw(runtime_error(msg));
    }

    return(ListReceived(params, true));
}


void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
{
    int64 nFee;
    string strSentAccount;
    list<pair<CTxDestination, int64> > listReceived;
    list<pair<CTxDestination, int64> > listSent;

    wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);

    bool fAllAccounts = (strAccount == string("*"));

    // Sent
    if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& s, listSent)
        {
            Object entry;
            entry.push_back(Pair("account", strSentAccount));
            entry.push_back(Pair("address", CCoinAddress(s.first).ToString()));
            entry.push_back(Pair("category", "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }
    }

    // Received
    if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& r, listReceived)
        {
            string account;
            if (pwalletMain->mapAddressBook.count(r.first))
                account = pwalletMain->mapAddressBook[r.first];
            if (fAllAccounts || (account == strAccount))
            {
                Object entry;
                entry.push_back(Pair("account", account));
                entry.push_back(Pair("address", CCoinAddress(r.first).ToString()));
                if (wtx.IsCoinBase())
                {
                    if (wtx.GetDepthInMainChain() < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                }
                else
                    entry.push_back(Pair("category", "receive"));
                entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
        }
    }
}

void AcentryToJSON(const CAccountingEntry& acentry, const string& strAccount, Array& ret)
{
    bool fAllAccounts = (strAccount == string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount)
    {
        Object entry;
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", (boost::int64_t)acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

Value listtransactions(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 3)) {
        string msg = "listtransactions [account] [count=10] [from=0]\n"
          "Returns up to [count] most recent transactions\n"
          "skipping the first [from] transactions for account [account]";
        throw(runtime_error(msg));
    }

    string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    Array ret;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, strAccount);

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret);
        CAccountingEntry *const pacentry = (*it).second.second;
        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount+nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;
    Array::iterator first = ret.begin();
    std::advance(first, nFrom);
    Array::iterator last = ret.begin();
    std::advance(last, nFrom+nCount);

    if (last != ret.end()) ret.erase(last, ret.end());
    if (first != ret.begin()) ret.erase(ret.begin(), first);

    std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    return ret;
}


Value listaccounts(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "listaccounts [minconf=1]\n"
          "Returns object that has account names as keys, account balances as values";
        throw(runtime_error(msg));
    }

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    map<string, int64> mapAccountBalances;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& entry, pwalletMain->mapAddressBook) {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
            mapAccountBalances[entry.second] = 0;
    }

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        int64 nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64> > listReceived;
        list<pair<CTxDestination, int64> > listSent;
        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);
        mapAccountBalances[strSentAccount] -= nFee;
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& s, listSent)
            mapAccountBalances[strSentAccount] -= s.second;
        if (wtx.GetDepthInMainChain() >= nMinDepth)
        {
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& r, listReceived)
                if (pwalletMain->mapAddressBook.count(r.first))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.first]] += r.second;
                else
                    mapAccountBalances[""] += r.second;
        }
    }

    list<CAccountingEntry> acentries;
    CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
    BOOST_FOREACH(const CAccountingEntry& entry, acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    Object ret;
    BOOST_FOREACH(const PAIRTYPE(string, int64)& accountBalance, mapAccountBalances) {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}


Value listsinceblock(const Array &params, bool fHelp) {

    if(fHelp) {
        string msg = "listsinceblock [hash] [target-confirmations]\n"
          "Gets all transactions in blocks since a block [hash] or all transactions if omitted";
        throw(runtime_error(msg));
    }

    CBlockIndex *pindex = NULL;
    int target_confirms = 1;

    if (params.size() > 0)
    {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        pindex = CBlockLocator(blockId).GetBlockIndex();
    }

    if (params.size() > 1)
    {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    }

    int depth = pindex ? (1 + nBestHeight - pindex->nHeight) : -1;

    Array transactions;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain() < depth)
            ListTransactions(tx, "*", 0, true, transactions);
    }

    uint256 lastblock;

    if (target_confirms == 1)
    {
        lastblock = hashBestChain;
    }
    else
    {
        int target_height = pindexBest->nHeight + 1 - target_confirms;

        CBlockIndex *block;
        for (block = pindexBest;
             block && block->nHeight > target_height;
             block = block->pprev)  { }

        lastblock = block ? block->GetBlockHash() : 0;
    }

    Object ret;
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}


Value gettransaction(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "gettransaction <txid>\n"
          "Displays detailed information about an in-wallet transaction <txid>";
        throw(runtime_error(msg));
    }

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;
    if(!pwalletMain->mapWallet.count(hash))
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction ID"));
    const CWalletTx &wtx = pwalletMain->mapWallet[hash];

    int64 nCredit = wtx.GetCredit();
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    int64 nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

    entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    if (wtx.IsFromMe())
        entry.push_back(Pair("fee", ValueFromAmount(nFee)));

    WalletTxToJSON(wtx, entry);

    Array details;
    ListTransactions(wtx, "*", 0, false, details);
    entry.push_back(Pair("details", details));

    return entry;
}


Value backupwallet(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "backupwallet <destination>\n"
          "Safely copies wallet.dat to <destination>,\n"
          "which can be a directory or a path with file name";
        throw(runtime_error(msg));
    }

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}


Value keypoolrefill(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 0)) {
        string msg = "keypoolrefill\n"
          "Tops up the pool of ready-for-use private and public key pairs"
          + HelpRequiringPassphrase();
        throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    pwalletMain->TopUpKeyPool();

    if(pwalletMain->GetKeyPoolSize() < GetArg("-keypool", 100))
      throw(JSONRPCError(RPC_WALLET_ERROR, "Error: Key pool refresh failed!"));

    return(Value::null);
}


void ThreadTopUpKeyPool(void* parg)
{
    // Make this thread recognisable as the key-topping-up thread
    RenameThread("pxc-key-top");

    pwalletMain->TopUpKeyPool();
}

void ThreadCleanWalletPassphrase(void* parg)
{
    // Make this thread recognisable as the wallet relocking thread
    RenameThread("pxc-lock-wa");

    int64 nMyWakeTime = GetTimeMillis() + *((int64*)parg) * 1000;

    ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

    if (nWalletUnlockTime == 0)
    {
        nWalletUnlockTime = nMyWakeTime;

        do
        {
            if (nWalletUnlockTime==0)
                break;
            int64 nToSleep = nWalletUnlockTime - GetTimeMillis();
            if (nToSleep <= 0)
                break;

            LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);
            Sleep(nToSleep);
            ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

        } while(1);

        if (nWalletUnlockTime)
        {
            nWalletUnlockTime = 0;
            pwalletMain->Lock();
        }
    }
    else
    {
        if (nWalletUnlockTime < nMyWakeTime)
            nWalletUnlockTime = nMyWakeTime;
    }

    LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);

    delete (int64*)parg;
}


Value walletpassphrase(const Array &params, bool fHelp) {

    if(pwalletMain->IsCrypted() && (fHelp || (params.size() != 2))) {
        string msg = "walletpassphrase <passphrase> <timeout>\n"
          "Stores the wallet decryption key <passphrase> in memory for <timeout> seconds";
        throw(runtime_error(msg));
    }

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    if (!pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_ALREADY_UNLOCKED, "Error: Wallet is already unlocked.");

    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    if(strWalletPass.length() > 0) {
        if(!pwalletMain->Unlock(strWalletPass))
          throw(JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The passphrase entered is incorrect."));
    } else {
        string msg = "walletpassphrase <passphrase> <timeout>\n"
          "Stores the wallet decryption key <passphrase> in memory for <timeout> seconds";
        throw(runtime_error(msg));
    }

    NewThread(ThreadTopUpKeyPool, NULL);
    int64* pnSleepTime = new int64(params[1].get_int64());
    NewThread(ThreadCleanWalletPassphrase, pnSleepTime);

    return Value::null;
}


Value walletpassphrasechange(const Array &params, bool fHelp) {

    if(pwalletMain->IsCrypted() && (fHelp || (params.size() != 2))) {
        string msg = "walletpassphrasechange <old-passphrase> <new-passphrase>\n"
          "Changes the wallet decryption key from <old-passphrase> to <new-passphrase>";
        throw(runtime_error(msg));
    }

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if((strOldWalletPass.length() < 1) || (strNewWalletPass.length() < 1)) {
        string msg = "walletpassphrasechange <old-passphrase> <new-passphrase>\n"
          "Changes the wallet decryption key from <old-passphrase> to <new-passphrase>";
        throw(runtime_error(msg));
    }

    if(!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
      throw(JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The passphrase entered is incorrect."));

    return(Value::null);
}


Value walletlock(const Array &params, bool fHelp) {

    if(pwalletMain->IsCrypted() && (fHelp || (params.size() != 0))) {
        string msg = "walletlock\n"
          "Removes the wallet decryption key from memory and locks the wallet.\n"
          "You need to call walletpassphrase again after this\n"
          "in order to call any methods which require the wallet to be unlocked.";
        throw(runtime_error(msg));
    }

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return Value::null;
}


Value encryptwallet(const Array &params, bool fHelp) {

    if(!pwalletMain->IsCrypted() && (fHelp || (params.size() != 1))) {
        string msg = "encryptwallet <passphrase>\n"
          "Encrypts the wallet with the <passphrase> key";
        throw(runtime_error(msg));
    }

    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if(strWalletPass.length() < 1) {
        string msg = "encryptwallet <passphrase>\n"
          "Encrypts the wallet with the <passphrase> key";
        throw(runtime_error(msg));
    }

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; Phoenixcoin server stopping, restart to run with encrypted wallet.  The keypool has been flushed, you need to make a new backup.";
}


class DescribeAddressVisitor : public boost::static_visitor<Object> {
private:
    isminetype mine;

public:
    DescribeAddressVisitor(isminetype mineIn) : mine(mineIn) {}

    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;

        obj.push_back(Pair("isscript", false));
        if(mine == MINE_SPENDABLE) {
            pwalletMain->GetPubKey(keyID, vchPubKey);
            obj.push_back(Pair("pubkey", HexStr(vchPubKey.Raw())));
            obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        }
        return(obj);
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        if(mine == MINE_SPENDABLE) {
            CScript subscript;
            pwalletMain->GetCScript(scriptID, subscript);
            std::vector<CTxDestination> dests;
            txnouttype whichType;
            int nRequired;
            uint i;

            ExtractDestinations(subscript, whichType, dests, nRequired);
            obj.push_back(Pair("script", GetTxnOutputType(whichType)));
            obj.push_back(Pair("hex", HexStr(subscript.begin(), subscript.end())));
            Array a;
            for(i = 0; i < dests.size(); i++)
              a.push_back(CCoinAddress(dests[i]).ToString());
            obj.push_back(Pair("addresses", a));
            if(whichType == TX_MULTISIG)
              obj.push_back(Pair("sigsrequired", nRequired));
        }
        return(obj);
    }
};


Value validateaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "validateaddress <address>\n"
          "Returns information about <address>";
        throw(runtime_error(msg));
    }

    CCoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if(isValid) {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();

        ret.push_back(Pair("address", currentAddress));
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : MINE_NO;
        ret.push_back(Pair("ismine", mine != MINE_NO));
        if(mine != MINE_NO) {
            ret.push_back(Pair("watchonly", mine == MINE_WATCH_ONLY));
            Object detail = boost::apply_visitor(DescribeAddressVisitor(mine), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if(pwalletMain->mapAddressBook.count(dest))
          ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return(ret);
}


Value lockunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "lockunspent unlock? [array-of-Objects]\n"
                "Updates list of temporarily unspendable outputs.");

    if (params.size() == 1)
        RPCTypeCheck(params, list_of(bool_type));
    else
        RPCTypeCheck(params, list_of(bool_type)(array_type));

    bool fUnlock = params[0].get_bool();

    if (params.size() == 1) {
        if (fUnlock)
            pwalletMain->UnlockAllCoins();
        return true;
    }

    Array outputs = params[1].get_array();
    for (Value& output : outputs)
    {
        if (output.type() != obj_type)
            throw JSONRPCError(-8, "Invalid parameter, expected object");
        const Object& o = output.get_obj();

        RPCTypeCheck(o, map_list_of("txid", str_type)("vout", int_type));

        string txid = find_value(o, "txid").get_str();
        if (!IsHex(txid))
            throw JSONRPCError(-8, "Invalid parameter, expected hex txid");

        int nOutput = find_value(o, "vout").get_int();
        if (nOutput < 0)
            throw JSONRPCError(-8, "Invalid parameter, vout must be positive");

        COutPoint outpt(uint256(txid), nOutput);

        if (fUnlock)
        		pwalletMain->UnlockCoin(outpt);
        else
        		pwalletMain->LockCoin(outpt);

    }

    return true;
}


Value listlockunspent(const Array& params, bool fHelp)
{
	if (fHelp || params.size() > 0)
		throw runtime_error(
			"listlockunspent\n"
			"Returns list of temporarily unspendable outputs.");

	vector<COutPoint> vOutpts;
	pwalletMain->ListLockedCoins(vOutpts);

	Array ret;

	for (COutPoint &outpt : vOutpts) {
		Object o;

		o.push_back(Pair("txid", outpt.hash.GetHex()));
		o.push_back(Pair("vout", (int)outpt.n));
		ret.push_back(o);	
	}

	return ret;			
}


Value validatepubkey(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "validatepubkey <key>\n"
          "Returns information about <key>";
        throw(runtime_error(msg));
    }

    std::vector<uchar> vchPubKey = ParseHex(params[0].get_str());
    CPubKey pubKey(vchPubKey);

    bool isValid = pubKey.IsValid();
    bool isCompressed = pubKey.IsCompressed();
    CKeyID keyID = pubKey.GetID();

    CCoinAddress address;
    address.Set(keyID);

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if(isValid) {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();

        ret.push_back(Pair("address", currentAddress));
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : MINE_NO;
        ret.push_back(Pair("ismine", mine != MINE_NO));
        ret.push_back(Pair("iscompressed", isCompressed));
        if(mine != MINE_NO) {
            ret.push_back(Pair("watchonly", mine == MINE_WATCH_ONLY));
            Object detail = boost::apply_visitor(DescribeAddressVisitor(mine), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if(pwalletMain->mapAddressBook.count(dest))
          ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return(ret);
}


Value resendtx(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "resendtx\n"
          "Resends unconfirmed wallet transactions.\n";
        throw(runtime_error(msg));
    }

    ResendWalletTransactions(true);

    return(Value::null);
}
