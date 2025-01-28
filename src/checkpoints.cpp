// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2012-2013 PPCoin developers
// Copyright (c) 2013-2025 Phoenixcoin Developers
// Distributed under the MIT/X11 software licence, see the accompanying
// file LICENCE or http://opensource.org/license/mit

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#if 0
#include "txdb.h"
#else
#include "main.h"
#include "db.h"
#endif

#include "uint256.h"
#include "checkpoints.h"

extern bool fTestNet;

namespace Checkpoints {
    typedef std::map<int, std::pair<uint256, uint> > MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (        1, std::make_pair(uint256("0x006e800301bbd850d7ba67ad9295c3a940f5fc8f5581e3484967bd33c4c8b965"), 1368040258) )
        (   100000, std::make_pair(uint256("0xe6f6b91c942fb9f3bc522e6fc6435c1b77aed48806e99bb942acbc3f809e09e7"), 1381275938) )
        (   200000, std::make_pair(uint256("0xe8bbfa06d1ea5fea06491403094b3d86bc11fbc0e3845add62d0c46dc8b95627"), 1388828071) )
        (   300000, std::make_pair(uint256("0x5c72edfd71f7b7ec5e26147c8d8d3152e3de279d49ca0b7102971cf8f30f83aa"), 1398261279) )
        (   400000, std::make_pair(uint256("0x40a98cddd43ba8154ddf87ded0fb9416631b0398a0d9948f0e53b802f14af4b1"), 1407916106) )
        (   500000, std::make_pair(uint256("0x77b50650fa74d03b8739b4a6d34842df70d7167f4b5aa050da6cd9f74cf4154e"), 1416796125) )
        (   600000, std::make_pair(uint256("0xb18f5e9a9cd9a0dd4b055c432c2f85914286a4f48ced445db3d46698848ee158"), 1425685777) )
        (   700000, std::make_pair(uint256("0x0d18f383033c9eef67455f92327d165bb7234691b62f4f6621b36a4e5be066f8"), 1434668550) )
        (   800000, std::make_pair(uint256("0x49bab7503b7af642f1acd29b00672383f9951992a940a39aa5bb50c0297f1566"), 1443572643) )
        (   900000, std::make_pair(uint256("0x07911e1fb999f49575a1b3efa7bad84d8324c9e789707e03a4822efda7d4b1c0"), 1452466519) )
        (  1000000, std::make_pair(uint256("0xf635c3a4aa0ca540ea3bfea3d0ac413f9e1b5dcea7d1e528b18523cfba5a02ad"), 1461358254) )
        (  1100001, std::make_pair(uint256("0x98102b6321f926b1525a6b81d33009b25e1f097a54d04a2460a7d28c9c786554"), 1470270740) )
        (  1200000, std::make_pair(uint256("0x6996853bdcf79dc878736e34bee9c1f574d9d9bc850ef201b6aae8992c1e0a21"), 1479145447) )
        (  1300000, std::make_pair(uint256("0x4abd6d77d47ffd7e8dcfc722428078ef2932028e5526e09cb2e028471fc75793"), 1488037008) )
        (  1400000, std::make_pair(uint256("0xd5abea56a12ddbf81792565789ae11d924cdd929b1e1af4f18b568cbdd74ca07"), 1496836525) )
        (  1500000, std::make_pair(uint256("0x04b78040e2a22225768c5c9ad41343876d9fa70e1cdcdc550f0756df618ef55b"), 1505698751) )
        (  1600000, std::make_pair(uint256("0xfd418845a71e37685ba56cd5f94996141ff6e8b9e6a4bc38d5d063e28e987074"), 1514546170) )
        (  1700000, std::make_pair(uint256("0x2747c57407de0b4504198848a46e86c0f7414203cbe51e37459b9a55c00e3dae"), 1523446688) )
        (  1800000, std::make_pair(uint256("0x0a168f3016d4ebb6e6a2b49a111a72aadd0381e84cac200ef82f114a75ccbc9f"), 1532170865) )
        (  1900000, std::make_pair(uint256("0x13c7815537337a25b1da443156686e1133a79ddc6bd8fb95389750205bf58210"), 1540917035) )
        (  2000000, std::make_pair(uint256("0xb415f4159123aeef929866fa830b67150abf7563c686f8911c99b052d1aeda7d"), 1550039975) )
        (  2100000, std::make_pair(uint256("0xfe0d9e6fd6f9553ba7aea75a62476eefed30bc54ecbeaa7b5403c122e47f34c2"), 1559020950) )
        (  2200000, std::make_pair(uint256("0xaa9233229e4b18e7d2abd0084358fe1e9ad52f01f8276ce5c6ad5309a77df429"), 1567902553) )
        (  2300000, std::make_pair(uint256("0x22cb9e1d5025fbffe0ba4f5bdb031260274fb577937976c171cd5c29cd5976be"), 1577011118) )
        (  2400000, std::make_pair(uint256("0x4ed40a6fc035a735700ac907d47accff18f40da3b7f2626202446f52ce7993bc"), 1585985501) )
        (  2500000, std::make_pair(uint256("0x99855dc131f842dd6f14d4d97a6c81e5b51a464115a7851a5a053f2f9858f8eb"), 1594905740) )
        (  2600000, std::make_pair(uint256("0x4a3b8a4141637104e427b79414570bddd6ef922bbeda48629adf4a11d394b186"), 1604470267) )
        (  2700000, std::make_pair(uint256("0x777634a0ffc4b17454da00bd2de0aabade9af287638e9db99743f04023d2a0d0"), 1613536574) )
        (  2800000, std::make_pair(uint256("0x6f0a5f4358455be430bf7e947cc156051e0e32af12833bdbe8e847b5a750b2af"), 1622468912) )
        (  2900000, std::make_pair(uint256("0x34d363216b5abcde063e6d30b4597cef135783ce54b53b473edb64f73a15463e"), 1631636992) )
        (  3000000, std::make_pair(uint256("0x3f507f61825551a914b9208e2e62351fd9beb5c86f71b13ca1f7de78d552a4b7"), 1640684012) )
        (  3100000, std::make_pair(uint256("0x660c4bdf66541ef91d614d993a89bc5031e72388203768bb00f609b95947232c"), 1649730605) )
        (  3200000, std::make_pair(uint256("0x7addb0cb9b72826ff0bda32acac0ad235de4a2a62451292d06e568237cbf4bfc"), 1658725195) )
        (  3300000, std::make_pair(uint256("0x7213f51165c3ec304757589ca22b21da48e4e788220732d3ec6b6cac0fdddb74"), 1667648119) )
        (  3400000, std::make_pair(uint256("0x4c1e72329c99da051e958e31f70e6c333288a745555c5caa6c00ec1330604e80"), 1676629459) )
        (  3500000, std::make_pair(uint256("0x6b12fb3416df39d198a3e6e0186459e818916b3cbb43b215744ea106ba2f42f8"), 1685633927) )
        (  3600000, std::make_pair(uint256("0x4fbfc131291f779fee8dcb91283e7d01dd2569adabeb48c5869dcdf8968a1e4e"), 1694649476) )
        (  3700000, std::make_pair(uint256("0xe874151b9c73b5ec299e7b0e04d50904252b43801bfac9c2355a9cfc9b039eb0"), 1703676051) )
        (  3800000, std::make_pair(uint256("0xa134c63c612b6c020c6e32d91d9b2d7e40f94fe497b07708dd74cf5928c2ddf3"), 1712744560) )
        (  3900000, std::make_pair(uint256("0x5edb8a4cf71966d277e0de1efb60db6580705a976fc2421ed85c30afb03ec593"), 1721636650) )
        (  4000000, std::make_pair(uint256("0xb89c98ec5fae38cb1fc0e255380f7fa86d2a6711d82bd63febf4ac55ace633f9"), 1730816840) )
        ;

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (       1, std::make_pair(uint256("0xebc580b1c5100b9763d5c5d8a4df724b1d0f32ed78203fd9321fe6f2fbff5373"), 1383844822) )
        ;

    bool CheckHardened(int nHeight, const uint256 &hash) {
        MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if(i == checkpoints.end()) return(true);
        return(hash == i->second.first);
    }

    int GetTotalBlocksEstimate() {
        MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        return(checkpoints.rbegin()->first);
    }

    int GetLastCheckpointTime() {
        MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        return(checkpoints.rbegin()->second.second);
    }

    CBlockIndex *GetLastCheckpoint(const std::map<uint256, CBlockIndex *> &mapBlockIndex) {
        MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type &i, checkpoints) {
            const uint256 &hash = i.second.first;
            std::map<uint256, CBlockIndex *>::const_iterator t = mapBlockIndex.find(hash);
            if(t != mapBlockIndex.end()) return(t->second);
        }
        return(NULL);
    }

    /* Advanced (broadcast centrally and synchronised automatically) checkpoints */

    uint256 hashSyncCheckpoint = 0;
    uint256 hashPendingCheckpoint = 0;
    CSyncCheckpoint checkpointMessage;
    CSyncCheckpoint checkpointMessagePending;
    uint256 hashInvalidCheckpoint = 0;
    CCriticalSection cs_hashSyncCheckpoint;

    CBlockIndex *GetLastSyncCheckpoint() {
        LOCK(cs_hashSyncCheckpoint);
        if(!mapBlockIndex.count(hashSyncCheckpoint)) {
          error("GetLastSyncCheckpoint(): block index missing for the current advanced checkpoint %s",
            hashSyncCheckpoint.ToString().c_str());
        } else {
            return(mapBlockIndex[hashSyncCheckpoint]);
        }
        return(NULL);
    }

    bool ValidateSyncCheckpoint(uint256 hashCheckpoint) {

        if(!mapBlockIndex.count(hashSyncCheckpoint)) {
            return(error("ValidateSyncCheckpoint(): block index record missing for the current advanced checkpoint %s",
              hashSyncCheckpoint.ToString().c_str()));
        }

        if(!mapBlockIndex.count(hashCheckpoint)) {
            return(error("ValidateSyncCheckpoint(): block index record missing for the received advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }

        CBlockIndex *pindexSyncCheckpoint = mapBlockIndex[hashSyncCheckpoint];
        CBlockIndex *pindexCheckpointRecv = mapBlockIndex[hashCheckpoint];

        if(pindexCheckpointRecv->nHeight <= pindexSyncCheckpoint->nHeight) {
            /* Received an older checkpoint, trace back from the current checkpoint
             * to the same height of the received checkpoint to verify
             * that the current checkpoint should be a descendant block */
            CBlockIndex *pindex = pindexSyncCheckpoint;
            while(pindex->nHeight > pindexCheckpointRecv->nHeight) {
                if(!(pindex = pindex->pprev))
                  return(error("ValidateSyncCheckpoint(): pprev null, block index failure"));
            }
            if(pindex->GetBlockHash() != hashCheckpoint) {
                hashInvalidCheckpoint = hashCheckpoint;
                return(error("ValidateSyncCheckpoint(): new advanced checkpoint %s is conflicting with the current checkpoint %s",
                  hashCheckpoint.ToString().c_str(), hashSyncCheckpoint.ToString().c_str()));
            }

            /* Ignore an older checkpoint */
            return(false);
        }

        /* Received checkpoint should be a descendant block of the current checkpoint.
         * Trace back to the same height of current checkpoint to verify. */
        CBlockIndex *pindex = pindexCheckpointRecv;
        while(pindex->nHeight > pindexSyncCheckpoint->nHeight) {
            if(!(pindex = pindex->pprev))
              return(error("ValidateSyncCheckpoint(): pprev2 null, block index failure"));
        }
        if(pindex->GetBlockHash() != hashSyncCheckpoint) {
            hashInvalidCheckpoint = hashCheckpoint;
            return(error("ValidateSyncCheckpoint(): new advanced checkpoint %s is not a descendant of the current checkpoint %s",
              hashCheckpoint.ToString().c_str(), hashSyncCheckpoint.ToString().c_str()));
        }

        return(true);
    }

    bool WriteSyncCheckpoint(const uint256 &hashCheckpoint) {
#if 0
        {
            LOCK(Checkpoints::cs_hashSyncCheckpoint);
            if(!pblocktree->WriteSyncCheckpoint(hashCheckpoint)) {
                return(error("WriteSyncCheckpoint(): data base write failure for an advanced checkpoint %s",
                  hashCheckpoint.ToString().c_str()));
            }
        }
#else
        CTxDB txdb;
        txdb.TxnBegin();
        if(!txdb.WriteSyncCheckpoint(hashCheckpoint)) {
            txdb.TxnAbort();
            return(error("WriteSyncCheckpoint(): data base write failure for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
        if(!txdb.TxnCommit()) {
            return(error("WriteSyncCheckpoint(): data base commit failure for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
        txdb.Close();
#endif

        Checkpoints::hashSyncCheckpoint = hashCheckpoint;
        return(true);
    }

    bool AcceptPendingSyncCheckpoint() {
        LOCK(cs_hashSyncCheckpoint);
        if((hashPendingCheckpoint != 0) && mapBlockIndex.count(hashPendingCheckpoint)) {
            if(!ValidateSyncCheckpoint(hashPendingCheckpoint)) {
                hashPendingCheckpoint = 0;
                checkpointMessagePending.SetNull();
                return(false);
            }

#if 0
            CBlockIndex *pindexCheckpoint = mapBlockIndex[hashPendingCheckpoint];
            if(!pindexCheckpoint->IsInMainChain()) {
                CBlock block;
                if(!block.ReadFromDisk(pindexCheckpoint)) {
                    return(error("AcceptPendingSyncCheckpoint(): ReadFromDisk() failed for an advanced checkpoint %s",
                      hashPendingCheckpoint.ToString().c_str()));
                }
                if(!SetBestChain(pindexCheckpoint)) {
                    hashInvalidCheckpoint = hashPendingCheckpoint;
                    return(error("AcceptPendingSyncCheckpoint(): SetBestChain() failed for an advanced checkpoint %s",
                      hashPendingCheckpoint.ToString().c_str()));
                }
            }
#else
            CTxDB txdb;
            CBlockIndex *pindexCheckpoint = mapBlockIndex[hashPendingCheckpoint];
            if(!pindexCheckpoint->IsInMainChain()) {
                CBlock block;
                if(!block.ReadFromDisk(pindexCheckpoint)) {
                    return(error("AcceptPendingSyncCheckpoint(): ReadFromDisk() failed for an advanced checkpoint %s",
                      hashPendingCheckpoint.ToString().c_str()));
                }
                if(!block.SetBestChain(txdb, pindexCheckpoint)) {
                    hashInvalidCheckpoint = hashPendingCheckpoint;
                    return(error("AcceptPendingSyncCheckpoint(): SetBestChain() failed for an advanced checkpoint %s",
                      hashPendingCheckpoint.ToString().c_str()));
                }
            }
            txdb.Close();
#endif

            if(!WriteSyncCheckpoint(hashPendingCheckpoint)) {
                return(error("AcceptPendingSyncCheckpoint(): write failure for an advanced checkpoint %s",
                  hashPendingCheckpoint.ToString().c_str()));
            }

            hashPendingCheckpoint = 0;
            checkpointMessage = checkpointMessagePending;
            checkpointMessagePending.SetNull();
            printf("AcceptPendingSyncCheckpoint(): advanced checkpoint at %s\n", hashSyncCheckpoint.ToString().c_str());

            /* Relay the checkpoint */
            if(!checkpointMessage.IsNull()) {
                BOOST_FOREACH(CNode *pnode, vNodes) checkpointMessage.RelayTo(pnode);
            }

            return(true);
        }

        return(false);
    }

    /* Checkpoint master: selects a block for checkpointing according to the policy */
    uint256 AutoSelectSyncCheckpoint() {
        /* No immediate checkpointing on either PoW or PoS blocks,
         * select by depth in the main chain rather than block time */
        const CBlockIndex *pindex = pindexBest;
        while(pindex->pprev &&
          (pindex->nHeight + (int)GetArg("-checkpointdepth", CHECKPOINT_DEFAULT_DEPTH)) > pindexBest->nHeight) {
            pindex = pindex->pprev;
        }
        return(pindex->GetBlockHash());
    }

    /* Check against a sync checkpoint */
    bool CheckSync(const uint256 &hashBlock, const CBlockIndex *pindexPrev) {
        int nHeight = pindexPrev->nHeight + 1;

        LOCK(cs_hashSyncCheckpoint);
        /* Sync checkpoint should always be an accepted block */
        assert(mapBlockIndex.count(hashSyncCheckpoint));
        const CBlockIndex *pindexSync = mapBlockIndex[hashSyncCheckpoint];

        if(nHeight > pindexSync->nHeight) {
            /* Trace back to same height as the sync checkpoint */
            const CBlockIndex *pindex = pindexPrev;
            while(pindex->nHeight > pindexSync->nHeight) {
                if(!(pindex = pindex->pprev))
                  return(error("CheckSync(): pprev null, block index failure"));
            }
            /* Only a sync checkpoint descendant can pass */
            if((pindex->nHeight < pindexSync->nHeight) || (pindex->GetBlockHash() != hashSyncCheckpoint)) {
                return(false);
            }
        }
        /* Reject the same height as the current sync checkpoint */
        if((nHeight == pindexSync->nHeight) && (hashBlock != hashSyncCheckpoint)) {
            return(false);
        }
        /* Reject lower height as well */
        if(nHeight < pindexSync->nHeight && !mapBlockIndex.count(hashBlock)) {
            return(false);
        }

        return(true);
    }

    bool WantedByPendingSyncCheckpoint(uint256 hashBlock) {
        LOCK(cs_hashSyncCheckpoint);
        if(hashPendingCheckpoint == 0) return(false);
        if(hashBlock == hashPendingCheckpoint) return(true);
        if(mapOrphanBlocks.count(hashPendingCheckpoint) &&
          hashBlock == WantedByOrphan(mapOrphanBlocks[hashPendingCheckpoint])) {
            return(true);
        }
        return(false);
    }

    bool ResetSyncCheckpoint() {
        LOCK(cs_hashSyncCheckpoint);

        /* Reset sync checkpoint to the last hardened checkpoint
         * by walking from the youngest hardened checkpoint to the oldest until
         * there is a match with the block index */
        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type &i, mapCheckpoints) {
            const uint256 &hash = i.second.first;
            const int &time = i.second.second;
            const int &height = i.first;
            if(mapBlockIndex.count(hash) && mapBlockIndex[hash]->IsInMainChain()) {
                if(!WriteSyncCheckpoint(hash)) {
                    return(error("ResetSyncCheckpoint(): write failure for an advanced checkpoint %s",
                      hash.ToString().c_str()));
                }
                printf("ResetSyncCheckpoint(): advanced checkpoint reset to a hardened checkpoint %s height %d %s\n",
                  hash.ToString().c_str(), height, DateTimeStrFormat(time).c_str());
                return(true);
            }
        }

        /* If no match, reset to the genesis block */
        if(!WriteSyncCheckpoint(hashGenesisBlock)) {
            return(error("ResetSyncCheckpoint(): write failure for the genesis block %s",
              hashGenesisBlock.ToString().c_str()));
        } else {
            printf("ResetSyncCheckpoint(): advanced checkpoint reset to the genesis block %s\n",
              hashGenesisBlock.ToString().c_str());
            return(true);
        }

        return(false);
    }

    void AskForPendingSyncCheckpoint(CNode *pfrom) {
        LOCK(cs_hashSyncCheckpoint);
        if(pfrom && (hashPendingCheckpoint != 0) &&
          (!mapBlockIndex.count(hashPendingCheckpoint)) &&
          (!mapOrphanBlocks.count(hashPendingCheckpoint))) {
            pfrom->AskFor(CInv(MSG_BLOCK, hashPendingCheckpoint));
        }
    }

    bool SetCheckpointPrivKey(std::string strPrivKey) {
        CSyncCheckpoint checkpoint;
        checkpoint.hashCheckpoint = hashGenesisBlock;

        /* Test signing a sync checkpoint with the genesis block */
        CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
        sMsg << (CUnsignedSyncCheckpoint)checkpoint;
        checkpoint.vchMsg = std::vector<uchar>(sMsg.begin(), sMsg.end());

        std::vector<uchar> vchPrivKey = ParseHex(strPrivKey);
        CKey key;
        key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end()));

        if(!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig)) {
            return(false);
        }

        CSyncCheckpoint::strMasterPrivKey = strPrivKey;

        return(true);
    }

    bool SendSyncCheckpoint(uint256 hashCheckpoint) {
        CSyncCheckpoint checkpoint;
        checkpoint.hashCheckpoint = hashCheckpoint;
        CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
        sMsg << (CUnsignedSyncCheckpoint)checkpoint;
        checkpoint.vchMsg = std::vector<uchar>(sMsg.begin(), sMsg.end());

        if(CSyncCheckpoint::strMasterPrivKey.empty()) {
            return(error("SendSyncCheckpoint(): master key unavailable"));
        }
        std::vector<uchar> vchPrivKey = ParseHex(CSyncCheckpoint::strMasterPrivKey);
        CKey key;
        key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end()));
        if(!key.Sign(Hash(checkpoint.vchMsg.begin(), checkpoint.vchMsg.end()), checkpoint.vchSig)) {
            return(error("SendSyncCheckpoint(): unable to sign an advanced checkpoint, check the private key"));
        }

        if(!checkpoint.ProcessSyncCheckpoint(NULL)) {
            printf("WARNING: SendSyncCheckpoint(): failed to process\n");
            return(false);
        }

        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode *pnode, vNodes) checkpoint.RelayTo(pnode);
        }

        return(true);
    }

    bool IsMatureSyncCheckpoint() {
        LOCK(cs_hashSyncCheckpoint);
        /* Sync checkpoint should always be an accepted block */
        assert(mapBlockIndex.count(hashSyncCheckpoint));
        const CBlockIndex *pindexSync = mapBlockIndex[hashSyncCheckpoint];
        return(nBestHeight >= (pindexSync->nHeight + nBaseMaturity));
    }

    bool IsSyncCheckpointTooOld(uint nSeconds) {
        LOCK(cs_hashSyncCheckpoint);
        /* Sync checkpoint should always be an accepted block */
        assert(mapBlockIndex.count(hashSyncCheckpoint));
        const CBlockIndex *pindexSync = mapBlockIndex[hashSyncCheckpoint];
        return(pindexSync->GetBlockTime() + nSeconds < GetAdjustedTime());
    }
}

const std::string CSyncCheckpoint::strMainPubKey =
  "047b13b3ae2132afa4e216671cd1cad76866643049cad069c296235891c1db907bffa319a9ad9a28c54bcd9ab5f5d0f03dac6abac44f227c436c9f66eee2bd38c2";
const std::string CSyncCheckpoint::strTestPubKey =
  "0460d85a95e4a867be3bbd860b5c699e1017b7090f81d84b4de698bdb2d320ad7d856fe0172b7b486ddf734f21e6f9012f1f1eaa749882009f8675d1a24dc73fe0";
std::string CSyncCheckpoint::strMasterPrivKey = "";

/* Signature verification of a sync checkpoint message */
bool CSyncCheckpoint::CheckSignature() {
    CKey key;

    std::string strMasterPubKey = fTestNet ? CSyncCheckpoint::strTestPubKey : CSyncCheckpoint::strMainPubKey;

    if(!key.SetPubKey(ParseHex(strMasterPubKey))) {
        return(error("CSyncCheckpoint::CheckSignature(): SetPubKey() failed"));
    }
    if(!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig)) {
        return(error("CSyncCheckpoint::CheckSignature(): signature verification failed"));
    }

    /* Unserialise the data */
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *(CUnsignedSyncCheckpoint*)this;
    return(true);
}

/* Sync checkpoint main processing routine */
bool CSyncCheckpoint::ProcessSyncCheckpoint(CNode *pfrom) {

    if(!CheckSignature()) return(false);

    LOCK(Checkpoints::cs_hashSyncCheckpoint);
    if(!mapBlockIndex.count(hashCheckpoint)) {

        /* Haven't received the checkpoint chain, keep the checkpoint as pending */
        Checkpoints::hashPendingCheckpoint = hashCheckpoint;
        Checkpoints::checkpointMessagePending = *this;
        printf("ProcessSyncCheckpoint(): pending for an advanced checkpoint %s\n",
          hashCheckpoint.ToString().c_str());

        /* Request any missing data */
        if(pfrom) {
            pfrom->PushGetBlocks(pindexBest, hashCheckpoint);
            /* Ask directly as well just in case */
            pfrom->AskFor(CInv(MSG_BLOCK, mapOrphanBlocks.count(hashCheckpoint) ?
              WantedByOrphan(mapOrphanBlocks[hashCheckpoint]) : hashCheckpoint));
        }

        return(false);
    }

    /* Old valid checkpoints are also reported as invalid */
    if(!Checkpoints::ValidateSyncCheckpoint(hashCheckpoint)) return(false);

#if 0
    CBlockIndex *pindexCheckpoint = mapBlockIndex[hashCheckpoint];
    if(!pindexCheckpoint->IsInMainChain()) {
        /* Checkpoint chain received but not yet the main chain */
        CBlock block;
        if(!block.ReadFromDisk(pindexCheckpoint)) {
            return(error("ProcessSyncCheckpoint(): ReadFromDisk() failed for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
        if(!SetBestChain(pindexCheckpoint)) {
            Checkpoints::hashInvalidCheckpoint = hashCheckpoint;
            return(error("ProcessSyncCheckpoint(): SetBestChain() failed for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
    }
#else
    CTxDB txdb;
    CBlockIndex *pindexCheckpoint = mapBlockIndex[hashCheckpoint];
    if(!pindexCheckpoint->IsInMainChain()) {
        /* Checkpoint chain received but not yet the main chain */
        CBlock block;
        if(!block.ReadFromDisk(pindexCheckpoint)) {
            return(error("ProcessSyncCheckpoint(): ReadFromDisk() failed for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
        if(!block.SetBestChain(txdb, pindexCheckpoint)) {
            Checkpoints::hashInvalidCheckpoint = hashCheckpoint;
            return(error("ProcessSyncCheckpoint(): SetBestChain() failed for an advanced checkpoint %s",
              hashCheckpoint.ToString().c_str()));
        }
    }
    txdb.Close();
#endif

    if(!Checkpoints::WriteSyncCheckpoint(hashCheckpoint)) {
        return(error("ProcessSyncCheckpoint(): write failure for an advanced checkpoint %s",
          hashCheckpoint.ToString().c_str()));
    }

    Checkpoints::checkpointMessage = *this;
    Checkpoints::hashPendingCheckpoint = 0;
    Checkpoints::checkpointMessagePending.SetNull();
    printf("ProcessSyncCheckpoint(): advanced checkpoint at %s\n", hashCheckpoint.ToString().c_str());

    return(true);
}

