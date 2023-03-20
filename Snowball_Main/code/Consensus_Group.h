//
// Created by shy71 on 2023/3/13.
//

#ifndef CONSENSUS_GROUP_H
#define CONSENSUS_GROUP_H

# include <vector>
# include <map>
# include <string>
# include "Blockchain.hpp"
# include "MyServer.hpp"

using namespace std;


typedef struct miner_info{
    std::string ip;
    uint32_t port;
    int share_of_block;
}miner_info;


typedef struct historyinfo{

    int round;
    vector<miner_info> miner;
    float time_consumption;

}history_info;


class Consensus_Group{
public:

    int concurrency_block_numbers;
    vector<miner_info> miner_list;
    int round;
    bool state;
    map<int, float> consensus_time;
    map<int, history_info> history;


    bool locker_write = false;
    std::mutex lock;
    std::condition_variable can_write;

    Consensus_Group();

    void join_Consensus_group(std::string ip, uint32_t port, bool certificate);
    int get_concurrency_block_numbers();
    bool is_consensus_started();

    bool legitimate_certificate(bool certificate);
    bool is_in_Consensus_Group(std::string ip, uint32_t port);
    miner_info *get_member_in_Consensus_Group(std::string ip, uint32_t port);
    bool add_in_history(int round, vector<miner_info> miner_list, float time);

    //这个函数需要改进，使用rand()生成随机数
    pair<string, uint32_t> choose_leader();

    void start_consensus_of_blocks();
    void print_consensus_info();//实验数据断点用


};


#endif
