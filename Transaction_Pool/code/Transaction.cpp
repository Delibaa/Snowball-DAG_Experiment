//
// Created by shy71 on 2023/2/28.
//

#include "Transaction.h"

mt19937 rng;
bool fake_transactions = false;
uint32_t ADDRESS_SIZE_IN_DWORDS = 5;

using namespace std;


string get_random_address( uint32_t size_in_dwords )
{
    stringstream sstream;
    for( int i=0; i<size_in_dwords; i++)
        sstream << setfill('0') << setw(8) << hex << rng();

    return sstream.str();
}


string create_one_transaction()
{
    if( fake_transactions ){
        return "0000000000000000000000000000000000000000:0000000000000000000000000000000000000000:0000000000:00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    }

    string tx = get_random_address(ADDRESS_SIZE_IN_DWORDS) + ":" + get_random_address(ADDRESS_SIZE_IN_DWORDS) + ":" + to_string(rng() );
    string sign_tx = sign_message( tx );

    return tx +":"+sign_tx;
}