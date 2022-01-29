/*
 * Copyright (c) 2018-2021 John Doering <ghostlander@phoenixcoin.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string>
#include <vector>

#include "base58.h"
#include "init.h"
#include "wallet.h"
#include "rpcmain.h"
#include "util.h"

using namespace json_spirit;
using namespace std;

extern CWallet *pwalletMain;

Value encryptmessage(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 2)) {
      string msg = "encryptmessage <key> <message>\n"
        "Encodes a text <message> with the public <key> provided.\n"
        "If an address is provided instead, the respective key is picked from the wallet.";
      throw(runtime_error(msg));
    }

    CPubKey pubKey;

    if(IsHex(params[0].get_str())) {
        pubKey = ParseHex(params[0].get_str());
    } else {
        CCoinAddress addr(params[0].get_str());
        if(addr.IsValid()) {
            CKeyID keyID;
            addr.GetKeyID(keyID);
            if(!pwalletMain->GetPubKey(keyID, pubKey))
              throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Public key not found for this address"));
        }
    }

    vector<uchar> vchEncrypted;
    string strData = params[1].get_str();
    pubKey.EncryptData(vector<uchar>(strData.begin(), strData.end()), vchEncrypted);

    return(EncodeBase58Check(vchEncrypted));
}


Value decryptmessage(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 2)) {
      string msg = "decryptmessage <key> <message>\n"
        "Decodes a text <message> with the private <key> provided.\n"
        "If an address is provided instead, the respective key is picked from the wallet.";
      throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    CKey key;
    CCoinAddress addr(params[0].get_str());

    if(addr.IsValid()) {
        CKeyID keyID;
        addr.GetKeyID(keyID);
        if(!pwalletMain->GetKey(keyID, key))
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key not found for this address"));
    } else {
        CCoinSecret vchSecret;
        if(!vchSecret.SetString(params[0].get_str()))
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key"));
        bool fCompressed;
        CSecret secret = vchSecret.GetSecret(fCompressed);
        key.SetSecret(secret, fCompressed);
    }

    vector<uchar> vchEncrypted;
    if(!DecodeBase58Check(params[1].get_str(), vchEncrypted))
      throw(JSONRPCError(RPC_INVALID_PARAMETER, "Invalid message string"));

    vector<unsigned char> vchDecrypted;
    key.DecryptData(vchEncrypted, vchDecrypted);

    return(std::string((const char *) &vchDecrypted[0], vchDecrypted.size()));
}


Value encryptdata(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 2)) {
      string msg = "encryptdata <key> <data>\n"
        "Encodes hexadecimal <data> with the public <key> provided.\n"
        "If an address is provided instead, the respective key is picked from the wallet.";
      throw(runtime_error(msg));
    }

    CPubKey pubKey;

    if(IsHex(params[0].get_str())) {
        pubKey = ParseHex(params[0].get_str());
    } else {
        CCoinAddress addr(params[0].get_str());
        if(addr.IsValid()) {
            CKeyID keyID;
            addr.GetKeyID(keyID);
            if(!pwalletMain->GetPubKey(keyID, pubKey))
              throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Public key not found for this address"));
        }
    }

    vector<uchar> vchEncrypted;
    pubKey.EncryptData(ParseHex(params[1].get_str()), vchEncrypted);

    return(HexStr(vchEncrypted));
}


Value decryptdata(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 2)) {
      string msg = "decryptdata <key> <data>\n"
        "Decodes hexadecimal <data> with the private <key> provided.\n"
        "If an address is provided instead, the respective key is picked from the wallet.";
      throw(runtime_error(msg));
    }

    EnsureWalletIsUnlocked();

    CKey key;
    CCoinAddress addr(params[0].get_str());

    if(addr.IsValid()) {
        CKeyID keyID;
        addr.GetKeyID(keyID);
        if(!pwalletMain->GetKey(keyID, key))
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key not found for this address"));
    } else {
        CCoinSecret vchSecret;
        if(!vchSecret.SetString(params[0].get_str()))
          throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key"));
        bool fCompressed;
        CSecret secret = vchSecret.GetSecret(fCompressed);
        key.SetSecret(secret, fCompressed);
    }

    vector<uchar> vchDecrypted;
    key.DecryptData(ParseHex(params[1].get_str()), vchDecrypted);

    return(HexStr(vchDecrypted));
}
