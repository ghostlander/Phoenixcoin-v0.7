// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin Developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

//
// Why base-58 instead of standard base-64 encoding?
// - Don't want 0OIl characters that look the same in some fonts and
//      could be used to create visually identical looking account numbers.
// - A string with non-alphanumeric characters is not as easily accepted as an account number.
// - E-mail usually won't line-break if there's no punctuation to break at.
// - Double-clicking selects the whole number as one word if it's all alphanumeric.
//

#ifndef BASE58_H
#define BASE58_H

#include <string>
#include <vector>

#include "bignum.h"
#include "key.h"
#include "script.h"

/* P2PK and P2PKH addresses begin with 'P' */
const uchar PUBKEY_ADDRESS_PREFIX = 0x38;

/* P2SH addresses begin with 'S' */
const uchar SCRIPT_ADDRESS_PREFIX = 0x3F;

/* Testnet P2PK and P2PKH addresses begin with 'm' or 'n' */
const uchar PUBKEY_ADDRESS_TEST_PREFIX = 0x6F;

/* Testnet P2SH addresses begin with 'M' or 'N' */
const uchar SCRIPT_ADDRESS_TEST_PREFIX = 0x34;

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Encode a byte sequence as a base58-encoded string
inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend) {
    CAutoBN_CTX pctx;
    CBigNum bn58 = 58;
    CBigNum bn0 = 0;
    // Convert big endian data to little endian
    // Extra zero at the end make sure bignum will interpret as a positive number
    std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
    reverse_copy(pbegin, pend, vchTmp.begin());
    // Convert little endian data to bignum
    CBigNum bn;
    bn.setvch(vchTmp);
    // Convert bignum to std::string
    std::string str;
    // Expected size increase from base58 conversion is approximately 137%
    // use 138% to be safe
    str.reserve((pend - pbegin) * 138 / 100 + 1);
    CBigNum dv;
    CBigNum rem;
    while(bn > bn0) {
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
        if(!BN_div(&dv, &rem, &bn, &bn58, pctx))
#else
        if(!BN_div(dv.get(), rem.get(), bn.cget(), bn58.cget(), pctx))
#endif
          throw(bignum_error("EncodeBase58 : BN_div() failed"));
        bn = dv;
        uint c = rem.getuint();
        str += pszBase58[c];
    }
    // Leading zeroes encoded as base58 zeros
    for(const unsigned char* p = pbegin; p < pend && *p == 0; p++)
        str += pszBase58[0];
    // Convert little endian std::string to big endian
    reverse(str.begin(), str.end());
    return str;
}

// Encode a byte vector as a base58-encoded string
inline std::string EncodeBase58(const std::vector<unsigned char>& vch) {
    return EncodeBase58(&vch[0], &vch[0] + vch.size());
}

// Decode a base58-encoded string psz into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet) {
    CAutoBN_CTX pctx;
    vchRet.clear();
    CBigNum bn58 = 58;
    CBigNum bn = 0;
    CBigNum bnChar;
    while(isspace(*psz))
        psz++;
    // Convert big endian string to bignum
    for(const char* p = psz; *p; p++) {
        const char* p1 = strchr(pszBase58, *p);
        if(p1 == NULL) {
            while(isspace(*p))
                p++;
            if(*p != '\0')
                return(false);
            break;
        }
        bnChar.setuint(p1 - pszBase58);
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
        if(!BN_mul(&bn, &bn, &bn58, pctx))
#else
        if(!BN_mul(bn.get(), bn.cget(), bn58.cget(), pctx))
#endif
          throw(bignum_error("DecodeBase58 : BN_mul() failed"));
        bn += bnChar;
    }
    // Get bignum as little endian data
    std::vector<unsigned char> vchTmp = bn.getvch();
    // Trim off sign byte if present
    if(vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
        vchTmp.erase(vchTmp.end()-1);
    // Restore leading zeros
    int nLeadingZeros = 0;
    for(const char* p = psz; *p == pszBase58[0]; p++)
        nLeadingZeros++;
    vchRet.assign(nLeadingZeros + vchTmp.size(), 0);
    // Convert little endian data to big endian
    reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
    return(true);
}

// Decode a base58-encoded string str into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet) {
    return DecodeBase58(str.c_str(), vchRet);
}

// Encode a byte vector to a base58-encoded string, including checksum
inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn) {
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(vchIn);
    uint256 hash = Hash(vch.begin(), vch.end());
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return EncodeBase58(vch);
}

// Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet) {
    if(!DecodeBase58(psz, vchRet))
        return(false);
    if(vchRet.size() < 4) {
        vchRet.clear();
        return(false);
    }
    uint256 hash = Hash(vchRet.begin(), vchRet.end()-4);
    if(memcmp(&hash, &vchRet.end()[-4], 4) != 0) {
        vchRet.clear();
        return(false);
    }
    vchRet.resize(vchRet.size()-4);
    return(true);
}

// Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
// returns true if decoding is successful
inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet) {
    return DecodeBase58Check(str.c_str(), vchRet);
}

/** Base class for all base58-encoded data */
class CBase58Data {
protected:
    // the version byte
    unsigned char nVersion;

    // the actually encoded data
    std::vector<unsigned char> vchData;

    CBase58Data() {
        nVersion = 0;
        vchData.clear();
    }

    ~CBase58Data() {
        /* Zero the memory, as it may contain sensitive data */
        if(!vchData.empty()) OPENSSL_cleanse((void *) &vchData[0], vchData.size());
    }

    void SetData(int nVersionIn, const void* pdata, size_t nSize) {
        nVersion = nVersionIn;
        vchData.resize(nSize);
        if(!vchData.empty())
            memcpy(&vchData[0], pdata, nSize);
    }

    void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend) {
        SetData(nVersionIn, (void*)pbegin, pend - pbegin);
    }

public:
    bool SetString(const char* psz) {
        std::vector<unsigned char> vchTemp;
        DecodeBase58Check(psz, vchTemp);
        if(vchTemp.empty()) {
            vchData.clear();
            nVersion = 0;
            return(false);
        }
        nVersion = vchTemp[0];
        vchData.resize(vchTemp.size() - 1);
        if(!vchData.empty())
          memcpy(&vchData[0], &vchTemp[1], vchData.size());
        OPENSSL_cleanse(&vchTemp[0], vchData.size());
        return(true);
    }

    bool SetString(const std::string& str) {
        return SetString(str.c_str());
    }

    std::string ToString() const {
        std::vector<uchar> vch;
        vch.resize(1, nVersion);
        vch.swap(vch);
        vch.insert(vch.end(), vchData.begin(), vchData.end());
        return EncodeBase58Check(vch);
    }

    int CompareTo(const CBase58Data& b58) const {
        if(nVersion < b58.nVersion) return -1;
        if(nVersion > b58.nVersion) return  1;
        if(vchData < b58.vchData)   return -1;
        if(vchData > b58.vchData)   return  1;
        return 0;
    }

    bool operator==(const CBase58Data& b58) const {
        return CompareTo(b58) == 0;
    }
    bool operator<=(const CBase58Data& b58) const {
        return CompareTo(b58) <= 0;
    }
    bool operator>=(const CBase58Data& b58) const {
        return CompareTo(b58) >= 0;
    }
    bool operator< (const CBase58Data& b58) const {
        return CompareTo(b58) <  0;
    }
    bool operator> (const CBase58Data& b58) const {
        return CompareTo(b58) >  0;
    }
};

/** base58-encoded addresses
 * Public-key-hash-addresses have version PUBKEY_ADDRESS (or PUBKEY_ADDRESS_TEST for testnet).
 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
 * Script-hash-addresses have version SCRIPT_ADDRESS (or SCRIPT_ADDRESS_TEST for testnet).
 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
 */
class CCoinAddress;
class CCoinAddressVisitor : public boost::static_visitor<bool> {
private:
    CCoinAddress *addr;
public:
    CCoinAddressVisitor(CCoinAddress *addrIn) : addr(addrIn) { }
    bool operator()(const CKeyID &id) const;
    bool operator()(const CScriptID &id) const;
    bool operator()(const CNoDestination &no) const;
};

class CCoinAddress : public CBase58Data {
public:
    enum {
        PUBKEY_ADDRESS = PUBKEY_ADDRESS_PREFIX,
        SCRIPT_ADDRESS = SCRIPT_ADDRESS_PREFIX,
        PUBKEY_ADDRESS_TEST = PUBKEY_ADDRESS_TEST_PREFIX,
        SCRIPT_ADDRESS_TEST = SCRIPT_ADDRESS_TEST_PREFIX,
    };

    bool Set(const CKeyID &id) {
        SetData(fTestNet ? PUBKEY_ADDRESS_TEST : PUBKEY_ADDRESS, &id, 20);
        return(true);
    }

    bool Set(const CScriptID &id) {
        SetData(fTestNet ? SCRIPT_ADDRESS_TEST : SCRIPT_ADDRESS, &id, 20);
        return(true);
    }

    bool Set(const CTxDestination &dest) {
        return(boost::apply_visitor(CCoinAddressVisitor(this), dest));
    }

    bool IsValid() const {
        bool fExpectTestNet;
        uint nExpectedSize;

        switch(nVersion) {
            case(PUBKEY_ADDRESS):
                /* Hash of public key */
                fExpectTestNet = false;
                nExpectedSize = 20;
                break;
            case(SCRIPT_ADDRESS):
                /* Hash of CScript */
                fExpectTestNet = false;
                nExpectedSize = 20;
                break;
            case(PUBKEY_ADDRESS_TEST):
                fExpectTestNet = true;
                nExpectedSize = 20;
                break;
            case(SCRIPT_ADDRESS_TEST):
                fExpectTestNet = true;
                nExpectedSize = 20;
                break;
            default:
                return(false);
        }

        if((fExpectTestNet != fTestNet) || (vchData.size() != nExpectedSize))
          return(false);

        return(true);
    }

    CCoinAddress() {
    }

    CCoinAddress(const CTxDestination &dest) {
        Set(dest);
    }

    CCoinAddress(const std::string& strAddress) {
        SetString(strAddress);
    }

    CCoinAddress(const char *pszAddress) {
        SetString(pszAddress);
    }

    CTxDestination Get() const {
        if(!IsValid()) return(CNoDestination());
        switch(nVersion) {
            case(PUBKEY_ADDRESS):
            case(PUBKEY_ADDRESS_TEST): {
                uint160 id;
                memcpy(&id, &vchData[0], 20);
                return(CKeyID(id));
            }
            case(SCRIPT_ADDRESS):
            case(SCRIPT_ADDRESS_TEST): {
                uint160 id;
                memcpy(&id, &vchData[0], 20);
                return(CScriptID(id));
            }
        }
        return(CNoDestination());
    }

    bool GetKeyID(CKeyID &keyID) const {
        if(!IsValid()) return(false);
        switch(nVersion) {
            case(PUBKEY_ADDRESS):
            case(PUBKEY_ADDRESS_TEST): {
                uint160 id;
                memcpy(&id, &vchData[0], 20);
                keyID = CKeyID(id);
                return(true);
            }
            default:
                return(false);
        }
    }

    bool IsScript() const {
        if(!IsValid()) return(false);
        switch(nVersion) {
            case(SCRIPT_ADDRESS):
            case(SCRIPT_ADDRESS_TEST):
                return(true);
            default:
                return(false);
        }
    }
};

bool inline CCoinAddressVisitor::operator()(const CKeyID &id) const {
    return(addr->Set(id));
}
bool inline CCoinAddressVisitor::operator()(const CScriptID &id) const {
    return(addr->Set(id));
}
bool inline CCoinAddressVisitor::operator()(const CNoDestination &id) const {
    return(false);
}

/** A base58-encoded secret key */
class CCoinSecret : public CBase58Data {
public:
    enum {
        PRIVKEY_ADDRESS = CCoinAddress::PUBKEY_ADDRESS + 128,
        PRIVKEY_ADDRESS_TEST = CCoinAddress::PUBKEY_ADDRESS_TEST + 128,
    };

    void SetSecret(const CSecret &vchSecret, bool fCompressed) {
        assert(vchSecret.size() == 32);
        SetData(fTestNet ? PRIVKEY_ADDRESS_TEST : PRIVKEY_ADDRESS, &vchSecret[0], vchSecret.size());
        if(fCompressed) vchData.push_back(1);
    }

    CSecret GetSecret(bool &fCompressedOut) {
        CSecret vchSecret;
        vchSecret.resize(32);
        memcpy(&vchSecret[0], &vchData[0], 32);
        fCompressedOut = (vchData.size() == 33);
        return(vchSecret);
    }

    bool IsValid() const {
        bool fExpectTestNet = false;

        switch(nVersion) {
            case(PRIVKEY_ADDRESS):
                break;
            case(PRIVKEY_ADDRESS_TEST):
                fExpectTestNet = true;
                break;
            default:
                return(false);
        }

        /* Key type must match the current network */
        if(fExpectTestNet != fTestNet) return(false);
        /* The only known key sizes */
        if((vchData.size() < 32) || (vchData.size() > 33)) return(false);
        /* Compressed key format */
        if((vchData.size() == 33) && (vchData[32] != 1)) return(false);

        return(true);
    }

    bool SetString(const char* pszSecret) {
        return CBase58Data::SetString(pszSecret) && IsValid();
    }

    bool SetString(const std::string& strSecret) {
        return SetString(strSecret.c_str());
    }

    CCoinSecret(const CSecret &vchSecret, bool fCompressed) {
        SetSecret(vchSecret, fCompressed);
    }

    CCoinSecret() {
    }
};

#endif /* BASE58_H */
