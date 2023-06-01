#include "Transaction_Pool.h"
#include "params.h"

extern unsigned long time_of_pool_start;
extern string my_ip;
extern uint32_t my_port;

Transaction_Pool::Transaction_Pool()
{
    tx_pool.clear();
    epoch_time = EPOCH_TIME;
    epoch_now = 1;
}

bool Transaction_Pool::add_tx_in_pool(string tx, unsigned long time, bool &added)
{
    transaction new_tx;
    new_tx.version = epoch_now;
    new_tx.tx = tx;
    new_tx.time = time;
    tx_pool.push_back(new_tx);
    added = true;

    int concurrency_size = CONCURRENCY_BLOCKS;
    if (tx_pool.size() >= concurrency_size * TX_NUMBERS_IN_A_BLOCK)
    {
        printf("\n[+] Receiving transactions size of %d concurrency blocks\n", concurrency_size);
        fflush(stdout);
    }

    // comuputing sync rate
    if (tx_pool.size() >= (3000 * epoch_now))
    {
        if (write_to_file())
        {
            unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
            float secs = (time_of_now - time_of_pool_start) / 1000.0;
            printf("\n[+]Time durtion of receiving txs is %.2fs\n", secs);
            return true;
        }
        else
        {
            cout << "Write false!" << endl;
            return false;
        }
    }

    return false;
}

bool Transaction_Pool::write_to_file()
{

    string filename = string(FOLDER_TRANSACTION_POOL) + "/_" + my_ip + "_" + to_string(my_port) + "/" + "transactions/" + "EPOCH_" + to_string(epoch_now);

    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Cannot create document: " << filename << endl;
        return false;
    }

    for (auto iter = tx_pool.begin(); iter != tx_pool.end(); iter++)
    {
        string l = to_string(iter->version) + ":" + iter->tx + ":" + to_string(iter->time);
        file << l << endl;
    }

    file.close();
    epoch_now++;

    return true;
}