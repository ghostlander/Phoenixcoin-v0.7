// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2012-2013 PPCoin developers
// Copyright (c) 2013-2025 Phoenixcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <map>

#include "sync.h"
#include "serialize.h"
#include "net.h"
#include "util.h"

/* Shouldn't be higher than the default number of confirmations for regular transactions
 * to avoid double spends and other abuse;
 * small value might lead to checkpointing of a fork with a lower trust score */
#define CHECKPOINT_DEFAULT_DEPTH TX_MATURITY

#ifdef WINDOWS
#undef STRICT
#undef PERMISSIVE
#undef ADVISORY
#endif

class uint256;
class CBlockIndex;
class CNode;
class CSyncCheckpoint;

/* Block chain checkpoints are compiled-in sanity checks
 * which should be updated with every release */
namespace Checkpoints {

    /** Checkpointing mode */
    enum CPMode {
        /* Strict policy; performs conflict verification and resolves conflicts */
        STRICT = 0,
        /* Advisory policy; performs conflict verification, but doesn't try to resolve them */
        ADVISORY = 1,
        /* Permissive policy; doesn't perform any conflict checks */
        PERMISSIVE = 2
    };

    /* Returns true if a block passes hardcoded checkpoint checks */
    bool CheckHardened(int nHeight, const uint256 &hash);

    /* Returns a conservative estimate of the total number of blocks, 0 if unknown */
    int GetTotalBlocksEstimate();

    /* Returns the last CBlockIndex * in mapBlockIndex that is a checkpoint */
    CBlockIndex *GetLastCheckpoint(const std::map<uint256, CBlockIndex *> &mapBlockIndex);

    /* Returns the time stamp of the last checkpoint */
    int GetLastCheckpointTime();

    extern uint256 hashSyncCheckpoint;
    extern CSyncCheckpoint checkpointMessage;
    extern uint256 hashInvalidCheckpoint;
    extern CCriticalSection cs_hashSyncCheckpoint;

    CBlockIndex *GetLastSyncCheckpoint();
    bool WriteSyncCheckpoint(const uint256 &hashCheckpoint);
    bool AcceptPendingSyncCheckpoint();
    uint256 AutoSelectSyncCheckpoint();
    bool CheckSync(const uint256 &hashBlock, const CBlockIndex *pindexPrev);
    bool WantedByPendingSyncCheckpoint(uint256 hashBlock);
    bool ResetSyncCheckpoint();
    void AskForPendingSyncCheckpoint(CNode *pfrom);
    bool SetCheckpointPrivKey(std::string strPrivKey);
    bool SendSyncCheckpoint(uint256 hashCheckpoint);
    bool IsMatureSyncCheckpoint();
    bool IsSyncCheckpointTooOld(uint nSeconds);
}

class CUnsignedSyncCheckpoint {
public:
    int nVersion;
    /* Checkpoint block */ 
    uint256 hashCheckpoint;

    IMPLEMENT_SERIALIZE(
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashCheckpoint);
    )

    void SetNull() {
        nVersion = 1;
        hashCheckpoint = 0;
    }

    std::string ToString() const {
        return(strprintf(
          "CSyncCheckpoint(\n"
           "    nVersion       = %d\n"
           "    hashCheckpoint = %s\n"
           ")\n",
          nVersion, hashCheckpoint.ToString().c_str()));
    }

    void print() const {
        printf("%s", ToString().c_str());
    }
};

class CSyncCheckpoint : public CUnsignedSyncCheckpoint {
public:
    static const std::string strMainPubKey;
    static const std::string strTestPubKey;
    static std::string strMasterPrivKey;

    std::vector<uchar> vchMsg;
    std::vector<uchar> vchSig;

    CSyncCheckpoint() {
        SetNull();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vchMsg);
        READWRITE(vchSig);
    )

    void SetNull() {
        CUnsignedSyncCheckpoint::SetNull();
        vchMsg.clear();
        vchSig.clear();
    }

    bool IsNull() const {
        return(hashCheckpoint == 0);
    }

    uint256 GetHash() const {
        return(SerializeHash(*this));
    }

    bool RelayTo(CNode *pnode) const {
        /* Returns true if hasn't been sent already */
        if (pnode->hashCheckpointKnown != hashCheckpoint) {
            pnode->hashCheckpointKnown = hashCheckpoint;
            pnode->PushMessage("checkpoint", *this);
            return(true);
        }
        return(false);
    }

    bool CheckSignature();
    bool ProcessSyncCheckpoint(CNode *pfrom);
};

#endif /* CHECKPOINTS_H */
