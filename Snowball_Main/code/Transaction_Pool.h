#ifndef TRANSACTION_POOL_H
#define TRANSACTION_POOL_H

#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>
#include <mutex>
#include <condition_variable>

using namespace std;

typedef struct transaction
{
    int version;
    string tx;
    unsigned long time;
} transaction;

class Transaction_Pool
{
public:
    vector<transaction> tx_pool;
    unsigned long epoch_time;
    int epoch_now;

    bool locker_write = false;
    std::mutex lock;
    std::condition_variable can_write;

    Transaction_Pool();
    bool add_tx_in_pool(string tx, unsigned long time, bool &added);
    bool write_to_file();
};

#endif