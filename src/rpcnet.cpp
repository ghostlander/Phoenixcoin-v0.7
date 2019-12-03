// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>

#include "net.h"
#include "rpc.h"
#include "ntp.h"

using namespace json_spirit;
using namespace std;

Value getconnectioncount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getconnectioncount\n"
            "Returns the number of connections to other nodes.");

    LOCK(cs_vNodes);
    return (int)vNodes.size();
}

static void CopyNodeStats(std::vector<CNodeStats>& vstats)
{
    vstats.clear();

    LOCK(cs_vNodes);
    vstats.reserve(vNodes.size());
    BOOST_FOREACH(CNode* pnode, vNodes) {
        CNodeStats stats;
        pnode->copyStats(stats);
        vstats.push_back(stats);
    }
}

Value getpeerinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpeerinfo\n"
            "Returns data about each connected network node.");

    vector<CNodeStats> vstats;
    CopyNodeStats(vstats);

    Array ret;

    BOOST_FOREACH(const CNodeStats& stats, vstats) {
        Object obj;

        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%08" PRI64x, stats.nServices)));
        obj.push_back(Pair("lastsend", DateTimeStrFormat(stats.nLastSend)));
        obj.push_back(Pair("lastrecv", DateTimeStrFormat(stats.nLastRecv)));
        obj.push_back(Pair("conntime", DateTimeStrFormat(stats.nTimeConnected)));
        if(stats.nPingTime < (~0U - 1U)) {
            obj.push_back(Pair("pingtime", strprintf("%u ms", stats.nPingTime)));
        } else {
            obj.push_back(Pair("pingtime", "invalid"));
        }
        obj.push_back(Pair("version", stats.nVersion));
        obj.push_back(Pair("subver", stats.strSubVer));
        obj.push_back(Pair("inbound", stats.fInbound));
        obj.push_back(Pair("releasetime", (boost::int64_t)stats.nReleaseTime));
        obj.push_back(Pair("startingheight", stats.nStartingHeight));
        obj.push_back(Pair("txbytes", (boost::int64_t)stats.nTxBytes));
        obj.push_back(Pair("rxbytes", (boost::int64_t)stats.nRxBytes));
        obj.push_back(Pair("banscore", stats.nMisbehavior));

        ret.push_back(obj);
    }

    return ret;
}

Value addnode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() == 2)
        strCommand = params[1].get_str();
    if (fHelp || params.size() != 2 ||
        (strCommand != "onetry" && strCommand != "add" && strCommand != "remove"))
        throw runtime_error(
            "addnode <node> <add|remove|onetry>\n"
            "Attempts to add or remove <node> from the addnode list or try a connection to <node> once.");

    string strNode = params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        ConnectNode(addr, strNode.c_str());
        return Value::null;
    }

    LOCK(cs_vAddedNodes);
    vector<string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++)
        if (strNode == *it)
            break;

    if (strCommand == "add")
    {
        if (it != vAddedNodes.end())
            throw JSONRPCError(-23, "Error: Node already added");
        vAddedNodes.push_back(strNode);
    }
    else if(strCommand == "remove")
    {
        if (it == vAddedNodes.end())
            throw JSONRPCError(-24, "Error: Node has not been added.");
        vAddedNodes.erase(it);
    }

    return Value::null;
}

Value getaddednodeinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getaddednodeinfo <extinfo> [node:port]\n"
            "Returns information about the given added node or all added nodes\n"
            "<extinfo> is true or false to provide extended information or not\n"
            "(note that onetry addnodes are not listed)\n");

    bool fDns = params[0].get_bool();

    list<string> laddedNodes(0);
    if (params.size() == 1)
    {
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            laddedNodes.push_back(strAddNode);
    }
    else
    {
        string strNode = params[1].get_str();
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            if (strAddNode == strNode)
            {
                laddedNodes.push_back(strAddNode);
                break;
            }
        if (laddedNodes.size() == 0)
            throw JSONRPCError(-24, "Error: Node has not been added");
    }

    if (!fDns)
    {
        Object ret;
        BOOST_FOREACH(string& strAddNode, laddedNodes)
            ret.push_back(Pair("addednode", strAddNode));
        return ret;
    }

    Array ret;

    list<pair<string, vector<CService> > > laddedAddreses(0);
    BOOST_FOREACH(string& strAddNode, laddedNodes)
    {
        vector<CService> vservNode(0);
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
            laddedAddreses.push_back(make_pair(strAddNode, vservNode));
        else
        {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            obj.push_back(Pair("connected", false));
            Array addresses;
            obj.push_back(Pair("addresses", addresses));
        }
    }

    LOCK(cs_vNodes);
    for (list<pair<string, vector<CService> > >::iterator it = laddedAddreses.begin(); it != laddedAddreses.end(); it++)
    {
        Object obj;
        obj.push_back(Pair("addednode", it->first));

        Array addresses;
        bool fConnected = false;
        BOOST_FOREACH(CService& addrNode, it->second)
        {
            bool fFound = false;
            Object node;
            node.push_back(Pair("address", addrNode.ToString()));
            BOOST_FOREACH(CNode* pnode, vNodes)
                if (pnode->addr == addrNode)
                {
                    fFound = true;
                    fConnected = true;
                    node.push_back(Pair("connected", pnode->fInbound ? "inbound" : "outbound"));
                    break;
                }
            if (!fFound)
                node.push_back(Pair("connected", "false"));
            addresses.push_back(node);
        }
        obj.push_back(Pair("connected", fConnected));
        obj.push_back(Pair("addresses", addresses));
        ret.push_back(obj);
    }

    return ret;
}

Value getnettotals(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 0))
      throw(runtime_error(
        "getnettotals\n"
        "Returns information about network traffic including bytes received,\n"
        "bytes transmitted and current time."));

    Object obj;
    obj.push_back(Pair("rxbytestotal", (boost::uint64_t)CNode::GetTotalBytesRx()));
    obj.push_back(Pair("txbytestotal", (boost::uint64_t)CNode::GetTotalBytesTx()));
    obj.push_back(Pair("timemillis", (boost::int64_t)GetTimeMillis()));
    return(obj);
}

Value ntptime(const Array &params, bool fHelp) {
    int64 nTime;
    CNetAddr ip;

    if(fHelp || (params.size() > 1))
      throw(runtime_error(
        "ntptime [server]\n"
        "Returns current time from a specific or random NTP server."));

    if(params.size() > 0) {
        nTime = NtpGetTime(params[0].get_str());
    } else {
        nTime = NtpGetTime(ip);
    }

    Object obj;
    switch(nTime) {

        case(-1):
            throw(runtime_error("Socket init error"));

        case(-2):
            throw(runtime_error("Non-blocking socket mode failed"));

        case(-3):
            throw(runtime_error("Unable to send data"));

        case(-4):
            throw(runtime_error("Receive timed out"));

        default:
            if((nTime > 0) && (nTime != 2085978496)) {
                /* Update the offset */
                nNtpOffset = nTime - GetTime();
                /* Remove the warning if back to normal */
                if(fNtpWarning && (abs64(nNtpOffset) <= 5 * 60)) {
                    strMiscWarning.clear();
                    fNtpWarning = false;
                }
                /* Push data into the object */
                if(ip.IsValid()) obj.push_back(Pair("server", ip.ToStringIP().c_str()));
                obj.push_back(Pair("time", DateTimeStrFormat(nTime)));
                obj.push_back(Pair("sample", (boost::int64_t)nTime));
                obj.push_back(Pair("offset", (boost::int64_t)nNtpOffset));
        } else throw(runtime_error("Unknown response"));

    }

    return(obj);
}
