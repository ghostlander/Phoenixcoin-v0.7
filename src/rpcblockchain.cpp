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

double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = pindexBest;
    }

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
    int confirmations = -1;
    // Only report if confirmations of the block is on the main chain
    if (blockindex->IsInMainChain())
        confirmations = nBestHeight - blockindex->nHeight + 1;
    result.push_back(Pair("confirmations", confirmations));
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
          "Returns the number of blocks in the longest block chain";
        throw(runtime_error(msg));
    }

    return(nBestHeight);
}


Value getdifficulty(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getdifficulty\n"
          "Returns the current proof-of-work difficulty\n"
          "as a multiple of the lowest possible difficulty";
        throw(runtime_error(msg));
    }

    return(GetDifficulty());
}


Value getnetworkhashps(const Array &params, bool fHelp) {

    if(fHelp || (params.size() > 1)) {
        string msg = "getnetworkhashps [blocks]\n"
          "Calculates estimated network hashes per second based on the last 30 blocks.\n"
          "Pass in [blocks] to override the default value;\n"
          "zero specifies the number of blocks since the last retarget.";
        throw(runtime_error(msg));
    }

    int lookup = params.size() > 0 ? params[0].get_int() : 30;

    if(pindexBest == NULL) return 0;

    // If look-up is zero or negative value, then use blocks since the last retarget
    if(lookup <= 0) lookup = pindexBest->nHeight % 20 + 1;

    // If look-up is larger than block chain, then set it to the maximum allowed
    if(lookup > pindexBest->nHeight) lookup = pindexBest->nHeight;

    CBlockIndex* pindexPrev = pindexBest;
    for(int i = 0; i < lookup; i++) pindexPrev = pindexPrev->pprev;

    double timeDiff = pindexBest->GetBlockTime() - pindexPrev->GetBlockTime();
    double timePerBlock = timeDiff / lookup;

    return (boost::int64_t)(((double)GetDifficulty() * pow(2.0, 32)) / timePerBlock);
}


Value settxfee(const Array &params, bool fHelp) {

    if(fHelp || (params.size() < 1) || (params.size() > 1)) {
        string msg = "settxfee <amount>\n"
          "Sets the transaction fee per 1000 bytes of data.\n"
          "<amount> is a real and is rounded to the nearest 0.00000001";
        throw(runtime_error(msg));
    }

    // Amount
    int64 nAmount = 0;
    if (params[0].get_real() != 0.0)
        nAmount = AmountFromValue(params[0]);        // rejects 0.0 amounts

    nTransactionFee = nAmount;
    return true;
}

Value getrawmempool(const Array &params, bool fHelp) {

    if(fHelp || (params.size() != 0)) {
        string msg = "getrawmempool\n"
          "Returns IDs of all transactions stored in the memory pool";
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
          "Returns hash of block in the best block chain at <height>";
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
          "Returns details of a block with a given block hash";
        throw(runtime_error(msg));
    }

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex);
}
