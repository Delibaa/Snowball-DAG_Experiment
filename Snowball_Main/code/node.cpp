#include <ctime>
#include <iostream>
#include <string>
#include <list>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include "Blockchain.hpp"
#include "miner.h"
#include "MyServer.hpp"
#include "crypto_stuff.h"
#include "get_ip.h"
#include "configuration.h"
#include "misc.h"
#include "Consensus_Group.h"
#include "Transaction_Pool.h"

using boost::asio::ip::tcp;
using namespace std;
using namespace std::chrono;

Blockchain *bc;
Consensus_Group *cg;
Transaction_Pool *tp;
string my_ip = "badip";
uint32_t my_port;
mt19937 rng;
unsigned long time_of_start;
unsigned long time_of_consensus_group_start;
unsigned long time_of_pool_start;
boost::thread *mythread;
tcp_server *ser = NULL;

std::condition_variable go_on_mining;
std::mutex mining_lock;
bool mining;

int main(int argc, char **argv)
{

    uint32_t random_seed = (((argc > 1 && atoi(argv[1]) != 0) ? atoi(argv[1]) : 1) * random_device()()) * time(NULL);
    rng.seed(random_seed);

    if (argc <= 2)
    {
        printf("Need two parameters (port,file_peers). Only %d provided.\n", argc - 1);
        exit(1);
    }

    /*
     * Get my ip and port,
     * getting IP with google DNS, which serves when you have to run code in different servers
     */
    string ip = get_my_local_ip();
    uint32_t port = atoi(argv[1]);
    my_ip = ip;
    my_port = port;

    /*
     * If third argument is provided, then it is the public IP
     * in a single server model, it's 127.0.0.1
     */
    if (argc >= 4)
    {
        string some_ip = string(argv[3]);
        if (split(some_ip, ".").size() == 4)
        {
            ip = my_ip = some_ip;
            cout << "Provided public ip:" << my_ip << ":" << endl;
        }
    }

    /*
     * If fourth argument is provided, then it is the private IP
     */
    string private_ip = "";
    if (argc >= 5)
    {
        string some_ip = string(argv[4]);
        if (split(some_ip, ".").size() == 4)
        {
            private_ip = some_ip;
            cout << "Provided private ip:" << private_ip << ":" << endl;
        }
    }

    /*
     * Create folders for Blockchains (where full blocks are stored),
     * Sessions (where communication is stored),
     * Hashes(the hash of max block - 6 of chain 0),
     * Transaction_pool(the partial block generated from transaction pool).
     * Check params.h to see how they are set
     */
    string folders[] = {string(FOLDER_BLOCKCHAIN), string(FOLDER_SESSIONS), string(FOLDER_HASHES), string(FOLDER_PINGS), string(FOLDER_BLOCKS)};
    for (int i = 0; i < 5; i++)
    {
        if (boost::filesystem::is_directory(folders[i]))
        {
            continue;
        }
        boost::filesystem::path dir(folders[i].c_str());
        if (!boost::filesystem::create_directory(dir))
        {
            cout << "Cannot create directory " << folders[i] << endl;
            cout << "Exiting..." << endl;
            // exit(1);
        }
        else
            cout << "[+] Created folder " << folders[i] << endl;
    }

    string folder = string(FOLDER_TRANSACTION_POOL);
    if (!boost::filesystem::is_directory(folder))
    {
        cout << "Cannot find the transaction pool folder" << endl;
        cout << "Exiting..." << endl;
        exit(1);
    }

    /*
     * Set the configuration from a file
     */
    string config_file = string(FILE_CONFIGURATION);
    cout << "[+] Set configuration from " << config_file << endl;
    set_configuration(config_file);

    /*
     * The blockchain
     */
    BlockHash hashes[MAX_CHAINS];
    for (int i = 0; i < CHAINS; i++)
        hashes[i] = i;
    bc = new Blockchain(hashes);

    /*
     * The Consensus_Group
     */
    cg = new Consensus_Group();

    /*
     * The Transaction_Pool
     * Setting epoch_time of 60 sec
     */
    tp = new Transaction_Pool();
    time_of_pool_start = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);

    /*
     * The server
     * start accept right away
     */
    boost::asio::io_service io_service;
    tcp_server server(io_service, ip, atoi(argv[1]));
    ser = &server;

    /*
     * Create the folder for blockchain
     */
    string _filePath = server.get_server_folder();
    const char *path = _filePath.c_str();
    boost::filesystem::path dir(path);
    if (!boost::filesystem::create_directory(dir))
    {
        cout << "Cannot create directory " << _filePath << endl;
        cout << "Exiting..." << endl;
        exit(1);
    }

    /*
     * Set keys for ECC
     */
    prepare_ecc_crypto(string(FILE_ECC_KEY));
    string some_test_string = "123";
    if (!verify_message(some_test_string, sign_message(some_test_string)))
    {
        cout << "Something is wrong with ECC" << endl
             << "Exiting..." << endl;
        exit(3);
    }

    /*
     * Peers of full node, flat communication
     */
    ifstream infile(argv[2]);
    string l;
    cout << "[ ] Adding peers from " << argv[2] << endl;
    std::vector<Peers> tmp;
    while (getline(infile, l))
    {
        int p = l.find(":");
        if (p > 0)
        {
            Peer pr;
            pr.ip = l.substr(0, p);
            pr.port = atoi(l.substr(p + 1, l.length()).c_str());
            if (pr.ip != my_ip && pr.ip != private_ip || pr.port != port)
                tmp.push_back(pr);
        }
    }
    infile.close();
    int tr = 0;
    while (tr < 10000 && tmp.size() > 0 && server.no_peers() < MAX_PEERS)
    {

        int t = rng() % tmp.size();
        if (tmp[t].ip != my_ip || tmp[t].port != port)
        {
            server.add_peer(tmp[t], false);
            tmp.erase(tmp.begin() + t);
        }
    }
    server.print_peers();

    /*
     * Load allowed IP for connecting peers,including the peers you want to connect
     */
    if (REJECT_CONNECTIONS_FROM_UNKNOWNS)
    {
        ifstream infile2(FILE_PEER_IPS);
        string l;
        cout << "[ ] Adding IPs for connecting peers from " << FILE_PEER_IPS << endl;
        while (getline(infile2, l))
        {
            string ip = string(l);
            boost::trim(ip);
            vector<std::string> sp = split(ip, ".");
            if (sp.size() == 4)
            {
                server.add_peers_ip(ip);
                cout << "\tAdd ::" << ip << "::" << endl;
            }
        }
        infile2.close();
    }

    /*
     * Start the mining thread
     */
    boost::thread t1(miner, bc, cg);
    mythread = &t1;

    /*
     * Start the server
     */
    server.run_network();
    time_of_start = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    time_of_pool_start = time_of_start;
    io_service.run();

    return 0;
}