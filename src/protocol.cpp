// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

#ifndef WINDOWS
#include <arpa/inet.h>
#endif

#include "netbase.h"
#include "util.h"
#include "protocol.h"

static const char *ppszTypeName[] = {
    "ERROR",
    "tx",
    "block",
};

CMessageHeader::CMessageHeader() {
    memcpy(pchMessageStart, ::pchMessageStart, MESSAGE_START_SIZE);
    memset(pchCommand, 0x00, COMMAND_SIZE);
    nMessageSize = std::numeric_limits<uint32_t>::max();
    nChecksum = 0;
}

CMessageHeader::CMessageHeader(const char *pszCommand, uint nMessageSizeIn) {
    memcpy(pchMessageStart, ::pchMessageStart, MESSAGE_START_SIZE);
    strncpy(pchCommand, pszCommand, COMMAND_SIZE - 1);
    pchCommand[COMMAND_SIZE - 1] = 0x00;
    nMessageSize = nMessageSizeIn;
    nChecksum = 0;
}

bool CMessageHeader::IsCommandValid() const {
    uint i = 0;
    uchar c;

    /* Count valid command characters ("a" to "z") until the 1st invalid */
    while(i < COMMAND_SIZE) {
        c = pchCommand[i];
        if((c < 0x61) || (c > 0x7A)) break;
        i++;
    }

    /* Invalid starting character or no null termination */
    if(!i || (i == COMMAND_SIZE)) return(false);

    /* Verify null termination */
    while(i < COMMAND_SIZE) {
        c = pchCommand[i];
        if(c) return(false);
        i++;
    }

    return(true);
}


CAddress::CAddress() : CService()
{
    Init();
}

CAddress::CAddress(CService ipIn, uint64 nServicesIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
}

void CAddress::Init()
{
    nServices = NODE_NETWORK;
    nTime = 100000000;
    nLastTry = 0;
}

CInv::CInv()
{
    type = 0;
    hash = 0;
}

CInv::CInv(int typeIn, const uint256& hashIn)
{
    type = typeIn;
    hash = hashIn;
}

CInv::CInv(const std::string& strType, const uint256& hashIn)
{
    unsigned int i;
    for (i = 1; i < ARRAYLEN(ppszTypeName); i++)
    {
        if (strType == ppszTypeName[i])
        {
            type = i;
            break;
        }
    }
    if (i == ARRAYLEN(ppszTypeName))
        throw std::out_of_range(strprintf("CInv::CInv(string, uint256) : unknown type '%s'", strType.c_str()));
    hash = hashIn;
}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

bool CInv::IsKnownType() const
{
    return (type >= 1 && type < (int)ARRAYLEN(ppszTypeName));
}

const char* CInv::GetCommand() const
{
    if (!IsKnownType())
        throw std::out_of_range(strprintf("CInv::GetCommand() : type=%d unknown type", type));
    return ppszTypeName[type];
}

std::string CInv::ToString() const
{
    return strprintf("%s %s", GetCommand(), hash.ToString().substr(0,20).c_str());
}

void CInv::print() const
{
    printf("CInv(%s)\n", ToString().c_str());
}

