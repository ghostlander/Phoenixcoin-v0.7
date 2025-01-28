// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

#include <vector>

#include "ntp.h"
#include "net.h"
#include "util.h"
#include "rpcmain.h"

using namespace json_spirit;
using namespace std;


Value getconnectioncount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getconnectioncount\n"
          "Displays the number of connections to other nodes.";
        throw(runtime_error(msg));
    }

    LOCK(cs_vNodes);
    return((int)vNodes.size());
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


Value getpeerinfo(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getpeerinfo\n"
          "Displays data about each connected network node.";
        throw(runtime_error(msg));
    }

    vector<CNodeStats> vstats;
    CopyNodeStats(vstats);

    Array ret;

    BOOST_FOREACH(const CNodeStats& stats, vstats) {
        Object obj;

        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%016" PRI64x "", stats.nServices)));
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


Value addnode(const Array &params, bool fHelp) {
    string strCommand;

    if(params.size() == 2)
      strCommand = params[1].get_str();

    if(fHelp || (params.size() != 2) ||
      ((strCommand != "onetry") && (strCommand != "add") && (strCommand != "remove"))) {
        string msg = "addnode <node:port> <add|remove|onetry>\n"
          "Attempts to add or remove a node from the list or try a connection to the node once.";
        throw(runtime_error(msg));
    }

    string strNode = params[0].get_str();

    if(strCommand == "onetry") {
        CAddress addr;
        ConnectNode(addr, strNode.c_str());
        return(Value::null);
    }

    LOCK(cs_vAddedNodes);
    vector<string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++)
      if(strNode == *it) break;

    if(strCommand == "add") {
        if(it != vAddedNodes.end())
          throw(JSONRPCError(-23, "Error: Node has been added already"));
        vAddedNodes.push_back(strNode);
    }
    else if(strCommand == "remove") {
        if(it == vAddedNodes.end())
          throw(JSONRPCError(-24, "Error: Node has not been added"));
        vAddedNodes.erase(it);
    }

    return(Value::null);
}


Value getaddednodeinfo(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "getaddednodeinfo <extinfo> [node:port]\n"
          "Displays information on the particular added node or all added nodes.\n"
          "<extinfo> is true or false to provide extended information or not to.\n"
          "One try addnodes are not listed.\n";
        throw(runtime_error(msg));
    }

    bool fDns = params[0].get_bool();

    list<string> laddedNodes(0);
    if(params.size() == 1) {
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string &strAddNode, vAddedNodes)
          laddedNodes.push_back(strAddNode);
    } else {
        string strNode = params[1].get_str();
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string &strAddNode, vAddedNodes)
          if(strAddNode == strNode) {
              laddedNodes.push_back(strAddNode);
              break;
          }
        if(!laddedNodes.size())
          throw(JSONRPCError(-24, "Error: Node has not been added"));
    }

    if(!fDns) {
        Object ret;
        BOOST_FOREACH(string &strAddNode, laddedNodes)
          ret.push_back(Pair("addednode", strAddNode));
        return(ret);
    }

    Array ret;

    list<pair<string, vector<CService> > > laddedAddreses(0);
    BOOST_FOREACH(string &strAddNode, laddedNodes) {
        vector<CService> vservNode(0);
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
          laddedAddreses.push_back(make_pair(strAddNode, vservNode));
        else {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            obj.push_back(Pair("connected", false));
            Array addresses;
            obj.push_back(Pair("addresses", addresses));
        }
    }

    LOCK(cs_vNodes);
    for(list<pair<string, vector<CService> > >::iterator it = laddedAddreses.begin();
      it != laddedAddreses.end(); it++) {
        Object obj;
        obj.push_back(Pair("addednode", it->first));

        Array addresses;
        bool fConnected = false;
        BOOST_FOREACH(CService &addrNode, it->second) {
            bool fFound = false;
            Object node;
            node.push_back(Pair("address", addrNode.ToString()));
            BOOST_FOREACH(CNode *pnode, vNodes)
              if(pnode->addr == addrNode) {
                  fFound = true;
                  fConnected = true;
                  node.push_back(Pair("connected", pnode->fInbound ? "inbound" : "outbound"));
                  break;
              }
            if(!fFound) node.push_back(Pair("connected", "false"));
            addresses.push_back(node);
        }
        obj.push_back(Pair("connected", fConnected));
        obj.push_back(Pair("addresses", addresses));
        ret.push_back(obj);
    }

    return(ret);
}


Value getnettotals(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 0)) {
        string msg = "getnettotals\n"
          "Displays network traffic related information\n"
          "including bytes received, bytes transmitted and current time.";
        throw(runtime_error(msg));
    }

    Object obj;
    obj.push_back(Pair("rxbytestotal", (boost::uint64_t)CNode::GetTotalBytesRx()));
    obj.push_back(Pair("txbytestotal", (boost::uint64_t)CNode::GetTotalBytesTx()));
    obj.push_back(Pair("timemillis", (boost::int64_t)GetTimeMillis()));
    return(obj);
}


Value ntptime(const Array &params, bool fHelp) {
    int64 nTime;
    CNetAddr ip;

    if(fHelp || (params.size() > 1)) {
        string msg = "ntptime [server]\n"
          "Displays current time received from a specific or random NTP server.";
        throw(runtime_error(msg));
    }

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
