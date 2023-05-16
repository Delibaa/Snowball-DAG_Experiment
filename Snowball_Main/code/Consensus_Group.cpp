//
// Created by shy71 on 2023/2/21.
//

# include <vector>
# include <string>
# include "Consensus_Group.h"
# include "stdio.h"

extern tcp_server *ser;
extern boost::thread *mythread;
extern Blockchain *bc;
extern unsigned long time_of_start;
extern unsigned long time_of_consensus_group_start;


Consensus_Group::Consensus_Group()
{
    state = false;
    concurrency_block_numbers = 0;

    round = 1;
    miner_list.clear();

    consensus_time.clear();
    history_info.clear();
    future_info.clear();

}

void Consensus_Group::join_Consensus_group(std::string ip, uint32_t port, bool certificate, int round_now)
{
    
    if(round_now == round && is_consensus_started()){

        printf("\033[33;1m[ ] CONSENSUS round %d has been started! Join error \n\033[0m", round_now);
        fflush(stdout);
        return;

    }

    if(!legitimate_certificate(certificate)){

        printf("\033[33;1m THE node with ip:%s:%d is not qualified for consensus group. Join error \n\033[0m",ip.c_str(), port);
        fflush(stdout);
        return;
    }

    if(round_now > round){

        add_in_future(round_now, ip, port);
        return;
    }

    if(is_in_Consensus_Group(ip,port)){

        miner_info *tmp = get_member_in_Consensus_Group(ip,port);
        tmp->share_of_block++;
    }
    else{

        miner_info miner = {ip, port, 1};
        miner_list.push_back(miner);
    }

    concurrency_block_numbers++;

}


int Consensus_Group::get_concurrency_block_numbers()
{
    return concurrency_block_numbers;
}


bool Consensus_Group::is_in_Consensus_Group(std::string ip, uint32_t port)
{
    for(int i = 0; i< miner_list.size(); i++){
        if(miner_list[i].ip == ip && miner_list[i].port == port) return true;
    }

    return false;
}

bool Consensus_Group::is_in_future_consensus_round(std::string ip, uint32_t port, int round){

    round_info tmp = future_info.find(round)->second;
    for(int i=0; i< tmp.miner.size(); i++){
        if(tmp.miner[i].ip == ip && tmp.miner[i].port == port) return true;
    }
    return false;
}


miner_info *Consensus_Group::get_member_in_Consensus_Group(std::string ip, uint32_t port)
{
    for(int i = 0; i< miner_list.size(); i++){
        if(miner_list[i].ip == ip && miner_list[i].port == port) return &miner_list[i];
    }

    printf("=======the node with ip:%s:%d is not the member of consensus group=======\n", ip.c_str(), port);
    return nullptr;
}

miner_info *Consensus_Group::get_member_in_future_consensus_round(std::string ip, uint32_t port, int round){

    for(int i=0; i< future_info.find(round)->second.miner.size(); i++){
        if(future_info.find(round)->second.miner[i].ip == ip && future_info.find(round)->second.miner[i].port == port) return &future_info.find(round)->second.miner[i];
    }

    return nullptr;
}

pair<string, uint32_t> Consensus_Group::choose_leader()
{
    //需要后续修改为随机硬币，输入相同的所有人得到相同的输出
    //uint32_t leader_port = miner_list[(rand()%(miner_list.size()))].port;
    uint32_t leader_port = 8001;
    string leader_ip = "127.0.0.1";

    leader_info.first = leader_ip;
    leader_info.second = leader_port;

    return leader_info;
}

void Consensus_Group::start_consensus_of_blocks()
{

    state = true;
    time_of_consensus_group_start = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
    if(PRINT_CONSENSUS_MESSAGE){
        printf("\033[33;1m[ ] Consensus round %d will start. \n\033[0m ",round);
    }
    

}

bool Consensus_Group::is_consensus_started() {
    
    return state;

}

bool Consensus_Group::add_in_history(int round, vector<miner_info> miner_list, float time){

    //A history
    round_info A_history;
    A_history.round = round;
    for(int i = 0; i < miner_list.size();i++){
        A_history.miner.push_back(miner_list[i]);
    }
    A_history.leader_info.first = leader_info.first;
    A_history.leader_info.second = leader_info.second;
    A_history.time_consumption = time;

    //upadating consensus group
    consensus_time.insert(pair<int, float>(round, time));
    history_info.insert(pair<int, round_info>(round, A_history));

    return true;

}

bool Consensus_Group::add_in_future(int round, std::string ip, uint32_t port){

    if( future_info.find(round) == future_info.end() ){

        //insert verified future round information
        miner_info miner;
        miner.ip = ip;
        miner.port = port;
        miner.share_of_block = 1;

        round_info future;
        future.round = round;
        future.miner.push_back(miner);
        future_info.insert(make_pair(round, future));

    }

    if ( future_info.find(round) != future_info.end() )
    {
        if (is_in_future_consensus_round(ip, port, round))
        {
            miner_info *tmp = get_member_in_future_consensus_round(ip,port,round);
            tmp->share_of_block++;
        }
        else{
            miner_info miner = {ip, port, 1};
            future_info.find(round)->second.miner.push_back(miner);
        }
        

    }
    
    return true;

}



bool Consensus_Group::legitimate_certificate(bool certificate) {
    //修改成为验证逻辑
    if(certificate) return true;
    return false;
}

void Consensus_Group::print_consensus_info(){

    unsigned long time_of_finish = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
    float secs = (time_of_finish - time_of_start)/ 1000.0;

    printf("\n=============== [CONSENSUS GROUP INFO: ]   Round:  %d     Block Members:  %d     Consensus state:  %s\n", round, concurrency_block_numbers, state?"running":"rest");
    printf("\n=============== [PHASE REQUEST WAITING QUEUE: ]   Leader_info:    %s:%d   Waiting to request: %lu\n", leader_info.first.c_str(),leader_info.second,bc->pre_blocks.size());
    printf("\n=============== [PHASE VALIDATE WAITING QUEUE: ]   Waiting to Verify:  %ld\n",bc->waiting_for_phase_1_block.size());
    if(!consensus_time.empty()){
        float ave_consensus_time =0;
        float total_consensus_time =0;
        for(int i =1; i<= consensus_time.size();i++){

            total_consensus_time += consensus_time.find(i)->second;

        }
        ave_consensus_time = total_consensus_time/consensus_time.size();
        printf("\n=============== [AVERAGE CONSENSUS TIME: ]    Time:   %0.2f   Concurrent Block Numbers:   %d\n",ave_consensus_time, CONCURRENCY_BLOCKS);
    }
}

