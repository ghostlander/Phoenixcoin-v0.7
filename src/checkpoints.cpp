// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>  // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints {
typedef std::map<int, uint256> MapCheckpoints;

//
// What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
//
static MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
    (1, uint256("0x006e800301bbd850d7ba67ad9295c3a940f5fc8f5581e3484967bd33c4c8b965"))
    (100000, uint256("0xe6f6b91c942fb9f3bc522e6fc6435c1b77aed48806e99bb942acbc3f809e09e7"))
    (200000, uint256("0xe8bbfa06d1ea5fea06491403094b3d86bc11fbc0e3845add62d0c46dc8b95627"))
    (300000, uint256("0x5c72edfd71f7b7ec5e26147c8d8d3152e3de279d49ca0b7102971cf8f30f83aa"))
    (400000, uint256("0x40a98cddd43ba8154ddf87ded0fb9416631b0398a0d9948f0e53b802f14af4b1"))
    (500000, uint256("0x77b50650fa74d03b8739b4a6d34842df70d7167f4b5aa050da6cd9f74cf4154e"))
    (600000, uint256("0xb18f5e9a9cd9a0dd4b055c432c2f85914286a4f48ced445db3d46698848ee158"))
    (700000, uint256("0x0d18f383033c9eef67455f92327d165bb7234691b62f4f6621b36a4e5be066f8"))
    (800000, uint256("0x49bab7503b7af642f1acd29b00672383f9951992a940a39aa5bb50c0297f1566"))
    (900000, uint256("0x07911e1fb999f49575a1b3efa7bad84d8324c9e789707e03a4822efda7d4b1c0"))
    (1000000, uint256("0xf635c3a4aa0ca540ea3bfea3d0ac413f9e1b5dcea7d1e528b18523cfba5a02ad"))
    (1100001, uint256("0x98102b6321f926b1525a6b81d33009b25e1f097a54d04a2460a7d28c9c786554"))
    (1200000, uint256("0x6996853bdcf79dc878736e34bee9c1f574d9d9bc850ef201b6aae8992c1e0a21"))
    (1300000, uint256("0x4abd6d77d47ffd7e8dcfc722428078ef2932028e5526e09cb2e028471fc75793"))
    (1400000, uint256("0xd5abea56a12ddbf81792565789ae11d924cdd929b1e1af4f18b568cbdd74ca07"))
    (1500000, uint256("0x04b78040e2a22225768c5c9ad41343876d9fa70e1cdcdc550f0756df618ef55b"))
    (1600000, uint256("0xfd418845a71e37685ba56cd5f94996141ff6e8b9e6a4bc38d5d063e28e987074"))
    (1700000, uint256("0x2747c57407de0b4504198848a46e86c0f7414203cbe51e37459b9a55c00e3dae"))
    (1800000, uint256("0x0a168f3016d4ebb6e6a2b49a111a72aadd0381e84cac200ef82f114a75ccbc9f"))
    (1900000, uint256("0x13c7815537337a25b1da443156686e1133a79ddc6bd8fb95389750205bf58210"))
    ;

static MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of
    (1, uint256("0xebc580b1c5100b9763d5c5d8a4df724b1d0f32ed78203fd9321fe6f2fbff5373"))
    ;

bool CheckBlock(int nHeight, const uint256 &hash) {
    MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);
    MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
    if(i == checkpoints.end()) {
        return(true);
    }
    return hash == i->second;
}

int GetTotalBlocksEstimate() {
    MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);
    return checkpoints.rbegin()->first;
}

CBlockIndex *GetLastCheckpoint(const std::map<uint256, CBlockIndex *> &mapBlockIndex) {
    MapCheckpoints &checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);
    BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type & i, checkpoints) {
        const uint256 &hash = i.second;
        std::map<uint256, CBlockIndex *>::const_iterator t = mapBlockIndex.find(hash);
        if(t != mapBlockIndex.end()) {
            return t->second;
        }
    }
    return NULL;
}
}
