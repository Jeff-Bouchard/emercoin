#include <kernelrecord.h>

#include <wallet/wallet.h>
#include <base58.h>
#include <timedata.h>
#include <chainparams.h>
#include <key_io.h>

#include <math.h>
using namespace std;

bool KernelRecord::showTransaction(bool isCoinbase, int depth)
{
    if (isCoinbase) {
        if (depth < 2)
            return false;
    } else {
        if (depth == 0)
            return false;
    }

    return true;
}

/*
 * Decompose CWallet transaction to model kernel records.
 */
vector<KernelRecord> KernelRecord::decomposeOutput(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx)
{
    const Consensus::Params& params = Params().GetConsensus();
    vector<KernelRecord> parts;
    int64_t nTime = wtx.tx->nTime;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;
    int nDayWeight = (min((GetAdjustedTime() - nTime), params.nStakeMaxAge) - params.nStakeMinAge) / 86400;

    int numBlocks;
    interfaces::WalletTxStatus status;
    interfaces::WalletOrderForm orderForm;
    bool inMempool;
    wallet.getWalletTxDetails(hash, status, orderForm, inMempool, numBlocks);

    if (showTransaction(wtx.is_coinbase, status.depth_in_main_chain)) {
        for (size_t nOut = 0; nOut < wtx.tx->vout.size(); nOut++) {
            CTxOut txOut = wtx.tx->vout[nOut];
            if (wallet.txoutIsMine(txOut)) {
                CTxDestination address;
                std::string addrStr;

                uint64_t coinAge = max(txOut.nValue * nDayWeight / COIN, (int64_t)0);
#if 0
                // oleg: Old code from Peercoin/EM
                if (ExtractDestination(txOut.scriptPubKey, address)) {
                    // Sent to Bitcoin Address
                    addrStr = EncodeDestination(address);
                } else {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    addrStr = mapValue["to"];
                }
#endif
                // We add only mintable UTXOs into minting list
                // don't add nonstandard or name UTXOs
                txnouttype utxo_type = ExtractDestination(txOut.scriptPubKey, address);
                if (
                    utxo_type == TX_PUBKEY ||
                    utxo_type == TX_PUBKEYHASH ||
                    utxo_type == TX_SCRIPTHASH ||
                    utxo_type == TX_WITNESS_V0_SCRIPTHASH ||
                    utxo_type == TX_WITNESS_V0_KEYHASH
                ) {
                    std::vector<interfaces::WalletTxOut> coins = wallet.getCoins({COutPoint(hash, nOut)});
                    bool isSpent = coins.size() >= 1 ? coins[0].is_spent : true;
                    parts.push_back(KernelRecord(hash, nTime, addrStr, txOut.nValue, nOut, isSpent, coinAge));
                }
            }
        }
    }

    return parts;
}

std::string KernelRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}

int64_t KernelRecord::getAge() const
{
    return (GetAdjustedTime() - nTime) / 86400;
}

double KernelRecord::getProbToMintStake(double difficulty, int timeOffset) const
{
    const Consensus::Params& params = Params().GetConsensus();
    double maxTarget = pow(static_cast<double>(2), 224);
    double target = maxTarget / difficulty;
    int dayWeight = (min((GetAdjustedTime() - nTime) + timeOffset, params.nStakeMaxAge) - params.nStakeMinAge) / 86400;
    uint64_t coinAge = max(nValue * dayWeight / COIN, (int64_t)0);
    return target * coinAge / pow(static_cast<double>(2), 256);
}

double KernelRecord::getProbToMintWithinNMinutes(double difficulty, int minutes)
{
    if(difficulty != prevDifficulty || minutes != prevMinutes)
    {
        double prob = 1;
        double p;
        int d = minutes / (60 * 24); // Number of full days
        int m = minutes % (60 * 24); // Number of minutes in the last day
        int i, timeOffset;

        // Probabilities for the first d days
        for(i = 0; i < d; i++)
        {
            timeOffset = i * 86400;
            p = pow(1 - getProbToMintStake(difficulty, timeOffset), 86400);
            prob *= p;
        }

        // Probability for the m minutes of the last day
        timeOffset = d * 86400;
        p = pow(1 - getProbToMintStake(difficulty, timeOffset), 60 * m);
        prob *= p;

        prob = 1 - prob;
        prevProbability = prob;
        prevDifficulty = difficulty;
        prevMinutes = minutes;
    }
    return prevProbability;
}
