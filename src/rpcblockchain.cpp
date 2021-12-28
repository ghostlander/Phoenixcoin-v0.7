// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include "main.h"
#include "rpcmain.h"

using namespace json_spirit;
using namespace std;

double GetDifficulty(const CBlockIndex *blockindex) {

    /* The reference difficulty is 1.0 which is the lowest Bitcoin difficulty
     * and may not match the minimal difficulty of a particular altcoin */

    if(!blockindex && pindexBest)
      blockindex = pindexBest;

    if(!blockindex) return(1.0);

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}


Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    Array txs;
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
        txs.push_back(tx.GetHash().GetHex());
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));
    return result;
}


Value getblockcount(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getblockcount\n"
          "Displays the number of blocks in the best block chain.";
        throw(runtime_error(msg));
    }

    return(nBestHeight);
}


Value getdifficulty(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getdifficulty\n"
          "Displays the current proof-of-work difficulty\n"
          "as a multiple of the lowest possible difficulty.";
        throw(runtime_error(msg));
    }

    return(GetDifficulty());
}


Value settxfee(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1) || (AmountFromValue(params[0]) < MIN_TX_FEE)) {
        string msg = "settxfee <amount>\n"
          "Sets the transaction fee per 1000 bytes of data.\n"
          "<amount> is a floating point number rounded to the nearest 0.00000001";
        throw(runtime_error(msg));
    }

    nTransactionFee = AmountFromValue(params[0]);
    if(nTransactionFee < MIN_TX_FEE) nTransactionFee = MIN_TX_FEE;

    return(true);
}


Value getrawmempool(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getrawmempool\n"
          "Displays IDs of all transactions stored in the memory pool.";
        throw(runtime_error(msg));
    }

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "getblockhash <height>\n"
          "Displays hash of block in the best block chain at <height>.";
        throw(runtime_error(msg));
    }

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}


Value getblock(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 1)) {
        string msg = "getblock <hash>\n"
          "Displays details of a block with a <hash> given.";
        throw(runtime_error(msg));
    }

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if(!mapBlockIndex.count(hash))
      throw(JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found"));

    CBlock block;
    CBlockIndex *pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return(blockToJSON(block, pblockindex));
}
