//
// Created by shy71 on 2023/2/28.
//

#ifndef TRANSACTION_POOL_CRYPTO_H
#define TRANSACTION_POOL_CRYPTO_H

#define DUMMY_SIGNATURE "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
#define FILE_ECC_KEY "_ecc_key"
#define _FOLDER_TRANSACTION_POOL "_Transaction_pool"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

using namespace std;


string sha256(const string str);

void prepare_ecc_crypto( string filename_key_pem );

string sign_message( string message );
bool verify_message( string message, string signature);









#endif //TRANSACTION_POOL_CRYPTO_H
