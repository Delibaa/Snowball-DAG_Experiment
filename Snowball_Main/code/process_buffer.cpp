#include <boost/thread/mutex.hpp>
#include "process_buffer.h"


extern mt19937 rng;
extern boost::thread *mythread;
extern unsigned long time_of_start;
extern string my_ip;
extern uint32_t my_port;



void process_buffer( string &m, tcp_server *ser, Blockchain *bc, Consensus_Group *cg )
{


    size_t pos_h = m.find("#");
    if(  pos_h != 0 || pos_h == string::npos){

      if (   PRINT_TRANSMISSION_ERRORS  ){
        cout <<"something is wrong with the provided message pos: "<<pos_h;
        cout <<" m:"<<m.size()<<":"<<m<<":";
        cout << endl;

        exit(0);
      }

      m = "";

      return;
    }


    map<string,int> passed;


    vector <size_t> positions;
    size_t pos = m.find("#");
    while( pos != string::npos ){
      positions.push_back( pos );
      pos = m.find( "#", pos + 1);//应该是套接字中还有其他的字符流，所以用这个标记隔开
    }
    positions.push_back( m.size() + 1 );



    int p;
    //是捏，需要解析很多消息
    for( p=0; p < positions.size() -1 ; p++){

      string w = m.substr(positions[p],positions[p+1]-positions[p]);
      if (w[w.size() -1] != '!' )
        break;

      w = w.substr(0,w.size() -1 );


      vector<std::string> sp = split( w , ",");
      if( sp.size() < 1 ) continue;


      if( sp[0] == "#ping"){

          string sender_ip;
          uint32_t sender_port;
          string tt;
          uint32_t dnext; 
          unsigned long tsec;
          int mode;
          if( ! parse__ping( sp, passed, sender_ip,  sender_port, tt, dnext, tsec, mode ) )
          {
            if ( p+2 == positions.size() ) break;
            continue;
          }

          // Add pinger to list of peers
          ser->add_indirect_peer_if_doesnt_exist(sender_ip+":"+to_string(sender_port));


          // If mode=0, it means we measure latency. Thus once a pingID has been seen, we don't update
          // if mode=1, it means we measure diameter. Thus we update hash pings if dnext is smaller than previously seen

          // If ping seen before, then do nothing
          if ( ! (ser->add_ping( tt, dnext, mode==1 ) ) ) continue;

          // Add the file of pings
          unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
          string filename =  string(FOLDER_PINGS)+"/"+my_ip+to_string(my_port);
          ofstream file;
          file.open(filename, std::ios_base::app);
          file << mode << " " << tt << " " << (dnext + 1) << " " << ((time_of_now > tsec) ? ( time_of_now - tsec ) : 0) << endl;
          file.close();

          // Send ping to other peers
          string s = create__ping( tt, dnext+1, tsec, mode );
          ser->write_to_all_peers( s );

      }
      else if( sp[0] == "#ask_block"){

          string sender_ip;
          uint32_t sender_port;
          uint32_t chain_id;
          BlockHash hash;
          uint32_t max_number_of_blocks;
          if( ! parse__ask_block( sp, passed,     sender_ip, sender_port, chain_id, hash, max_number_of_blocks ) )
          {
            if ( p+2 == positions.size() ) break;
            continue;
          }

          // Add it to blind peers
          ser->add_indirect_peer_if_doesnt_exist(sender_ip+":"+to_string(sender_port));

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d ASKS  for chain:block %3d : %lx\n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
              fflush(stdout);
          }

          // First check if it is in the main chain
          block *b = bc->find_block_by_hash_and_chain_id( hash, chain_id );

          // If not, check in the incomplete chains
          if (NULL == b)
              b = bc->find_incomplete_block_by_hash_and_chain_id( hash, chain_id );

          // send several (max_number_of_blocks) blocks at once
          for( int i=0; i<max_number_of_blocks && i < MAX_ASK_BLOCKS;i++){
              if (NULL != b && b->parent != NULL ){
                  ser-> send_block_to_one_peer( sender_ip, sender_port, chain_id, b->parent->hash, b->hash, b);
                  b = b->parent;
              }
              else
                  break;
          }


      }
      else if( sp[0] == "#process_block"){

          string sender_ip;
          uint32_t sender_port;
          network_block nb;
          if( ! parse__process_block( sp, passed,  sender_ip, sender_port,  nb ) ) 
          {
            if ( p+2 == positions.size() ) break;
            continue;
          }


          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d SENDS new chain:block %3d : (%lx %lx) \n\033[0m", sender_ip.c_str(), sender_port, nb.chain_id, nb.parent, nb.hash);
              fflush(stdout);
          }


          //注意这里的设置是0
          if ( CAN_INTERRUPT ){
              // If the new block is extension on the block it is mining then interrupt mining
              // /block *t = bc->find_deepest_child_by_chain_id( nb.chain_id );
              //if (NULL != t && t->hash == parent )
                  mythread->interrupt();
          }



          // Add the block to the blockchain
          bool added;
          bool need_parent = bc->add_received_block( nb.chain_id, nb.parent, nb.hash, nb, added );
          if (added )
              ser->send_block_to_peers( &nb );

          // If needed parent then ask peers
          uint32_t chain_depth = bc->get_deepest_child_by_chain_id(nb.chain_id)->nb->depth;

          // Ask parent block from peers
          if ( need_parent ){

              if( PRINT_SENDING_MESSAGES ){
                printf ("\033[34;1mAsking %d: %lx from all peers\033[0m\n", nb.chain_id, nb.parent );
                fflush(stdout);
              }

              string s = create__ask_block( nb.chain_id, nb.parent, chain_depth, nb.depth );
              ser->write_to_all_peers( s );
            }

          // Add it to blind peers
          ser->add_indirect_peer_if_doesnt_exist(sender_ip+":"+to_string(sender_port) );


      }
      else if( sp[0] == "#got_full_block"){

          string sender_ip;
          uint32_t sender_port;
          uint32_t chain_id;
          BlockHash hash;
          uint32_t max_number_of_blocks;

          if( ! parse__got_full_block( sp, passed, sender_ip, sender_port, chain_id, hash ) )
          {
            if ( p+2 == positions.size() ) break;
            continue;
          } 

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d BEGS for got full block chain:block %3d : %lx  \n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
              fflush(stdout);
          }


          // Check if this node has the full block, and if so send to the asking peer
          if( bc->have_full_block(chain_id, hash) ){
            string s = create__have_full_block( chain_id, hash );
            ser->write_to_one_peer(sender_ip, sender_port, s );

            if( PRINT_SENDING_MESSAGES ){
              printf ("\033[34;1m1mIhaveFullBlock %d: %lx to peer %s:%d\033[0m\n", chain_id, hash, sender_ip.c_str(), sender_port );
              fflush(stdout);
            }


          }


      }
      
      else if( sp[0] == "#have_full_block"){

          string sender_ip;
          uint32_t sender_port;
          uint32_t chain_id;
          BlockHash hash;
          uint32_t max_number_of_blocks;

          if( ! parse__have_full_block( sp, passed,     sender_ip, sender_port, chain_id, hash ) )
          {
            if ( p+2 == positions.size() ) break;
            continue;
          }

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d HAVE full block chain:block %3d : %lx  \n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
              fflush(stdout);
          }


          // Make sure you still DON't have the full block
          if ( bc->have_full_block( chain_id, hash) ) continue;


          // Check that the reply from the peer node was the FIRST such reply for the asking block, and if so ask the peer node for the full block
          unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
          if( bc->still_waiting_for_full_block( hash, time_of_now )){
              string s = create__ask_full_block( chain_id,  hash);
              ser->write_to_one_peer(sender_ip, sender_port, s );

              if( PRINT_SENDING_MESSAGES ){
                printf ("\033[34;1mASKFULLBLOCK %d: %lx to peer %s:%d\033[0m\n", chain_id, hash, sender_ip.c_str(), sender_port );
                fflush(stdout);
              }

          }

      }

      else if( sp[0] == "#ask_full_block"){

          string sender_ip;
          uint32_t sender_port;
          uint32_t chain_id;
          BlockHash hash;
          uint32_t max_number_of_blocks;
          if( ! parse__ask_full_block( sp, passed,     sender_ip, sender_port, chain_id, hash ) )
          {
            if ( p+2 == positions.size() ) break;
            continue;
          }  

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d WANTS full block chain:block %3d : %lx  \n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
              fflush(stdout);
          }


          // Make sure you have the block 
          if ( ! bc->have_full_block( chain_id, hash) ) continue;
          // Get the full block (data) and send it to the asking peer
          string s = create__full_block(chain_id, hash, ser, bc); 
          ser->write_to_one_peer(sender_ip, sender_port, s );

          if( PRINT_SENDING_MESSAGES ){
            printf ("\033[34;1mSENDING FULLBLOCK!! %d: %lx to peer %s:%d\033[0m\n", chain_id, hash, sender_ip.c_str(), sender_port );
            fflush(stdout);
          }


      }

      else if( sp[0] == "#full_block" ){

          string sender_ip;
          uint32_t sender_port;
          uint32_t chain_id;
          BlockHash hash;
          string txs = "fake ones";
          network_block nb;
          unsigned long sent_time;

          if( ! parse__full_block( sp, passed, sender_ip, sender_port, chain_id, hash, txs, nb, sent_time ) )
          {
              if ( p+2 == positions.size() ) break;
              continue;
          }

          // Make sure the block does not exist
          if( bc->have_full_block( chain_id, hash) ){
              continue;
          }

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d NICE FULL BLOCKS chain:block %3d : %lx  \n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
              fflush(stdout);
          }


          if ( txs.size() >= 0  ){

              if ( PRINT_RECEIVING_MESSAGES ){
                  printf("\033[32;1m%s:%d PROVIDES full chain:block %3d : %lx\n\033[0m", sender_ip.c_str(), sender_port, chain_id, hash);
                  fflush(stdout);
              }

              block * b = bc->find_block_by_hash_and_chain_id( hash, chain_id );
              if (NULL == b || NULL == b->nb ){
                  cout << "Cannot find block with such hash and chain_id " << hash<<" "<<chain_id<<endl;
                  cout << "Or network block does not exist: "<< endl;
                  fflush(stdout);
                  continue;
              }
              bool all_good = false;
              if(nb.consensusPart.verify_1_numbers >= (CONCURRENCY_BLOCKS /3)*2){
                  all_good = true;
              }

//              int prevpos = 0;
//              int pos = 0;
//              int tot_transactions = 0;
//              bool all_good = true;
//              while ( all_good &&  ((pos  = txs.find("\n",pos+1)) >= 0) ){
//
//                  string l = txs.substr(prevpos, pos-prevpos);
//                  if( fake_transactions ||  verify_transaction( l ) ){
//                      tot_transactions ++;
//                  }
//                  else
//                      all_good = false;
//
//                  prevpos = pos+1;
//              }
//
//              if( tot_transactions != b->nb->no_txs ){
//                  if ( tot_transactions * 1.08 >= b->nb->no_txs )
//                  {
//                      if ( PRINT_TRANSMISSION_ERRORS ){
//                          cout << "The number of TXS differ from the one provided earlier: "<<tot_transactions<<" "<<b->nb->no_txs<<endl;
//                          cout << sender_port <<" " << chain_id << " "<<hash << endl;
//                          cout << txs << ":"<<endl;
//
//                      }
//                  }
//                  if (tot_transactions > 0 && p+2 == positions.size() )
//                      break;
//                  continue;
//              }

              if( all_good){

                  // Adding all from nb
                  network_block *n = b->nb;
                  n->trailing    = nb.trailing;
                  n->trailing_id = nb.trailing_id;
                  n->merkle_root_chains = nb.merkle_root_chains;
                  n->consensusPart.verify_1_numbers = nb.consensusPart.verify_1_numbers;
                  n->consensusPart.verify_2_numbers = nb.consensusPart.verify_1_numbers;
                  n->merkle_root_txs = nb.merkle_root_txs;
                  n->proof_new_chain =  nb.proof_new_chain;
                  n->time_mined = nb.time_mined;
                  for( int j=0; j<NO_T_DISCARDS; j++){
                      n->time_commited[j] = 0;
                      n->time_partial[j] = 0;
                  }

                  string h = sha256( n->merkle_root_chains + n->merkle_root_txs );
                  uint32_t chain_id_from_hash = get_chain_id_from_hash( h );

                  // Verify the chain ID is correct
                  if ( chain_id_from_hash != n->chain_id ){
                      if ( PRINT_TRANSMISSION_ERRORS ) printf("\033[31;1mChain_id incorrect for the new block \033[0m\n");
                      continue;
                  }

                  // Verify blockhash is correct
                  if ( string_to_blockhash( h ) != n->hash ){
                      if ( PRINT_TRANSMISSION_ERRORS ) printf("\033[31;1mBlockhash is incorrect\033[0m\n");
                      continue;
                  }

                  // Verify the new block chain Merkle proof
                  if (! verify_merkle_proof( n->proof_new_chain, n->parent, n->merkle_root_chains, chain_id_from_hash )){
                      if ( PRINT_TRANSMISSION_ERRORS ) printf("\033[31;1mFailed to verify new block chain Merkle proof \033[0m\n");
                      continue;
                  }

                  // Verify trailing
                  // If it cannot find the trailing block then ask for it
                  if( NULL == bc->find_block_by_hash_and_chain_id( n->trailing, n->trailing_id ) ){
                      string s_trailing = create__ask_block( n->trailing_id, n->trailing, 0, 0  );
                      ser->write_to_all_peers( s_trailing );
                      continue;
                  }

                  string tx = create_one_transaction();
                  uint32_t tx_size = tx.size();
                  // Increase amount of received bytes (and include message bytes )
                  ser->add_bytes_received( 0, tx_size * n->no_txs );

                  if ( PRINT_VERIFYING_TXS ){
                      printf("\033[32;1mAll %4d txs are verified \n\033[0m", n->no_txs);
                      fflush(stdout);
                  }

//                  // Store into the file
//                  if ( WRITE_BLOCKS_TO_HDD ){
//                      string filename = ser->get_server_folder()+"/"+blockhash_to_string( hash );
//                      ofstream file;
//                      try{ file.open(filename); }
//                      catch(const std::string& ex){  continue; }
//                      file << txs;
//                      file.close();
//                  }

                  // Remove from hash table
                  unsigned long time_of_now = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
                  string required_time_to_send=to_string( (time_of_now > sent_time) ? (time_of_now - sent_time) : 0    );
                  bc->set_block_full( chain_id, hash, sender_ip+":"+to_string(sender_port)+" "+required_time_to_send );
                  ser->additional_verified_transaction(nb.no_txs);




              }
              else{
                  if ( PRINT_VERIFYING_TXS ){
                      printf("\033[31;1mSome txs cannot be verified \n\033[0m");
                      fflush(stdout);
                  }
              }


          }
      }

      else if( sp[0] == "#mining_succeed"){

          string sender_ip;
          uint32_t sender_port;
          bool certificate;

          if( ! parse__mining_succeed(sp, passed, sender_ip, sender_port, certificate))
          {
              if ( p+2 == positions.size()) break;
              continue;
          }

        //   if ( CAN_INTERRUPT && certificate == true){
        //       // If the new block is extension on the block it is mining then interrupt mining
        //       // /block *t = bc->find_deepest_child_by_chain_id( nb.chain_id );
        //       //if (NULL != t && t->hash == parent )
        //       mythread->interrupt();
        //   }

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d SENDS message of mining succeed! \n\033[0m", sender_ip.c_str(), sender_port);
              fflush(stdout);
          }

        //   std::unique_lock<std::mutex> l(cg->lock);
        //   cg->can_write.wait( l, [cg](){return !cg->locker_write;});
        //   cg->locker_write = true;
           
           
           if(cg->is_consensus_started()){

            if(PRINT_CONSENSUS_MESSAGE){
                printf("\033[33;1m[ ] Consensus round %d has been started! Join from internet error! \n\033[0m", cg->round);
            }

            continue;

          }

           std::unique_lock<std::mutex> l(cg->lock);
           cg->can_write.wait( l, [cg](){return !cg->locker_write;});
           cg->locker_write = true;

           cg->join_Consensus_group(sender_ip,sender_port,certificate);
           
           if(cg->get_concurrency_block_numbers() >= CONCURRENCY_BLOCKS){
            
               cg->start_consensus_of_blocks();
               pair<string, uint32_t> leader_info = cg->choose_leader();
               string leader_ip = leader_info.first;
               uint32_t leader_port = leader_info.second;

               if(PRINT_CONSENSUS_MESSAGE){
                    printf("\033[33;1m[ ] The leader of consensus round %d is %s:%d. \n\033[0m\n", cg->round, leader_ip.c_str(), leader_port);
                }

                if (leader_port == my_port && leader_ip == my_ip){
                //生成偏序并发块并进行传播，原型中先使用交易相同的交易池

                    mine_Consensus_blocks(bc, cg);

                }
            }

            cg->locker_write = false;
            l.unlock();
            cg->can_write.notify_one();


          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d HAVE mined block successfully and become a member of consensus group\n\033[0m\n", sender_ip.c_str(), sender_port);
              fflush(stdout);
          }

      }

      else if ( sp[0] == "#consensus_block"){

          string sender_ip;
          uint32_t sender_port;
          //需要修改的不要传指针
          consensus_part cp;

          if( !parse__consensus_block(sp, passed, sender_ip, sender_port, cp))
          {
              if ( p+2 == positions.size() ) break;
              continue;
          }

          //detect leader
           if( (sender_ip != "127.0.0.1") ||  (sender_port != 8001) )
           {
               if(PRINT_CONSENSUS_MESSAGE){
                    printf ("\033[32;1m%s:%d isn't the right leader\n\033[0m\n", sender_ip.c_str(), sender_port);
                    fflush(stdout);
                }
               continue;
           }

           if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m Have a consensus block from leader %s:%d\n\033[0m\n", sender_ip.c_str(), sender_port);
              fflush(stdout);
          }

           //自己挖区块,这个地方赋0吧
           cp.verify_1_numbers = 0;
           cp.verify_2_numbers = 0;
        //    for(int i =0; i<cg->miner_list.size();i++){
        //        if(cg->miner_list[i].ip == my_ip && cg->miner_list[i].port == my_port){
        //            cp.verify_1_numbers = cg->miner_list[i].share_of_block;
        //            cp.verify_2_numbers = cg->miner_list[i].share_of_block;
        //        }
        //    }

           bool mined = mine_new_block(bc,cp);

           if(!mined){
               if(PRINT_MINING_MESSAGES){
                  printf("\033[32;1m MINING FAILURE!\n\033[0m\n");
                  fflush(stdout);
               }
               
           }


      }

      else if ( sp[0] == "#verified_1"){

          string sender_ip;
          uint32_t sender_port;
          network_block nb;

          if( !parse__verified_1_info(sp, passed, sender_ip, sender_port, nb))
          {
              if ( p+2 == positions.size() ) break;
              continue;
          }

          //verify
          if(!cg->is_consensus_started() && nb.consensusPart.round == cg->round) continue;

          if(nb.consensusPart.round < cg->round){

            if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d ASKING to vote on the block\n\033[0m\n", sender_ip.c_str(), sender_port);
              fflush(stdout);
            }

            map<int, history_info>::iterator iter;
            iter = cg->history.find(nb.consensusPart.round);

            for(int i = 0; i < iter->second.miner.size();i++){
                if(iter->second.miner[i].ip == ser->get_ip() && iter->second.miner[i].port == ser->get_port()){

                    bool added;
                    bool need_parent = bc->add_received_block( nb.chain_id, nb.parent, nb.hash, nb, added );

                    //泛洪传播机制
                    if (added )
                        ser->send_block_to_peers( &nb );

                   // If needed parent then ask peers
                    uint32_t chain_depth = bc->get_deepest_child_by_chain_id(nb.chain_id)->nb->depth;

                   // Ask parent block from peers
                    if ( need_parent ){

                        if( PRINT_SENDING_MESSAGES ){
                            printf ("\033[34;1mAsking %d: %lx from all peers\033[0m\n", nb.chain_id, nb.parent );
                            fflush(stdout);
                        }

                        string s = create__ask_block( nb.chain_id, nb.parent, chain_depth, nb.depth );
                        ser->write_to_all_peers( s );
                    }

                    // Add it to blind peers
                    ser->add_indirect_peer_if_doesnt_exist(sender_ip+":"+to_string(sender_port) );

                    int votes = iter->second.miner[i].share_of_block;
                    string s = create__answer_verified_1_info(nb.hash, votes);
                    ser->write_to_one_peer(sender_ip, sender_port, s); 

                    if( PRINT_SENDING_MESSAGES ){
                        printf ("\033[34;1mSENDING VOTES IN PHASE VALIDATE!! to peer %s:%d, with votes %d\033[0m\n", sender_ip.c_str(), sender_port, votes);
                        fflush(stdout);
                    }

                   break;

                }
            }

            continue;
            
          }


          if(!cg->is_in_Consensus_Group(sender_ip, sender_port)){

              if(PRINT_CONSENSUS_MESSAGE){
                    printf ("\033[32;1m[]the peer %s:%d is not the member of consensus group\n\033[0m\n", sender_ip.c_str(),sender_port);
                    fflush(stdout);
                }
              continue;
          }

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d ASKING to vote on the block\n\033[0m\n", sender_ip.c_str(), sender_port);
              fflush(stdout);
          }

          //add verify logic

          //verify pass, adding waiting for result queue
          bool added;
          bool need_parent = bc->add_received_block( nb.chain_id, nb.parent, nb.hash, nb, added );

          //泛洪传播机制
          if (added )
              ser->send_block_to_peers( &nb );

          // If needed parent then ask peers
          uint32_t chain_depth = bc->get_deepest_child_by_chain_id(nb.chain_id)->nb->depth;

          // Ask parent block from peers
          if ( need_parent ){

              if( PRINT_SENDING_MESSAGES ){
                  printf ("\033[34;1mAsking %d: %lx from all peers\033[0m\n", nb.chain_id, nb.parent );
                  fflush(stdout);
              }

              string s = create__ask_block( nb.chain_id, nb.parent, chain_depth, nb.depth );
              ser->write_to_all_peers( s );
          }

          // Add it to blind peers
          ser->add_indirect_peer_if_doesnt_exist(sender_ip+":"+to_string(sender_port) );


          //when verify pass, the votes of mine
          int votes = cg->get_member_in_Consensus_Group(ser->get_ip(), ser->get_port())->share_of_block;
          string s = create__answer_verified_1_info(nb.hash, votes);
          ser->write_to_one_peer(sender_ip, sender_port, s);

          if( PRINT_SENDING_MESSAGES ){
              printf ("\033[34;1mSENDING VOTES IN PHASE VALIDATE!! to peer %s:%d, with votes %d\033[0m\n", sender_ip.c_str(), sender_port, votes);
              fflush(stdout);
          }

      }

      else if( sp[0] == "#answer_verified_1"){

          string sender_ip;
          uint32_t sender_port;
          BlockHash hash;
          int votes;
          if( !parse__answer_verified_1_info(sp, passed, sender_ip, sender_port, hash, votes) )
          {
              if ( p+2 == positions.size() ) break;
              continue;
          }

          if(!cg->is_consensus_started()) continue;

          map<BlockHash, network_block>::iterator iter;
          iter = bc->waiting_for_phase_1_block.find(hash);

          if ( PRINT_RECEIVING_MESSAGES ){
              printf("\033[32;1m%s:%d Had voted on the block with %d\n\033[0m\n", sender_ip.c_str(), sender_port, votes);
              fflush(stdout);
          }
          int my_votes = 0;
          if(iter->second.consensusPart.verify_1_numbers == 0){
            my_votes = cg->get_member_in_Consensus_Group(ser->get_ip(), ser->get_port())->share_of_block;
          }

          //have enough votes in PHASE VALIDATE
          if( iter!=bc->waiting_for_phase_1_block.end() ){
             iter->second.consensusPart.verify_1_numbers +=my_votes;
             iter->second.consensusPart.verify_1_numbers += votes;
              if(iter->second.consensusPart.verify_1_numbers >= (CONCURRENCY_BLOCKS / 3)*2){

                  if(PRINT_CONSENSUS_MESSAGE){
                    printf ("\033[32;1m[]BLOCK %lx has had enough votes in PHASE VALIDATE!\n\033[0m\n", iter->second.hash);
                    fflush(stdout);
                  }

                  bc->total_ask_for_verify1_blocks_in_one_round++;
                  bc->total_verify_local_block++;
                  block *bz = bc->find_block_by_hash_and_chain_id(iter->second.hash, iter->second.chain_id);
                  if(NULL !=bz && NULL != bz->nb){
                      bz->nb->consensusPart.verify_1_numbers = iter->second.consensusPart.verify_1_numbers;
                      bz->is_full_block = true;
                  }
                  bc->waiting_for_phase_1_block.erase(iter);

                  //round over
                  if(bc->waiting_for_phase_1_block.empty() /*&& bc->total_ask_for_verify1_blocks_in_one_round>=minerInfo->share_of_block*/){

                      unsigned long time_of_finish = std::chrono::system_clock::now().time_since_epoch() /  std::chrono::milliseconds(1);
                      float secs = (time_of_finish - time_of_start)/ 1000.0;

                      if(PRINT_CONSENSUS_MESSAGE){
                          printf ("\033[32;1m[]The consensus round %d has been finished, time duration %.2f\n\033[0m\n", cg->round, secs);
                          fflush(stdout);
                      }
                      //upadating
                      cg->add_in_history(cg->round, cg->miner_list, secs);

                      cg->concurrency_block_numbers = 0;
                      cg->miner_list.clear();
                      cg->round++;
                      cg->state = false;
                      bc->total_ask_for_verify1_blocks_in_one_round = 0;
                  }
              }
          }

      }

  }

  if( positions.size() > 1 &&  positions[p] < m.size())
      m = m.substr( positions[p] );
  else
      m = "";




}
