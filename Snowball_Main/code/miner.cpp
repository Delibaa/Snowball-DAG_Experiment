/*
Copyright (c) 2018, Ivica Nikolic <cube444@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <openssl/sha.h>
#include "Blockchain.hpp"
#include "MyServer.hpp"

#include "verify.h"
#include "transactions.h"
#include "crypto_stuff.h"
#include "requests.h"
#include "miner.h"
#include "Consensus_Group.h"

using namespace std;

extern mt19937 rng;
extern tcp_server *ser;
extern boost::thread *mythread;

extern std::condition_variable go_on_mining;
extern std::mutex mining_lock;
extern bool mining;

uint32_t total_mined = 0;

bool mine_new_block(Blockchain *bc, consensus_part cp)
{
    // mutex of blockchain
    std::unique_lock<std::mutex> l(bc->lock);
    bc->can_write.wait(l, [bc]()
                       { return !bc->locker_write; });
    bc->locker_write = true;

    // Concatenate the candidates of all chains
    vector<string> leaves;

    // Last block of the trailing chain
    block *trailing_block = bc->get_deepest_child_by_chain_id(0);
    int trailing_id = 0;
    for (int i = 0; i < CHAINS; i++)
    {

        block *b = bc->get_deepest_child_by_chain_id(i);
        if (NULL == b)
        {
            cout << "Something is wrong in mine_new_block: get_deepest return NULL" << endl;
            exit(3);
        }
        if (NULL == b->nb)
        {
            cout << "Something is wrong in mine_new_block: get_deepest return block with NULL nb pointer" << endl;
            exit(3);
        }
        if (b->nb->next_rank > trailing_block->nb->next_rank)
        {
            trailing_block = b;
            trailing_id = i;
        }

        // leaves of all chains to be proof of next round
        leaves.push_back(blockhash_to_string(b->hash));
    }

    // Make a complete binary tree
    uint32_t tot_size_add = (int)pow(2, ceil(log(leaves.size()) / log(2))) - leaves.size();
    for (int i = 0; i < tot_size_add; i++)
        leaves.push_back(EMPTY_LEAF);

    // hash to produce the hash of the new block
    string merkle_root_chains = compute_merkle_tree_root(leaves);

    //这个地方不需要改，只需要作为2f+1引用取块的代替，毕竟只验证上面部分
    string merkle_root_txs = to_string(rng());
    string h = sha256(merkle_root_chains + merkle_root_txs);

    // Determine the chain where it should go
    uint32_t chain_id = get_chain_id_from_hash(h);

    // Determine the new block
    BlockHash new_block = string_to_blockhash(h);

    // Create a block
    uint32_t no_txs = TX_NUMBERS_IN_A_BLOCK;

    //1.根据cp中的order创建blokchain文件 2.更新tx_list为交易的哈希值 3.更新cp中的交易默克尔根
    bool tmp = create_block_from_transaction_pool(&cp, ser->get_server_folder() + "/" + blockhash_to_string(new_block));

    if (!tmp)
    {
        printf("Cannot create the file with transactions\n");
        fflush(stdout);
        return false;
    }

    // Find Merkle path for the winning chain
    vector<string> proof_new_chain = compute_merkle_proof(leaves, chain_id);

    // Last block of the chain where new block will be mined
    block *parent = bc->get_deepest_child_by_chain_id(chain_id);

    network_block nb;
    nb.chain_id = chain_id;
    nb.parent = parent->hash;
    nb.hash = new_block;
    nb.trailing = trailing_block->hash;
    nb.trailing_id = trailing_id;
    nb.merkle_root_chains = merkle_root_chains;
    nb.merkle_root_txs = merkle_root_txs;
    nb.proof_new_chain = proof_new_chain;
    nb.no_txs = no_txs;
    nb.rank = parent->nb->next_rank;
    nb.next_rank = trailing_block->nb->next_rank;
    if (nb.next_rank <= nb.rank)
        nb.next_rank = nb.rank + 1;

    nb.depth = parent->nb->depth + 1;
    unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    nb.time_mined = time_of_now;
    nb.time_received = time_of_now;
    for (int j = 0; j < NO_T_DISCARDS; j++)
    {
        nb.time_commited[j] = 0;
        nb.time_partial[j] = 0;
    }

    // consensus part
    nb.consensusPart.round = cp.round;
    nb.consensusPart.order_in_round = cp.order_in_round;
    nb.consensusPart.tx_list = cp.tx_list;
    nb.consensusPart.merkel_root_of_txs = merkle_root_txs;
    nb.consensusPart.verify_1_numbers = cp.verify_1_numbers;
    nb.consensusPart.verify_2_numbers = cp.verify_2_numbers;

    bc->add_block_by_parent_hash_and_chain_id(nb.parent, nb.hash, nb.chain_id, nb);

    block *bz = bc->find_block_by_hash_and_chain_id(nb.hash, nb.chain_id);

    if (NULL != bz && NULL != bz->nb)
    {
        if (PRINT_MINING_MESSAGES)
        {
            printf("\033[33;1m[+] Mined block on chain[%d] : [%lx %lx]\n\033[0m", chain_id, parent->hash, new_block);
            fflush(stdout);
        }
    }

    //add for Phase validate
    bc->add_waiting_for_phase_1_blocks(nb.hash, nb);
    bc->add_mined_block();

    bc->locker_write = false;
    l.unlock();
    bc->can_write.notify_one();

    return true;
}

bool mine_Consensus_blocks(Blockchain *bc, Consensus_Group *cg)
{

    // Phase REQUEST
    int order_in_round = 1;
    int tx_list = 1;

    for (int i = 0; i < cg->miner_list.size(); i++)
    {
        consensus_part consensus_part_tmp;
        int block_numbers = cg->miner_list[i].share_of_block;

        if (cg->miner_list[i].ip == my_ip && cg->miner_list[i].port == my_port)
        {
            for (int j = 0; j < block_numbers; j++)
            {

                // pre set 500 txs
                consensus_part_tmp.round = cg->round;
                consensus_part_tmp.order_in_round = order_in_round;
                //这个地方去掉tx_list的赋值，在mine_new_block里面
                consensus_part_tmp.tx_list = pair<int, int>(tx_list, tx_list + TX_NUMBERS_IN_A_BLOCK);
                consensus_part_tmp.verify_1_numbers = cg->miner_list[i].share_of_block;
                consensus_part_tmp.verify_2_numbers = cg->miner_list[i].share_of_block;

                mine_new_block(bc, consensus_part_tmp);
                order_in_round++;
                tx_list += TX_NUMBERS_IN_A_BLOCK;
            }
        }
        else
        {
            for (int j = 0; j < block_numbers; j++)
            {

                consensus_part_tmp.round = cg->round;
                consensus_part_tmp.order_in_round = order_in_round;
                //这个地方根据order_in_round映射，然后将映射区块的哈希列表放到tx_list里
                consensus_part_tmp.tx_list = pair<int, int>(tx_list, tx_list + TX_NUMBERS_IN_A_BLOCK);
                order_in_round++;
                tx_list += TX_NUMBERS_IN_A_BLOCK;

                // waiting for phase REQUEST
                bc->add_pre_blocks(cg->miner_list[i].ip, cg->miner_list[i].port, consensus_part_tmp);
            }

            if (PRINT_CONSENSUS_MESSAGE)
            {
                printf("\033[34;1m WILL send %d Consensus block to the peer %s:%d\033[0m\n", block_numbers, cg->miner_list[i].ip.c_str(), cg->miner_list[i].port);
                fflush(stdout);
            }
        }
    }
    return true;
}

// mining sumulation
uint32_t get_mine_time_in_milliseconds()
{

    std::exponential_distribution<double> exp_dist(1.0 / EXPECTED_MINE_TIME_IN_MILLISECONDS);
    uint32_t msec = exp_dist(rng);

    if (PRINT_MINING_MESSAGES)
    {
        printf("\033[33;1m[ ] Will mine new block in  %.3f  seconds \n\033[0m", (float)msec / 1000);
        fflush(stdout);
    }

    return msec;
}

void miner(Blockchain *bc, Consensus_Group *cg)
{
    // time to connect network first
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    // mining loop
    while (true)
    {

        try
        {

            bool MINER_CERTIFICATE = false;
            // round
            int round_now = cg->round;

            if (round_now > 1)
            {

                // references from last round
                int con_blocks = bc->get_numbers_of_concurrency_blocks_in_a_round(round_now - 1);

                // printf("\n++++++++ Test the concurrency Log point %d +++++\n", con_blocks);

                if (con_blocks == CONCURRENCY_BLOCKS)
                {

                    boost::this_thread::sleep(boost::posix_time::milliseconds(get_mine_time_in_milliseconds()));
                    MINER_CERTIFICATE = true;
                    if (PRINT_MINING_MESSAGES)
                    {
                        printf("\033[33;1m[ ] Mining succeed! \n\033[0m");
                    }
                }
                else
                {
                    if (PRINT_MINING_MESSAGES)
                    {
                        printf("\033[33;1m[ ] Not qualified for mining! \n\033[0m");
                    }
                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                    continue;
                }
            }

            if (round_now == 1)
            {
                boost::this_thread::sleep(boost::posix_time::milliseconds(get_mine_time_in_milliseconds()));
                MINER_CERTIFICATE = true;
                if (PRINT_MINING_MESSAGES)
                {
                    printf("\033[33;1m[ ] Mining succeed! \n\033[0m");
                }
            }

            boost::this_thread::interruption_point();

            // mutex of consensus group
            std::unique_lock<std::mutex> l(cg->lock);
            cg->can_write.wait(l, [cg]()
                               { return !cg->locker_write; });
            cg->locker_write = true;

            cg->join_Consensus_group(ser->get_ip(), ser->get_port(), MINER_CERTIFICATE, round_now);

            cg->locker_write = false;
            l.unlock();
            cg->can_write.notify_one();

            string random = to_string(rng());
            string s = create__mining_succeed(MINER_CERTIFICATE, random, cg->round);
            ser->write_to_all_peers(s);

            if (PRINT_SENDING_MESSAGES)
            {
                printf("\033[34;1mSENDING MINING SUCEED!! to peers\033[0m\n");
                fflush(stdout);
            }

            // start consensus group
            if (cg->get_concurrency_block_numbers() >= CONCURRENCY_BLOCKS)
            {

                cg->start_consensus_of_blocks();

                mining = false;

                pair<string, uint32_t> leader_info = cg->choose_leader();
                string leader_ip = leader_info.first;
                uint32_t leader_port = leader_info.second;

                if (PRINT_CONSENSUS_MESSAGE)
                {
                    printf("\033[33;1m[ ] The leader of consensus round %d is %s:%d. \n\033[0m\n", cg->round, leader_ip.c_str(), leader_port);
                }

                if (leader_port == my_port && leader_ip == my_ip)
                {
                    // attention transaction pool
                    mine_Consensus_blocks(bc, cg);
                }

                std::unique_lock<std::mutex> lock_mining(mining_lock);
                go_on_mining.wait(lock_mining, []
                                  { return mining; });

                // printf("\n+++++++++ test round continue,the round is %d++++++\n",cg->round);
            }
        }
        catch (boost::thread_interrupted &)
        {

            if (PRINT_CONSENSUS_MESSAGE)
            {
                printf("\033[33;1m[ ] Miner Interrupted!\n\033[0m");
            }
            fflush(stdout);

            std::unique_lock<std::mutex> lock_mining(mining_lock);
            go_on_mining.wait(lock_mining, []
                              { return mining; });

            // printf("\n+++++++++ test round continue,the round is %d++++++\n",cg->round);
        }
    }
}
