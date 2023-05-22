#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

#include "Blockchain.hpp"
#include "params.h"

extern mt19937 rng;
extern string my_ip;
extern uint32_t my_port;

using namespace std;


string create_one_transaction();
int create_transaction_block( BlockHash hash, string filename );
bool verify_transaction( string tx );

bool create_block_from_transaction_pool(consensus_part *cp, string filename);
bool generate_concurrent_blocks(consensus_part *cp);
bool verify_pre_block(consensus_part *cp);
bool verify_validate_block(network_block *nb);



#endif