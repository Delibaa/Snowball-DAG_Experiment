//
// Created by shy71 on 2023/3/13.
//

#ifndef CONSENSUS_GROUP_H
#define CONSENSUS_GROUP_H

#include <vector>
#include <map>
#include <string>
#include "Blockchain.hpp"
#include "MyServer.hpp"
#include "params.h"

using namespace std;

typedef struct miner_info
{
    std::string ip;
    uint32_t port;
    int share_of_block;
} miner_info;

typedef struct roundinfo
{

    int round;
    pair<string, uint32_t> leader_info;
    vector<miner_info> miner;
    float time_consumption;

} round_info;

class Consensus_Group
{
public:
    bool state;
    int concurrency_block_numbers;

    int round;
    pair<string, uint32_t> leader_info;
    vector<miner_info> miner_list;

    map<int, float> consensus_time;
    map<int, round_info> history_info;
    map<int, round_info> future_info;

    bool locker_write = false;
    std::mutex lock;
    std::condition_variable can_write;

    Consensus_Group();

    void join_Consensus_group(std::string ip, uint32_t port, bool certificate, int round_now);
    int get_concurrency_block_numbers();
    bool is_consensus_started();

    bool legitimate_certificate(bool certificate);
    bool is_in_Consensus_Group(std::string ip, uint32_t port);
    miner_info *get_member_in_Consensus_Group(std::string ip, uint32_t port);
    bool add_in_history(int round, vector<miner_info> miner_list, float time);
    bool add_in_future(int round, std::string ip, uint32_t port);

    bool is_in_future_consensus_round(std::string ip, uint32_t port, int round);
    miner_info *get_member_in_future_consensus_round(std::string ip, uint32_t port, int round);

    // 这个函数需要改进，使用rand()生成随机数
    pair<string, uint32_t> choose_leader();

    void start_consensus_of_blocks();
    void print_consensus_info();
};

#endif
