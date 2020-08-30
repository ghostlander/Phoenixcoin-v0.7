// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "init.h" // for pwalletMain
#include "base58.h"
#include "rpcmain.h"
#include "ui_interface.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#define printf OutputDebugStringF

using namespace json_spirit;
using namespace std;

class CTxDump
{
public:
    CBlockIndex *pindex;
    int64 nValue;
    bool fSpent;
    CWalletTx* ptx;
    int nOut;
    CTxDump(CWalletTx* ptx = NULL, int nOut = -1)
    {
        pindex = NULL;
        nValue = 0;
        fSpent = false;
        this->ptx = ptx;
        this->nOut = nOut;
    }
};

Value importprivkey(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 3)) {
        string msg = "importprivkey <key> [label] [rescan]\n"
          "Adds a private <key> to your wallet in the format of RPC dumpprivkey.\n"
          "Block chain re-scanning is on (true) by default.\n";
        throw(runtime_error(msg));
    }

    string strSecret = params[0].get_str();
    string strLabel = "";
    if(params.size() > 1)
      strLabel = params[1].get_str();

    bool fRescan = true;
    if(params.size() > 2)
      fRescan = params[2].get_bool();

    CCoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");

    CKey key;
    bool fCompressed;
    CSecret secret = vchSecret.GetSecret(fCompressed);
    key.SetSecret(secret, fCompressed);
    CKeyID vchAddress = key.GetPubKey().GetID();
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBookName(vchAddress, strLabel);

        if (!pwalletMain->AddKey(key))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        pwalletMain->UpdateTimeFirstKey();

        if(fRescan) {
            pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
            pwalletMain->ReacceptWalletTransactions();
        }
    }

    return(Value::null);
}

Value importaddress(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 3)) {
        string msg =  "importaddress <address> [label] [rescan]\n"
        "Adds a watch only (unspendable) address to your wallet.\n"
        "P2PKH pubkey script in hex may be specified instead of the address.\n"
        "Block chain re-scanning is off (false) by default.\n";
        throw(runtime_error(msg));
    }

    string strLabel = "";
    if(params.size() > 1)
      strLabel = params[1].get_str();

    bool fRescan = false;
    if(params.size() > 2)
      fRescan = params[2].get_bool();

    CScript script;
    CCoinAddress addr;

    if(IsHex(params[0].get_str())) {
        std::vector<uchar> vchScriptPubKey(ParseHex(params[0].get_str()));
        if(vchScriptPubKey.size() != 25)
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid P2PKH pubkey script"));
        script = CScript(vchScriptPubKey.begin(), vchScriptPubKey.end());
        /* Copy the public key hash */
        std::string strTemp = params[0].get_str().substr(6, 40);
        /* Insert the Base58 prefix */
        char prefix[3];
        sprintf(prefix, "%x", fTestNet ? PUBKEY_ADDRESS_TEST_PREFIX : PUBKEY_ADDRESS_PREFIX);
        strTemp.insert(0, prefix);
        /* Convert and encode */
        std::vector<uchar> vchTemp(ParseHex(strTemp));
        addr = CCoinAddress(EncodeBase58Check(vchTemp));
    } else {
        CKeyID keyID;
        addr = CCoinAddress(params[0].get_str());
        if(!addr.GetKeyID(keyID))
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address"));
        script = GetScriptForPubKeyHash(keyID);
    }

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        if(::IsMine(*pwalletMain, script) == MINE_SPENDABLE)
          throw(JSONRPCError(RPC_WALLET_ERROR, "The private key is already in the wallet"));

        if(pwalletMain->HaveWatchOnly(script))
          throw(JSONRPCError(RPC_WALLET_ERROR, "The address is being watched already"));

        pwalletMain->MarkDirty();

        if(addr.IsValid())
          pwalletMain->SetAddressBookName(addr.Get(), strLabel);

        if(!pwalletMain->AddWatchOnly(script))
          throw(JSONRPCError(RPC_WALLET_ERROR, "Failed to add the address to the wallet"));

        if(fRescan) {
            pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
            pwalletMain->ReacceptWalletTransactions();
        }

    }

    return(Value::null);
}

Value importpubkey(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 3)) {
        string msg =  "importpubkey <key> [label] [rescan]\n"
        "Adds a watch only (unspendable) public key in hex to your wallet.\n"
        "Block chain re-scanning is off (false) by default.\n";
        throw(runtime_error(msg));
    }

    string strLabel = "";
    if(params.size() > 1)
      strLabel = params[1].get_str();

    bool fRescan = false;
    if(params.size() > 2)
      fRescan = params[2].get_bool();

    CScript script;
    CCoinAddress addr(params[0].get_str());

    if(!IsHex(params[0].get_str()))
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Hex string expected for public key"));

    CPubKey pubKey(std::vector<uchar> (ParseHex(params[0].get_str())));
    if(!pubKey.IsValid())
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid public key"));

    CKeyID keyID = pubKey.GetID();
    addr = CCoinAddress(keyID);
    script = GetScriptForPubKeyHash(keyID);

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        if(::IsMine(*pwalletMain, script) == MINE_SPENDABLE)
          throw(JSONRPCError(RPC_WALLET_ERROR, "The private key is already in the wallet"));

        if(pwalletMain->HaveWatchOnly(script))
          throw(JSONRPCError(RPC_WALLET_ERROR, "The public key is being watched already"));

        pwalletMain->MarkDirty();

        if(addr.IsValid())
          pwalletMain->SetAddressBookName(addr.Get(), strLabel);

        if(!pwalletMain->AddWatchOnly(script))
          throw(JSONRPCError(RPC_WALLET_ERROR, "Failed to add the public key to the wallet"));

        if(fRescan) {
            pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
            pwalletMain->ReacceptWalletTransactions();
        }

    }

    return(Value::null);
}

Value importwallet(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "importwallet <file>\n"
          "Imports keys from a <file> generated by RPC dumpwallet.\n"
          "The file name may be specified with a directory path.";
        throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    boost::filesystem::ifstream file;
    boost::filesystem::path pathImportFile = params[0].get_str().c_str();
    if(!pathImportFile.is_complete()) pathImportFile = GetDataDir(true) / pathImportFile;
    if(!boost::filesystem::exists(pathImportFile))
      throw(JSONRPCError(RPC_INVALID_PARAMETER, "The file with wallet keys doesn't exist"));
    file.open(pathImportFile, std::ios_base::in);
    if(!file.good())
      throw(JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open the file with wallet keys"));
    file.close();

    if(!ImportWallet(pwalletMain, pathImportFile.string().c_str()))
      throw(JSONRPCError(RPC_WALLET_ERROR, "Failed while importing keys to the wallet"));

    return(Value::null);
}

Value dumpprivkey(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "dumpprivkey <address>\n"
          "Reveals the private key corresponding to an <address>";
        throw(runtime_error(msg));
    }

    string strAddress = params[0].get_str();
    CCoinAddress address;
    if(!address.SetString(strAddress))
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address"));
    CKeyID keyID;
    if(!address.GetKeyID(keyID))
      throw(JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key"));
    CSecret vchSecret;
    bool fCompressed;
    if(!pwalletMain->GetSecret(keyID, vchSecret, fCompressed)) {
        throw(JSONRPCError(RPC_WALLET_ERROR,
          "Private key for address " + strAddress + " is not known"));
    }
    return(CCoinSecret(vchSecret, fCompressed).ToString());
}

Value dumpwallet(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "dumpwallet <file>\n"
          "Dumps all wallet keys to a <file> in a human readable format.\n"
          "The file name may be specified with a directory path.";
        throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    boost::filesystem::ofstream file;
    boost::filesystem::path pathDumpFile = params[0].get_str().c_str();
    if(!pathDumpFile.is_complete()) pathDumpFile = GetDataDir(true) / pathDumpFile;
    if(boost::filesystem::exists(pathDumpFile))
      throw(JSONRPCError(RPC_INVALID_PARAMETER, "The file for wallet keys exists already"));
    file.open(pathDumpFile, std::ios_base::out);
    if(!file.good())
      throw(JSONRPCError(RPC_INVALID_PARAMETER, "Cannot create the file for wallet keys"));
    file.close();

    if(!ExportWallet(pwalletMain, pathDumpFile.string().c_str()))
      throw(JSONRPCError(RPC_WALLET_ERROR, "Failed while exporting keys from the wallet"));

    return(Value::null);
}
