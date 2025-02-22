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

#include "requests.h"
#include "misc.h"

extern string my_ip;
extern uint32_t my_port;

string create__ask_block(uint32_t chain_id, BlockHash hash, uint32_t my_depth, uint32_t hash_depth)
{
  string s = "#ask_block," + my_ip + "," + to_string(my_port) + "," + to_string(chain_id) + "," + to_string(hash);
  s += "," + to_string((my_depth >= hash_depth) ? 1 : (hash_depth - my_depth));
  return s;
}

bool parse__ask_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, uint32_t &chain_id, BlockHash &hash, uint32_t &max_number_of_blocks)
{
  if (sp.size() < 6)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  chain_id = safe_stoi(sp[3], pr);
  hash = safe_stoull(sp[4], pr);
  max_number_of_blocks = safe_stoi(sp[5], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && chain_id < CHAINS))
  {
    cout << "Could not get proper values of ask_block" << endl;
    cout << pr << " " << sender_ip << " " << chain_id << endl;
    return false;
  }

  return true;
}

string create__process_block(network_block *nb)
{
  string s = "#process_block," + my_ip + "," + to_string(my_port) + "," + to_string(nb->chain_id) + "," + to_string(nb->parent) + "," + to_string(nb->hash) + ",";
  s += to_string(nb->no_txs) + ",";
  s += to_string(nb->depth) + ",";
  s += to_string(nb->rank) + ",";
  s += to_string(nb->next_rank) + ",";
  s += to_string(nb->consensusPart.round) + ",";
  s += to_string(nb->consensusPart.order_in_round) + ",";
  s += to_string(nb->consensusPart.tx_list.first) + ",";
  s += to_string(nb->consensusPart.tx_list.second) + ",";
  s += nb->consensusPart.merkel_root_of_txs;

  return s;
}

bool parse__process_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, network_block &nb)
{
  int MPL = merkle_proof_length();

  if (sp.size() < 15)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4] + sp[5], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  nb.chain_id = safe_stoi(sp[3], pr);
  nb.parent = safe_stoull(sp[4], pr);
  nb.hash = safe_stoull(sp[5], pr);
  nb.no_txs = safe_stoi(sp[6], pr);
  nb.depth = safe_stoi(sp[7], pr);
  nb.rank = safe_stoi(sp[8], pr);
  nb.next_rank = safe_stoi(sp[9], pr);
  nb.consensusPart.round = safe_stoi(sp[10], pr);
  nb.consensusPart.order_in_round = safe_stoi(sp[11], pr);
  nb.consensusPart.tx_list.first = safe_stoi(sp[12], pr);
  nb.consensusPart.tx_list.second = safe_stoi(sp[13], pr);
  nb.consensusPart.merkel_root_of_txs = sp[14];

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && nb.chain_id < CHAINS))
  {
    cout << "Could not get proper values of process_block" << endl;
    cout << pr << " " << sender_ip << " " << nb.chain_id << endl;

    for (int i = 1; i <= 9; i++)
      cout << sp[i] << endl;

    return false;
  }
  return true;
}

string create__got_full_block(uint32_t chain_id, BlockHash hash)
{
  string s = "#got_full_block," + my_ip + "," + to_string(my_port) + "," + to_string(chain_id) + "," + to_string(hash);
  return s;
}

bool parse__got_full_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, uint32_t &chain_id, BlockHash &hash)
{

  if (sp.size() < 5)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  chain_id = safe_stoi(sp[3], pr);
  hash = safe_stoull(sp[4], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && chain_id < CHAINS))
  {
    cout << "Could not get proper values of got_full_block" << endl;
    cout << pr << " " << sender_ip << " " << chain_id << endl;
    return false;
  }

  return true;
}

string create__have_full_block(uint32_t chain_id, BlockHash hash)
{
  string s = "#have_full_block," + my_ip + "," + to_string(my_port) + "," + to_string(chain_id) + "," + to_string(hash);
  return s;
}
bool parse__have_full_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, uint32_t &chain_id, BlockHash &hash)
{

  if (sp.size() < 5)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  chain_id = safe_stoi(sp[3], pr);
  hash = safe_stoull(sp[4], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && chain_id < CHAINS))
  {
    cout << "Could not get proper values of have_full_block" << endl;
    cout << pr << " " << sender_ip << " " << chain_id << endl;
    return false;
  }

  return true;
}

string create__ask_full_block(uint32_t chain_id, BlockHash hash)
{
  string s = "#ask_full_block," + my_ip + "," + to_string(my_port) + "," + to_string(chain_id) + "," + to_string(hash);
  return s;
}
bool parse__ask_full_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, uint32_t &chain_id, BlockHash &hash)
{

  if (sp.size() < 5)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  chain_id = safe_stoi(sp[3], pr);
  hash = safe_stoull(sp[4], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && chain_id < CHAINS))
  {
    cout << "Could not get proper values of ask_full_block" << endl;
    cout << pr << " " << sender_ip << " " << chain_id << endl;
    return false;
  }

  return true;
}

string create__full_block(uint32_t chain_id, BlockHash hash, tcp_server *ser, Blockchain *bc)
{
  string s = "#full_block," + my_ip + "," + to_string(my_port) + "," + to_string(chain_id) + "," + to_string(hash) + ",";

  block *b = bc->find_block_by_hash_and_chain_id(hash, chain_id);
  s += to_string(b->nb->consensusPart.verify_1_numbers);

  // Add everything removed from process_block
  network_block *nb = b->nb;
  if (NULL != nb)
  {
    s += ",";
    s += to_string(nb->trailing) + "," + to_string(nb->trailing_id) + "," + nb->merkle_root_chains + "," + nb->merkle_root_txs + ",";
    for (int i = 0; i < nb->proof_new_chain.size(); i++)
      s += nb->proof_new_chain[i] + ",";

    s += to_string(nb->time_mined);
    unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    s += "," + to_string(time_of_now);
  }
  else
  {

    printf("Network block cannot be found in create__full_block");
    exit(2);
  }

  return s;
}

bool parse__full_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, uint32_t &chain_id, BlockHash &hash, string &txs, network_block &nb, unsigned long &sent_time)
{
  int MPL = merkle_proof_length();

  if (sp.size() < 10 + 1 * MPL + 1)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  chain_id = safe_stoi(sp[3], pr);
  hash = safe_stoull(sp[4], pr);
  nb.consensusPart.verify_1_numbers = safe_stoi(sp[5], pr);
  // txs = sp[6];

  nb.trailing = safe_stoull(sp[6], pr);
  nb.trailing_id = safe_stoi(sp[7], pr);
  nb.merkle_root_chains = sp[8];
  nb.merkle_root_txs = sp[9];
  for (int j = 0; j < MPL; j++)
    nb.proof_new_chain.push_back(sp[10 + j]);
  nb.time_mined = safe_stoull(sp[10 + 1 * MPL], pr);
  sent_time = safe_stoull(sp[10 + 1 * MPL + 1], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0 && chain_id < CHAINS))
  {
    cout << "Could not get proper values of full_block:" << sp.size() << endl;
    cout << pr << " " << sender_ip << " " << chain_id << endl;
    for (int i = 0; i < sp.size(); i++)
      cout << sp[i] << endl;
    return false;
  }

  return true;
}

string create__ping(string tt, uint32_t dnext, unsigned long tsec, int mode)
{
  string s = "#ping," + my_ip + "," + to_string(my_port) + "," + tt + "," + to_string(dnext) + "," + to_string(tsec) + "," + to_string(mode);
  return s;
}

bool parse__ping(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, string &tt, uint32_t &dnext, unsigned long &tsec, int &mode)
{
  if (sp.size() < 7)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  tt = sp[3];
  dnext = safe_stoi(sp[4], pr);
  tsec = safe_stoull(sp[5], pr);
  mode = safe_stoi(sp[6], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr))
  {
    cout << "Could not get proper values of ping" << endl;
    cout << pr << endl;
    return false;
  }

  return true;
}

string create__mining_succeed(bool Certificate, string random, int round)
{
  string s = "#mining_succeed," + my_ip + "," + to_string(my_port) + "," + to_string(Certificate) + "," + random + "," + to_string(round);
  return s;
}

bool parse__mining_succeed(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, bool &Certificate, int &round)
{
  if (sp.size() < 6)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  Certificate = safe_stoi(sp[3], pr);
  round = safe_stoi(sp[5], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values of ming_succeed_certificate" << endl;
    cout << pr << " " << sender_ip << " " << endl;
    return false;
  }

  return true;
}

string create__consensus_block(consensus_part *cp)
{
  string s = "#consensus_block," + my_ip + "," + to_string(my_port) + "," + to_string(cp->round) + "," + to_string(cp->order_in_round) + "," + to_string(cp->tx_list.first) + "," + to_string(cp->tx_list.second) + "," + cp->list;

  return s;
}

bool parse__consensus_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, consensus_part &cp)
{
  if (sp.size() < 8)
    return false;

  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4] + sp[5] + sp[6], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  cp.round = safe_stoi(sp[3], pr);
  cp.order_in_round = safe_stoi(sp[4], pr);
  cp.tx_list.first = safe_stoi(sp[5], pr);
  cp.tx_list.second = safe_stoi(sp[6], pr);
  cp.list = sp[7];

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values of consensus block" << endl;
    cout << pr << " " << sender_ip << " " << endl;
    return false;
  }

  return true;
}

string create__have_consensus_block(int order_in_round, bool received)
{

  string s = "#have_consensus_block," + my_ip + "," + to_string(my_port) + "," + to_string(order_in_round) + "," + to_string(received);

  return s;
}

bool parse__have_consensus_block(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, int &pre_blocks, bool &received)
{

  if (sp.size() < 5)
    return false;

  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  pre_blocks = safe_stoi(sp[3], pr);
  received = safe_stoi(sp[4], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values of consensus block info" << endl;
    cout << pr << " " << sender_ip << " " << endl;
    return false;
  }

  return true;
}

string create__verified_1_info(network_block *nb, string random)
{
  string s = "#verified_1," + my_ip + "," + to_string(my_port) + "," + to_string(nb->chain_id) + "," + to_string(nb->parent) + "," + to_string(nb->hash) + ",";
  s += to_string(nb->no_txs) + ",";
  s += to_string(nb->depth) + ",";
  s += to_string(nb->rank) + ",";
  s += to_string(nb->next_rank) + ",";
  s += to_string(nb->consensusPart.round) + ",";
  s += to_string(nb->consensusPart.order_in_round) + ",";
  s += to_string(nb->consensusPart.tx_list.first) + ",";
  s += to_string(nb->consensusPart.tx_list.second) + ",";
  s += nb->consensusPart.merkel_root_of_txs + ",";
  s += random;
  return s;
}

bool parse__verified_1_info(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, network_block &nb)
{
  if (sp.size() < 16)
    return false;
  // 需要修改
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3] + sp[4] + sp[5], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  nb.chain_id = safe_stoi(sp[3], pr);
  nb.parent = safe_stoull(sp[4], pr);
  nb.hash = safe_stoull(sp[5], pr);
  nb.no_txs = safe_stoi(sp[6], pr);
  nb.depth = safe_stoi(sp[7], pr);
  nb.rank = safe_stoi(sp[8], pr);
  nb.next_rank = safe_stoi(sp[9], pr);
  nb.consensusPart.round = safe_stoi(sp[10], pr);
  nb.consensusPart.order_in_round = safe_stoi(sp[11], pr);
  nb.consensusPart.tx_list.first = safe_stoi(sp[12], pr);
  nb.consensusPart.tx_list.second = safe_stoi(sp[13], pr);
  nb.consensusPart.merkel_root_of_txs = sp[14];

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values before consensus phase 1" << endl;
    cout << pr << " " << sender_ip << " " << endl;
    return false;
  }

  return true;
}

string create__answer_verified_1_info(BlockHash hash, int votes)
{
  string s = "#answer_verified_1," + my_ip + "," + to_string(my_port) + "," + to_string(hash) + "," + to_string(votes);
  return s;
}

bool parse__answer_verified_1_info(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, BlockHash &hash, int &votes)
{
  if (sp.size() < 5)
    return false;
  // 需要修改
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  hash = safe_stoull(sp[3], pr);
  votes = safe_stoi(sp[4], pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values during consensus phase 1" << endl;
    cout << pr << " " << sender_ip << " " << endl;
    return false;
  }

  return true;
}

string create__transaction(string tx, unsigned long time)
{
  string s = "#transaction," + my_ip + "," + to_string(my_port) + "," + tx + "," + to_string(time);
  return s;
}

bool parse__transaction(vector<std::string> sp, map<string, int> &passed, string &sender_ip, uint32_t &sender_port, string &tx, unsigned long &time)
{

  if (sp.size() < 4)
    return false;
  if (key_present(sp[0] + sp[1] + sp[2] + sp[3], passed))
    return false;

  bool pr = true;
  sender_ip = sp[1];
  sender_port = safe_stoi(sp[2], pr);
  tx = sp[3];
  time = safe_stoull(sp[4],pr);

  if (PRINT_TRANSMISSION_ERRORS && !(pr && sender_ip.size() > 0))
  {
    cout << "Could not get proper values of transaction" << endl;
    cout << pr << " " << sender_ip << ":" << sender_port << " " << endl;
    return false;
  }

  return true;
}

bool key_present(string key, map<string, int> &passed)
{
  if (passed.find(key) != passed.end())
    return true;

  passed.insert(make_pair(key, 1));
  return false;
}
