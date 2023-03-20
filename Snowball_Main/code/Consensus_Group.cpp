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


Consensus_Group::Consensus_Group()
{
    concurrency_block_numbers = 0;
    miner_list.clear();
    consensus_time.clear();
    history.clear();
    round = 1;
    state = false;

}

void Consensus_Group::join_Consensus_group(std::string ip, uint32_t port, bool certificate)
{
    if(is_consensus_started()){

        if(PRINT_CONSENSUS_MESSAGE){
             printf("\033[33;1m[ ] Consensus has been started! join error \n\033[0m");
        }
        
        return;

    }

    if(!legitimate_certificate(certificate)){
        printf("======Not a true miner, join error======\n");
        fflush(stdout);
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


miner_info *Consensus_Group::get_member_in_Consensus_Group(std::string ip, uint32_t port)
{
    for(int i = 0; i< miner_list.size(); i++){
        if(miner_list[i].ip == ip && miner_list[i].port == port) return &miner_list[i];
    }

    printf("=======the node with ip:%s:%d is not the member of consensus group=======\n", ip.c_str(), port);
    return NULL;
}

pair<string, uint32_t> Consensus_Group::choose_leader()
{
    //需要后续修改为随机硬币，输入相同的所有人得到相同的输出
    //uint32_t leader_port = miner_list[(rand()%(miner_list.size()))].port;
    uint32_t leader_port = 8001;
    string leader_ip = "127.0.0.1";
    pair<string, uint32_t> leader_info(leader_ip, leader_port);
    return leader_info;
}

//开始共识
void Consensus_Group::start_consensus_of_blocks()
{

    state = true;
    if(PRINT_CONSENSUS_MESSAGE){
        printf("\033[33;1m[ ] Consensus round %d will start. \n\033[0m ",round);
    }
    

}


bool Consensus_Group::is_consensus_started() {
    
    return state;

}

bool Consensus_Group::add_in_history(int round, vector<miner_info> miner_list, float time){

    //A history
    history_info A_history;
    A_history.round = round;
    for(int i = 0; i < miner_list.size();i++){
        A_history.miner.push_back(miner_list[i]);
    }
    A_history.time_consumption = time;

    //upadating consensus group
    consensus_time.insert(pair<int, float>(round, time));
    history.insert(pair<int, history_info>(round, A_history));

    return true;

}


bool Consensus_Group::legitimate_certificate(bool certificate) {
    //修改成为验证逻辑
    if(certificate) return true;
    return false;
}

void Consensus_Group::print_consensus_info(){

    string tx = create_one_transaction();
    uint32_t tx_size = tx.size();
    unsigned long bytes_local_total_verify = bc->total_verify_local_block*500*tx_size;
    unsigned long time_of_finish = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
    float secs = (time_of_finish - time_of_start)/ 1000.0;

    printf("\n=============== [CONSENSUS GROUP INFO: ]   Round:  %d     Block Members:  %d     Consensus started:  %d\n", round, concurrency_block_numbers, state);
    printf("\n=============== [LOCAL WAITING QUEUE: ]   Waiting to Verify:  %lu\n",bc->waiting_for_phase_1_block.size());
    printf("\n=============== [LOCAL CONSENSUS THROUGHPUT: ]   txs MB/s:  %.2f  txs GB/h:  %.1f\n", bytes_local_total_verify/(1024.0*1024)/secs,bytes_local_total_verify/(1024.0*1024)/secs * 3600/1000 );
    printf("\n=============== [PAST CONSENSUS TIME: ]\n");
    if(!consensus_time.empty()){
        for(int i =1; i<= consensus_time.size();i++){
            printf("\n===============The round %d finished in %.2f\n",consensus_time.find(i)->first, consensus_time.find(i)->second);
        }
    }
}

