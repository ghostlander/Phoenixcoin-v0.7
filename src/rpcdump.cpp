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

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "importprivkey <phoenixcoinprivkey> [label]\n"
            "Adds a private key (as returned by dumpprivkey) to your wallet.");

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();
    CBitcoinSecret vchSecret;
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

        pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
        pwalletMain->ReacceptWalletTransactions();
    }

    return Value::null;
}

Value importwallet(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1))
      throw(runtime_error(
        "importwallet <file name>\n"
        "Imports keys from a wallet dump file.\n"
        "The file name may be specified with a directory path."));

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

Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey <phoenixcoinaddress>\n"
            "Reveals the private key corresponding to <phoenixcoinaddress>.");

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Phoenixcoin address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CSecret vchSecret;
    bool fCompressed;
    if (!pwalletMain->GetSecret(keyID, vchSecret, fCompressed))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret, fCompressed).ToString();
}

Value dumpwallet(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1))
      throw(runtime_error(
        "dumpwallet <file name>\n"
        "Dumps all wallet keys in a human readable format.\n"
        "The file name may be specified with a directory path."));

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
