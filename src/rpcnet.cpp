// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
        obj.push_back(Pair("banscore", stats.nMisbehavior));

        ret.push_back(obj);
    }

    return ret;
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
