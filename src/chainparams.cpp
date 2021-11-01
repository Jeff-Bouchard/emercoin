// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

#include "arith_uint256.h"
bool CheckProofOfWork2(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

struct thread_data {
   CBlock block;
   int32_t nTime;
   arith_uint256 target;
   Consensus::Params consensus;
};

void *SolveBlock(void *threadarg)
{
    struct thread_data *my_data;
    my_data = (struct thread_data *) threadarg;
    CBlock& block = my_data->block;
    int32_t& nTime = my_data->nTime;
    arith_uint256& target = my_data->target;

    block.nTime = nTime;
    block.nNonce = 0;
    bool ret;
    while (UintToArith256(block.GetHash()) > target) {
        if (block.nNonce == 2147483647)
            break;
        ++block.nNonce;
    }
    ret = CheckProofOfWork2(block.GetHash(), block.nBits, my_data->consensus);
    printf("!!!solved: ret=%d nNonce=%d, nTime=%d\n", ret, block.nNonce, block.nTime);
    assert(false);
    pthread_exit(NULL);
}

void MineGenesisBlock(const CBlock& genesis, const Consensus::Params& consensus)
{
    const int NUM_THREADS = 8;

    pthread_t threads[NUM_THREADS];
    struct thread_data td[NUM_THREADS];
    pthread_attr_t attr;
    void *status;
    int rc;
    int i = 0;

    // Initialize and set thread joinable
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for( i = 0; i < NUM_THREADS; i++ ) {
       printf("main() : creating thread\n");
       td[i].block = genesis;
       td[i].nTime = genesis.nTime+i;
       td[i].target = UintToArith256(consensus.bnInitialHashTarget);
       td[i].consensus = consensus;
       rc = pthread_create(&threads[i], &attr, SolveBlock, (void *)&td[i]);

       if (rc) {
          printf("Error:unable to create thread\n");
          exit(-1);
       }
    }

    // free attribute and wait for the other threads
    pthread_attr_destroy(&attr);
    for( i = 0; i < NUM_THREADS; i++ ) {
       rc = pthread_join(threads[i], &status);
    }
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTimeTx, uint32_t nTimeBlock, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(9999) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    txNew.nTime = nTimeTx;

    CBlock genesis;
    genesis.nTime    = nTimeBlock;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTimeTx, uint32_t nTimeBlock, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "2013: Emergence is inevitable! heideg.livejournal.com/313676.html";
    const CScript genesisOutputScript = CScript();
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTimeTx, nTimeBlock, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.BIP34Height = 212806;
        consensus.BIP34Hash = uint256S("0x00000000000000172a635091de597ef16848e9e6b7d3f3471c8724bc3fcc003d");
        consensus.BIP65Height = 212920;
        consensus.BIP66Height = 212806;
        consensus.MMHeight = 219809;
        consensus.V7Height = 311210;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~arith_uint256(0) >> 32;
        consensus.bnInitialHashTarget = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~arith_uint256(0) >> 32;
        consensus.nTargetTimespan = 7 * 24 * 60 * 60; // one week
        consensus.nTargetSpacing = 10 * 60;

        // emercoin: PoS spacing = nStakeTargetSpacing
        //           PoW spacing = depends on how much PoS block are between last two PoW blocks, with maximum value = nTargetSpacingMax
        consensus.nStakeTargetSpacing = 10 * 60;                // 10 minutes
        consensus.nTargetSpacingMax = 12 * consensus.nStakeTargetSpacing; // 2 hours
        consensus.nStakeMinAge = 60 * 60 * 24 * 30;             // minimum age for coin age
        consensus.nStakeMaxAge = 60 * 60 * 24 * 90;             // stake age of full weight
        consensus.nStakeModifierInterval = 6 * 60 * 60;         // time to elapse before new modifier is computed

        consensus.nCoinbaseMaturity = 32;
        consensus.nCoinbaseMaturityOld = 20;  // Used until block 193912 on mainNet.

        consensus.fPowAllowMinDifficultyBlocks = false;

        // The best chain should have at least this much work.
        consensus.nMinimumChainTrust = uint256S("0x000000000000000000000000000000000000000000000000002ceae94968cea4"); // at block 250 000

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x3b5b8bb145e5d267b06430582f5efc4a1cbe128a836cdf07ab2000c9caabe550"); // at block 250 000

        consensus.nRejectBlockOutdatedMajority = 850;
        consensus.nToCheckBlockUpgradeMajority = 1000;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xe6;
        pchMessageStart[1] = 0xe8;
        pchMessageStart[2] = 0xe9;
        pchMessageStart[3] = 0xe5;
        vAlertPubKey = ParseHex("04e14603d29d0a051df1392c6256bb271ff4a7357260f8e2b82350ad29e1a5063d4a8118fa4cc8a0175cb45776e720cf4ef02cc2b160f5ef0144c3bb37ba3eea58");
        nDefaultPort = 6661;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1386627289, 1386628033, 139946546, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000000bcccd459d036a588d1008fce8da3754b205736f32ddfd35350e84c2d"));
        assert(genesis.hashMerkleRoot == uint256S("0xd8eee032f95716d0cf14231dc7a238b96bbf827e349e75344c9a88e849262ee0"));

        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.push_back(CDNSSeedData("emercoin.com", "seed.emercoin.com"));
        // vSeeds.push_back(CDNSSeedData("emercoin.net", "seed.emercoin.net"));
        // vSeeds.push_back(CDNSSeedData("emergate.net", "seed.emergate.net"));
        vSeeds.push_back(CDNSSeedData("emcdns", "seed.emc"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,33);   // emercoin: addresses begin with 'E'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,92);   // emercoin: addresses begin with 'e'
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0,     uint256S("0x00000000bcccd459d036a588d1008fce8da3754b205736f32ddfd35350e84c2d"))
            ( 25000, uint256S("0x20cc6639e9593e4e9344e1d40a234c552da81cb90b991aed6200ff0f72a69719"))
            ( 50000, uint256S("0x4c3d02a982bcb47ed9e076f754870606a6892d258720dc13863e10badbfd0e78"))
            (100000, uint256S("0x0000000000000071c614fefb88072459cced7b9d9a9cffd04064d3c3d539ecaf"))
            (150000, uint256S("0x5d317133f36b13ba3cd335c142e51d7e7007c0e72fd8a0fef48d0f4f63f7827a"))
            (200000, uint256S("0x7af70a03354a9ae3f9bf7f6a1dd3da6b03dcc14f8d6ad237095d73dbeaf5184c"))
            (250000, uint256S("0x3b5b8bb145e5d267b06430582f5efc4a1cbe128a836cdf07ab2000c9caabe550"))
            (300000, uint256S("0xc1eceed5949ef15c5c2877be22da7af1a3414af17e5f96976da4306a617f1e99"))
            (365000, uint256S("0xaab448269c0e53c3058e31551e7d598306a9676ec63273909f224fe3df2459ae"))
            (500000, uint256S("0x5a066bbb01b0b4ba3c5dc430b132b01665d6beb0f232ba17c501b806f03e79b2"))
        };

        chainTxData = ChainTxData{
            // Data as of block ???
            1455207714, // * UNIX timestamp of last known number of transactions
            365081,     // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.005787037 // * estimated number of transactions per second after that timestamp
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x00000000097af4fce19ca3c9aa688a81a5440f054243112e7d348e8350697827");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.MMHeight = 0;
        consensus.V7Height = 457;
        consensus.powLimit = uint256S("0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~arith_uint256(0) >> 28;
        consensus.bnInitialHashTarget = uint256S("00000007ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); //~uint256(0) >> 29;
        consensus.nTargetTimespan = 7 * 24 * 60 * 60; // two week
        consensus.nTargetSpacing = 10 * 60;

        // emercoin: PoS spacing = nStakeTargetSpacing
        //           PoW spacing = depends on how much PoS block are between last two PoW blocks, with maximum value = nTargetSpacingMax
        consensus.nStakeTargetSpacing = 10 * 60;                // 10 minutes
        consensus.nTargetSpacingMax = 12 * consensus.nStakeTargetSpacing; // 2 hours
        consensus.nStakeMinAge = 60 * 60 * 24;                  // minimum age for coin age
        consensus.nStakeMaxAge = 60 * 60 * 24 * 90;             // stake age of full weight
        consensus.nStakeModifierInterval = 60 * 20;              // time to elapse before new modifier is computed

        consensus.nCoinbaseMaturity = 1;
        consensus.nCoinbaseMaturityOld = 1;

        consensus.fPowAllowMinDifficultyBlocks = true;

        // The best chain should have at least this much work.
        consensus.defaultAssumeValid = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.nRejectBlockOutdatedMajority = 450;
        consensus.nToCheckBlockUpgradeMajority = 500;

        pchMessageStart[0] = 0xcb;
        pchMessageStart[1] = 0xf2;
        pchMessageStart[2] = 0xc3;
        pchMessageStart[3] = 0xef;
        nDefaultPort = 6663;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1386627290, 1386628036, 38942574, 0x1d0fffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000000642cfda7d39a8281e1f8791ceb240ce2f5ed9082f60040fe4210c6a58"));
        assert(genesis.hashMerkleRoot == uint256S("0xbb898f6696fd0bc265978aa375b61a82aacfe6a95267d12605c82d11971d220e"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back(CDNSSeedData("emercoin", "tnseed.emercoin.com"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);     // Testnet pubkey hash: m or n
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);     // Testnet script hash: 2
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0,      uint256S("0x0000000642cfda7d39a8281e1f8791ceb240ce2f5ed9082f60040fe4210c6a58"))
            ( 10,     uint256S("0x000000003623656ac54d127c08c24e5b06530a85d00c306dc6e7f171b74323ca"))
            ( 200000, uint256S("0x3cf0c1385c54d5b2bf72df333a4b48b94ef71f82f52404222b66d5b78f5bdfa7"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 0; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 0; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.MMHeight = 0;
        consensus.V7Height = 457;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.bnInitialHashTarget = uint256S("00000007ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); //~uint256(0) >> 29;
        consensus.nTargetTimespan = 7 * 24 * 60 * 60; // one week
        consensus.nTargetSpacing = 10 * 60;

        // emercoin: PoS spacing = nStakeTargetSpacing
        //           PoW spacing = depends on how much PoS block are between last two PoW blocks, with maximum value = nTargetSpacingMax
        consensus.nStakeTargetSpacing = 10 * 60;                // 10 minutes
        consensus.nTargetSpacingMax = 12 * consensus.nStakeTargetSpacing; // 2 hours
        consensus.nStakeMinAge = 60 * 60 * 24;                  // minimum age for coin age
        consensus.nStakeMaxAge = 60 * 60 * 24 * 90;             // stake age of full weight
        consensus.nStakeModifierInterval = 6 * 20;              // time to elapse before new modifier is computed

        consensus.nCoinbaseMaturity = 32;
        consensus.nCoinbaseMaturityOld = 32;

        consensus.fPowAllowMinDifficultyBlocks = true;

        // The best chain should have at least this much work.
        consensus.nMinimumChainTrust = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.nRejectBlockOutdatedMajority = 850;
        consensus.nToCheckBlockUpgradeMajority = 1000;

        pchMessageStart[0] = 0xcb;
        pchMessageStart[1] = 0xf2;
        pchMessageStart[2] = 0xc0;
        pchMessageStart[3] = 0xef;
        nDefaultPort = 6664;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1386627289, 1386628033, 18330017, 0x1d0fffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000000810da236a5c9239aa1c49ab971de289dbd41d08c4120fc9c8920d2212"));
        assert(genesis.hashMerkleRoot == uint256S("0xd8eee032f95716d0cf14231dc7a238b96bbf827e349e75344c9a88e849262ee0"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x0000000642cfda7d39a8281e1f8791ceb240ce2f5ed9082f60040fe4210c6a58"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}
