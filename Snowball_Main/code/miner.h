#ifndef MINER_H
#define MINER_H

#include "Blockchain.hpp"
#include "requests.h"
#include "Consensus_Group.h"

bool mine_new_block( Blockchain *bc, consensus_part cp);
bool mine_Consensus_blocks(Blockchain *bc, Consensus_Group *cg);
void miner( Blockchain *bc, Consensus_Group *cg);


#endif
