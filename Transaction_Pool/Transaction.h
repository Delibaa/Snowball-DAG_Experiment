//
// Created by shy71 on 2023/2/28.
//

#ifndef TRANSACTION_POOL_TRANSACTION_H
#define TRANSACTION_POOL_TRANSACTION_H



#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include "Crypto.h"

using namespace std;


string create_one_transaction();

#endif //TRANSACTION_POOL_TRANSACTION_H
