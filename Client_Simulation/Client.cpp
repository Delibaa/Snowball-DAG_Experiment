#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Transaction.h"
#include "Crypto.h"
using namespace boost::asio;
using namespace std;
io_service service;

struct talk_to_svr
{
	talk_to_svr(int clientId, int tx_n)
		: sock_(service), started_(true), clientId(clientId), tx_number(tx_n) {}

	void connect(boost::asio::ip::tcp::endpoint endpoint)
	{
		sock_.connect(endpoint);
	}
	void loop()
	{
		int i = 0;
		while (started_ && i < tx_number)
		{
			write_tx();
			std::cout << "Client " << clientId << "with message" << i << ": Sent to server " << endl;
			i++;
			boost::this_thread::sleep(boost::posix_time::millisec(1));
			// change to async read , or will be blocked
			// read_answer();
		}
		boost::this_thread::sleep(boost::posix_time::millisec(10000));
		// sock_.close();
	}

private:
	void write_tx()
	{
		string tx = create_one_transaction();
		unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
		string message = "#transaction,Client," + to_string(clientId) + "," + tx + "," + to_string(time_of_now);
		// attention to request's format
		string s = message + "!";
		write(s);
		cout << message << endl;
	}
	void write(const std::string &msg)
	{
		sock_.write_some(buffer(msg));
	}
	void read_answer()
	{
		already_read_ = 0;
		read(sock_, buffer(buff_),
			 boost::bind(&talk_to_svr::read_complete, this, _1, _2));
		process_msg();
	}
	void process_msg()
	{
		std::string msg(buff_, already_read_);
	}

	size_t read_complete(const boost::system::error_code &err, size_t bytes)
	{
		if (err)
			return 0;
		already_read_ = bytes;
		bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
		return found ? 0 : 1;
	}

private:
	ip::tcp::socket sock_;
	enum
	{
		max_msg = 1024
	};
	int already_read_;
	char buff_[max_msg];
	bool started_;
	int clientId;
	int tx_number;
};

void run_client(int clientId, boost::asio::ip::tcp::endpoint endpoint, int tx_numbers)
{
	talk_to_svr client(clientId, tx_numbers);
	try
	{
		client.connect(endpoint);
		client.loop();
	}
	catch (boost::system::system_error &err)
	{
		// terminate
		std::cout << "client terminated :" << err.what() << std::endl;
	}
}

int main()
{
	// test ssl
	prepare_ecc_crypto(string(FILE_ECC_KEY));
	string some_test_string = "123";
	if (!verify_message(some_test_string, sign_message(some_test_string)))
	{
		cout << "Something is wrong with ECC" << endl
			 << "Exiting..." << endl;
		exit(3);
	}

	// Setting txs speed = tx_amount_in_an_epoch/epoch time
	unsigned long tx_amount_in_an_epoch = 30000;

	// connect to full nodes, randomly choose one full nodes
	vector<pair<string, int>> full_nodes_address;
	ifstream file(string(_FOLDER_PEERS));
	string l;
	while (getline(file, l))
	{
		int p = l.find(":");
		if (p > 0)
		{
			full_nodes_address.push_back(make_pair(l.substr(0, p), atoi(l.substr(p + 1, l.length()).c_str())));
		}
	}

	//random choose one full node
	boost::thread_group threads;
	int client_number = 5;
	vector<int> peer;
	for (int i = 0; i < client_number; i++)
	{
		int p = rand() % (full_nodes_address.size());
		peer.push_back(p);
	}

	int tx_number = tx_amount_in_an_epoch / client_number;
	cout << "The transaction sending amount is " << tx_amount_in_an_epoch << endl;

	for (int i = 0; i < client_number; i++)
	{
		boost::asio::ip::tcp::endpoint ep(ip::address::from_string(full_nodes_address[peer[i]].first), full_nodes_address[peer[i]].second);
		threads.create_thread(boost::bind(run_client, i, ep, tx_number));
	}
	threads.join_all();

	return 0;
}