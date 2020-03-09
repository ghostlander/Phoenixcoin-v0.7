// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>
#include <utility>
#include <string>
#include <vector>

#include "init.h"
#include "db.h"
#include "main.h"
#include "rpcmain.h"

using namespace json_spirit;
using namespace std;

Value getgenerate(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getgenerate\n"
          "Returns execution state of the internal proof-of-work miner";
        throw(runtime_error(msg));
    }

    return GetBoolArg("-gen");
}


Value setgenerate(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "setgenerate <state> [genproclimit]\n"
          "Sets execution state of the internal proof-of-work miner.\n"
          "<state> is true or false to set mining on or off respectively.\n"
          "[genproclimit] defines the maximum number of mining threads, -1 is unlimited.";
        throw(runtime_error(msg));
    }

    bool fGenerate = true;
    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    if (params.size() > 1)
    {
        int nGenProcLimit = params[1].get_int();
        mapArgs["-genproclimit"] = itostr(nGenProcLimit);
        if (nGenProcLimit == 0)
            fGenerate = false;
    }
    mapArgs["-gen"] = (fGenerate ? "1" : "0");

    GenerateCoins(fGenerate, pwalletMain);
    return Value::null;
}


Value gethashespersec(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "gethashespersec\n"
          "Returns hash rate of the internal proof-of-work miner";
        throw(runtime_error(msg));
    }

    if (GetTimeMillis() - nHPSTimerStart > 8000)
        return (boost::int64_t)0;
    return (boost::int64_t)dHashesPerSec;
}


Value getmininginfo(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
       string msg = "getmininginfo\n"
         "Returns information related to mining";
        throw(runtime_error(msg));
    }

    Object obj;
    obj.push_back(Pair("blocks",        (int)nBestHeight));
    obj.push_back(Pair("currentblocksize",(uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",(uint64_t)nLastBlockTx));
    obj.push_back(Pair("difficulty",    (double)GetDifficulty()));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    obj.push_back(Pair("generate",      GetBoolArg("-gen")));
    obj.push_back(Pair("genproclimit",  (int)GetArg("-genproclimit", -1)));
    obj.push_back(Pair("hashespersec",  gethashespersec(params, false)));
    obj.push_back(Pair("networkhashps", getnetworkhashps(params, false)));
    obj.push_back(Pair("pooledtx",      (uint64_t)mempool.size()));
    obj.push_back(Pair("testnet",       fTestNet));
    return obj;
}


/* RPC getwork provides a miner with the current best block header to solve
 * and receives the result if available */
Value getwork(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "getwork [data]\n"
          "If [data] is not specified, returns formatted data to work on:\n"
          "  \"data\" : block header\n"
          "  \"target\" : hash target\n"
          "  \"algorithm\" : hashing algorithm expected (optional)\n"
          "If [data] is specified, verifies the proof-of-work hash\n"
          "against target and returns true if successful.";
        throw(runtime_error(msg));
    }

    if(vNodes.empty())
      throw(JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Phoenixcoin is not connected!"));

    if(IsInitialBlockDownload())
      throw(JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Phoenixcoin is downloading blocks..."));

    typedef map<uint256, pair<CBlock*, CScript> > mapNewBlock_t;
    static mapNewBlock_t mapNewBlock;    // FIXME: thread safety
    static vector<CBlock*> vNewBlock;
    static CReserveKey reservekey(pwalletMain);

    if (params.size() == 0)
    {
        // Update block
        static unsigned int nTransactionsUpdatedLast;
        static CBlockIndex* pindexPrev;
        static int64 nStart;
        static CBlock* pblock;
        if (pindexPrev != pindexBest ||
            (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60))
        {
            if (pindexPrev != pindexBest)
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                BOOST_FOREACH(CBlock* pblock, vNewBlock)
                    delete pblock;
                vNewBlock.clear();
            }

            // Clear pindexPrev so future getworks make a new block, despite any failures from here on
            pindexPrev = NULL;

            // Store the pindexBest used before CreateNewBlock, to avoid races
            nTransactionsUpdatedLast = nTransactionsUpdated;
            CBlockIndex* pindexPrevNew = pindexBest;
            nStart = GetTime();

            // Create new block
            pblock = CreateNewBlock(reservekey);
            if (!pblock)
                throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
            vNewBlock.push_back(pblock);

            // Need to update only after we know CreateNewBlock succeeded
            pindexPrev = pindexPrevNew;
        }

        // Update nTime
        pblock->UpdateTime(pindexPrev);
        pblock->nNonce = 0;

        // Update nExtraNonce
        static unsigned int nExtraNonce = 0;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

        /* Save this block for the future use */
        mapNewBlock[pblock->hashMerkleRoot] = make_pair(pblock, pblock->vtx[0].vin[0].scriptSig);

        /* Prepare the block header for transmission */
        uint pdata[32];
        FormatDataBuffer(pblock, pdata);

        /* Get the current decompressed block target */
        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

        Object result;
        result.push_back(Pair("data",
          HexStr(BEGIN(pdata), fNeoScrypt ? (char *) &pdata[20] : END(pdata))));
        result.push_back(Pair("target",
          HexStr(BEGIN(hashTarget), END(hashTarget))));
        /* Optional */
        if(fNeoScrypt)
          result.push_back(Pair("algorithm", "neoscrypt"));
        else
          result.push_back(Pair("algorithm", "scrypt:1024,1,1"));

#if 0
        /* Dump block data sent */
        uint i;
        printf("Block data 80 bytes Tx: ");
        for(i = 0; i < 80; i++)
          printf("%02X", ((uchar *) &pdata[0])[i]);
        printf("\n");
#endif

        return(result);

    } else {

        /* Data received */
        vector<unsigned char> vchData = ParseHex(params[0].get_str());

        /* Must be no less actual data than sent previously */
        if(vchData.size() < 80)
          throw(JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter"));

        CBlock *pdata = (CBlock *) &vchData[0];

#if 0
        /* Dump block data received */
        uint i, size;
        size = (uint)vchData.size();
        printf("Block data %u bytes Rx: ", size);
        for(i = 0; i < size; i++)
          printf("%02X", ((uchar *) &vchData[0])[i]);
        printf("\n");
#endif

        if(!fNeoScrypt) {
            uint i;
            /* nVersion and hashPrevBlock aren't needed */
            for(i = 9; i < 20; i++)
              /* Convert BE to LE */
              ((uint *) pdata)[i] = ByteReverse(((uint *) pdata)[i]);
        }

        /* Pick up the block contents saved previously */
        if(!mapNewBlock.count(pdata->hashMerkleRoot))
          return(false);
        CBlock* pblock = mapNewBlock[pdata->hashMerkleRoot].first;

        /* Replace with the data received */
        pblock->nTime = pdata->nTime;
        pblock->nNonce = pdata->nNonce;
        pblock->vtx[0].vin[0].scriptSig = mapNewBlock[pdata->hashMerkleRoot].second;

        /* Re-build the merkle root */
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        /* Verify the resulting hash against target */
        return(CheckWork(pblock, *pwalletMain, reservekey, true));
    }
}


Value getblocktemplate(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "getblocktemplate [params]\n"
          "Returns data required to construct a block to work on:\n"
          "  \"version\" : block version\n"
          "  \"previousblockhash\" : hash of the current best block\n"
          "  \"transactions\" : contents of transactions to be included in the next block\n"
          "  \"coinbaseaux\" : auxiliary data to be included in the coin base\n"
          "  \"coinbasevalue\" : highest possible value of the coin base including transaction fees\n"
          "  \"target\" : hash target\n"
          "  \"mintime\" : minimum time stamp appropriate for the next block\n"
          "  \"curtime\" : current time stamp\n"
          "  \"mutable\" : list of ways the block template may be changed\n"
          "  \"noncerange\" : range of valid nonces\n"
          "  \"sigoplimit\" : maximum number of sigops per block\n"
          "  \"sizelimit\" : maximum block size\n"
          "  \"bits\" : compressed target of the next block\n"
          "  \"height\" : height of the next block\n"
          "See https://en.bitcoin.it/wiki/BIP_0022 for the complete specification.";
        throw(runtime_error(msg));
    }

    std::string strMode = "template";
    if (params.size() > 0)
    {
        const Object& oparam = params[0].get_obj();
        const Value& modeval = find_value(oparam, "mode");
        if (modeval.type() == str_type)
            strMode = modeval.get_str();
        else if (modeval.type() == null_type)
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if(vNodes.empty())
      throw(JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Phoenixcoin is not connected!"));

    if(IsInitialBlockDownload())
      throw(JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Phoenixcoin is downloading blocks..."));

    static CReserveKey reservekey(pwalletMain);

    // Update block
    static unsigned int nTransactionsUpdatedLast;
    static CBlockIndex* pindexPrev;
    static int64 nStart;
    static CBlock* pblock;
    if (pindexPrev != pindexBest ||
        (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = NULL;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = nTransactionsUpdated;
        CBlockIndex* pindexPrevNew = pindexBest;
        nStart = GetTime();

        // Create new block
        if(pblock)
        {
            delete pblock;
            pblock = NULL;
        }
        pblock = CreateNewBlock(reservekey);
        if (!pblock)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }

    // Update nTime
    pblock->UpdateTime(pindexPrev);
    pblock->nNonce = 0;

    Array transactions;
    map<uint256, int64_t> setTxIndex;
    int i = 0;
    CTxDB txdb("r");
    BOOST_FOREACH (CTransaction& tx, pblock->vtx)
    {
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        Object entry;

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;
        entry.push_back(Pair("data", HexStr(ssTx.begin(), ssTx.end())));

        entry.push_back(Pair("hash", txHash.GetHex()));

        MapPrevTx mapInputs;
        map<uint256, CTxIndex> mapUnused;
        bool fInvalid = false;
        if (tx.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
        {
            entry.push_back(Pair("fee", (int64_t)(tx.GetValueIn(mapInputs) - tx.GetValueOut())));

            Array deps;
            BOOST_FOREACH (MapPrevTx::value_type& inp, mapInputs)
            {
                if (setTxIndex.count(inp.first))
                    deps.push_back(setTxIndex[inp.first]);
            }
            entry.push_back(Pair("depends", deps));

            int64_t nSigOps = tx.GetLegacySigOpCount();
            nSigOps += tx.GetP2SHSigOpCount(mapInputs);
            entry.push_back(Pair("sigops", nSigOps));
        }

        transactions.push_back(entry);
    }

    Object aux;
    aux.push_back(Pair("flags", HexStr(COINBASE_FLAGS.begin(), COINBASE_FLAGS.end())));

    uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

    static Array aMutable;
    if (aMutable.empty())
    {
        aMutable.push_back("time");
        aMutable.push_back("transactions");
        aMutable.push_back("prevblock");
    }

    Object result;
    result.push_back(Pair("version", pblock->nVersion));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("transactions", transactions));
    result.push_back(Pair("coinbaseaux", aux));
    result.push_back(Pair("coinbasevalue", (int64_t)pblock->vtx[0].vout[0].nValue));
    result.push_back(Pair("target", hashTarget.GetHex()));
    result.push_back(Pair("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1));
    result.push_back(Pair("mutable", aMutable));
    result.push_back(Pair("noncerange", "00000000ffffffff"));
    result.push_back(Pair("sigoplimit", (int64_t)MAX_BLOCK_SIGOPS));
    result.push_back(Pair("sizelimit", (int64_t)MAX_BLOCK_SIZE));
    result.push_back(Pair("curtime", (int64_t)pblock->nTime));
    result.push_back(Pair("bits", HexBits(pblock->nBits)));
    result.push_back(Pair("height", (int64_t)(pindexPrev->nHeight+1)));

    return result;
}


Value submitblock(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 2)) {
        string msg = "submitblock <data> [workid]\n"
          "Attempts to submit a new block to the network.\n"
          "[workid] parameter is optional and ignored.\n";
        throw(runtime_error(msg));
    }

    vector<unsigned char> blockData(ParseHex(params[0].get_str()));
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    CBlock block;
    try {
        ssBlock >> block;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    if(!ProcessBlock(NULL, &block)) {
#if 0
        /* Dump block data received */
        uint i, size;
        size = (uint)blockData.size();
        printf("Block data %u bytes Rx: ", size);
        for(i = 0; i < size; i++)
          printf("%02X", ((uchar *) &blockData[0])[i]);
        printf("\n");
#endif
        return("rejected");
    }

    printf("GBT proof-of-work found\n   hash: 0x%s\n target: 0x%s\n",
      block.GetHashPoW().GetHex().c_str(),
      CBigNum().SetCompact(block.nBits).getuint256().GetHex().c_str());
    block.print();
    printf("generated %s\n", FormatMoney(block.vtx[0].vout[0].nValue).c_str());

    return(Value::null);
}
