/*
Copyright (c) 2018, Ivica Nikolic <cube444@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "transactions.h"
#include "crypto_stuff.h"
#include "misc.h"
#include "params.h"
#include "verify.h"
#include <sstream>

using namespace std;

string get_random_address(uint32_t size_in_dwords)
{
	stringstream sstream;
	for (int i = 0; i < size_in_dwords; i++)
		sstream << setfill('0') << setw(8) << hex << rng();

	return sstream.str();
}

string create_one_transaction()
{
	if (fake_transactions)
	{
		return "0000000000000000000000000000000000000000:0000000000000000000000000000000000000000:0000000000:00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
	}

	string tx = get_random_address(ADDRESS_SIZE_IN_DWORDS) + ":" + get_random_address(ADDRESS_SIZE_IN_DWORDS) + ":" + to_string(rng());
	string sign_tx = sign_message(tx);

	return tx + ":" + sign_tx;
}

bool create_block_from_transaction_pool(consensus_part *cp, string filename)
{
	// store file of block
	string folder_transaction_pool = string(FOLDER_TRANSACTION_POOL);
	int block_number = (cp->round - 1) * CONCURRENCY_BLOCKS + cp->order_in_round;
	string block_filename = folder_transaction_pool + "/_" + my_ip + "_" + to_string(my_port) + "/" + "_body/" + "block_" + to_string(block_number);

	uint32_t no_txs = cp->tx_list.second - cp->tx_list.first;
	vector<string> leaves;

	if (WRITE_BLOCKS_TO_HDD)
	{
		ofstream file;
		ifstream file_of_transaction_pool;

		try
		{
			file.open(filename);
		}
		catch (const std::string &ex)
		{
			printf("\n\033[33;1m[!] Open file FALSE with path %s\n\033[0m\n", filename.c_str());
			fflush(stdout);
			return false;
		}

		file_of_transaction_pool.open(block_filename.c_str());

		if (!file_of_transaction_pool.is_open())
		{
			printf("\n\033[33;1m[!] Open Transaction pool file FALSE with path %s\n\033[0m\n", block_filename.c_str());
			fflush(stdout);
			exit(1);
		}

		string tx_demo = create_one_transaction();
		int tx_size = tx_demo.size();

		for (int i = 0; i < no_txs; i++)
		{
			char buffer[tx_size + 100];
			file_of_transaction_pool.getline(buffer, tx_size + 100, '\n');
			file << buffer << endl;
			leaves.push_back(buffer);
		}
		file.close();
		file_of_transaction_pool.close();
	}

	// create tx_merkel_root
	uint32_t tot_size_add = (int)pow(2, ceil(log(leaves.size()) / log(2))) - leaves.size();
	for (int i = 0; i < tot_size_add; i++)
		leaves.push_back(EMPTY_LEAF);
	string merkel_root_txs = compute_merkle_tree_root(leaves);
	cp->merkel_root_of_txs = merkel_root_txs;

	return true;
}

bool generate_concurrent_blocks(consensus_part *cp)
{

	string folder_transaction_pool = string(FOLDER_TRANSACTION_POOL);
	int block_number = (cp->round - 1) * CONCURRENCY_BLOCKS + cp->order_in_round;
	string filename = folder_transaction_pool + "/_" + my_ip + "_" + to_string(my_port) + "/" + "_header/" + "pre_block_" + to_string(block_number);

	ifstream file_of_transaction_pool;
	try
	{
		file_of_transaction_pool.open(filename);
	}
	catch (const std::string &ex)
	{
		printf("\n\033[33;1m[!] Open Transaction pool file FALSE with path %s\n\033[0m\n", filename.c_str());
		fflush(stdout);
		return false;
	}

	stringstream buffer;
	buffer << file_of_transaction_pool.rdbuf();
	cp->list += buffer.str();

	return true;
}

bool is_transaction_in_pool(int block_numbers, string l)
{
	return true;
}

bool verify_pre_block(consensus_part *cp)
{

	int pos = 0;
	int prevpos = 0;
	int tot_transaction = 0;
	int block_number = (cp->round - 1) * CONCURRENCY_BLOCKS + cp->order_in_round;

	while ((pos = cp->list.find("\n", pos + 1)) >= 0)
	{
		string l = cp->list.substr(prevpos, pos - prevpos);
		if (is_transaction_in_pool(block_number, l))
		{
			tot_transaction++;
		}
		else
			return false;
		prevpos = pos + 1;
	}
	if (tot_transaction == (cp->tx_list.second - cp->tx_list.first))
	{
		return true;
	}
	else
	{
		printf("\n\033[33;1m[!] Transaction numbers %d differ with %d\n\033[0m\n", tot_transaction, cp->tx_list.second - cp->tx_list.first);
		fflush(stdout);
		return false;
	}
}

bool verify_validate_block(network_block *nb)
{

	string folder_transaction_pool = string(FOLDER_TRANSACTION_POOL);
	int block_number = (nb->consensusPart.round - 1) * CONCURRENCY_BLOCKS + nb->consensusPart.order_in_round;
	string block_filename = folder_transaction_pool + "/_" + my_ip + "_" + to_string(my_port) + "/" + "_body/" + "block_" + to_string(block_number);

	int no_txs = nb->consensusPart.tx_list.second - nb->consensusPart.tx_list.first;
	vector<string> leaves;
	ifstream file_of_transaction_pool;
	file_of_transaction_pool.open(block_filename.c_str());

	if (!file_of_transaction_pool.is_open())
	{
		printf("\n\033[33;1m[!] Open Transaction pool file FALSE with path %s\n\033[0m\n", block_filename.c_str());
		fflush(stdout);
		exit(1);
	}

	string tx_demo = create_one_transaction();
	int tx_size = tx_demo.size();

	for (int i = 0; i < no_txs; i++)
	{
		char buffer[tx_size + 100];
		file_of_transaction_pool.getline(buffer, tx_size + 100, '\n');
		leaves.push_back(buffer);
	}
	file_of_transaction_pool.close();
	uint32_t tot_size_add = (int)pow(2, ceil(log(leaves.size()) / log(2))) - leaves.size();
	for (int i = 0; i < tot_size_add; i++)
		leaves.push_back(EMPTY_LEAF);
	string merkel_root_txs = compute_merkle_tree_root(leaves);

	if (nb->consensusPart.merkel_root_of_txs == merkel_root_txs)
	{
		return true;
	}
	else
	{
		printf("\n\033[33;1m[!] Merkel root of txs %s differ with %s\n\033[0m\n", nb->consensusPart.merkel_root_of_txs.c_str(), merkel_root_txs.c_str());
		fflush(stdout);
		return false;
	}
}

int create_transaction_block(BlockHash hash, string filename)
{

	uint32_t l = 0, no_txs = 0;

	if (WRITE_BLOCKS_TO_HDD)
	{
		ofstream file;
		try
		{
			file.open(filename);
		}
		catch (const std::string &ex)
		{
			return false;
		}
		while (l < BLOCK_SIZE_IN_BYTES)
		{
			string tx = create_one_transaction();
			file << tx << endl;
			l += tx.size();
			no_txs++;
		}
		file.close();
	}
	else
	{
		while (l < BLOCK_SIZE_IN_BYTES)
		{
			string tx = create_one_transaction();
			l += tx.size();
			no_txs++;
		}
	}

	return no_txs;
}

bool verify_transaction(string tx)
{

	vector<string> s = split(tx, ":");
	if (s.size() == 4)
	{
		string ad1 = s[0];
		string ad2 = s[1];
		string amount = s[2];
		string signature = s[3];

		if (ad1.size() != 8 * ADDRESS_SIZE_IN_DWORDS || ad2.size() != 8 * ADDRESS_SIZE_IN_DWORDS || amount.size() <= 0)
			return false;

		string full = ad1 + ":" + ad2 + ":" + amount;

		return verify_message(full, signature);
	}
	else
	{
		if (PRINT_TRANSMISSION_ERRORS)
		{
			cout << "Incorrect transaction size:" << s.size() << endl;
			cout << "tx:" << tx << endl;
		}
		return false;
	}
}