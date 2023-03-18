#include <iostream>
#include "Crypto.h"
#include "Transaction.h"
#include <fstream>


int main() {

    //test ECC work
    prepare_ecc_crypto( string(FILE_ECC_KEY) );
    string some_test_string = "123";
    if(  !verify_message( some_test_string, sign_message( some_test_string) ) ){
        cout << "Something is wrong with ECC" << endl <<"Exiting..."<<endl;
        exit(3);
    }

    int block = 1;
    int block_numbers = 10000;
    uint32_t tx_size = create_one_transaction().size();

    cout << "Single transaction size is " << tx_size <<endl;

    uint32_t block_size = 20480;

    for(; block <= block_numbers; block++){
        string path = string(_FOLDER_TRANSACTION_POOL)+ "/" + "block_" + to_string(block);

        uint32_t l = 0;

        ofstream file;
        try {
            file.open(path);
        }
        catch( const std::string& ex){ return false; }

        //generate the transaction pool, first with a pool with small scale
        while (l < block_size){
            string tx = create_one_transaction();
            file << tx << endl;
            l +=tx_size;
        }

        file.close();
    }

    return 0;
}
