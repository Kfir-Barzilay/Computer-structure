/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include <math.h>
#include <iostream>
#include "bp_api.hpp"

#define MAXTABLESIZE 32
#define MAXBITS 5
#define T true
#define NT false

using namespace std;

enum TBT_col {
	TAG = 0,
	DST,
	HISTORY
}

enum FSM_state { //can decrease memory size by using typedef
	SNT = 0,
	WNT,
	WT,
	ST
};

class branchPredictor {
	public:
	unsigned BTB_size; //the rows of the btb table
	unsigned BTB_width;
	unsigned history_width; //number of history bits
	unsigned tag_width; //num of tag bits
	unsigned fsm_default_state; //when generating new machin this is the starting state
	bool isGlobalHistoty;
	bool isGlobalTabled;
	int shared;
	// BTBtable[rows][3];
	//rows = TBD
	//col[1] = tag , col[2] = target jump , col[3] = local history
	//if (isGlobalHistoty == true) then BTBtable[i][3] is equal for all i
	unsigned ** BTBtable
	//FSMtable[col_in_btb][row_in_specific table]
	//rows = TBD , number of table for the relevant BTB line
	//col[i] , the relevant state machine acoording to history
	//in case of (globalTable == true) rows = 1 only 1 table.
	unsigned **FSMtable; 

	branchPredictor(); // default constructor
	branchPredictor(unsigned BTB_size, 
					unsigned history_width, 
					unsigned tag_width,
					unsigned fsm_default_state,
					bool isGlobalHistoty,
					bool isGlobalTabled,
					int shared); // constructor
	branchPredictor(branchPredictor &bp); // copy constructor
	~branchPredictor(); // destructor
}

branchPredictor bp;


//---------------------------------------------------------------------------------------
branchPredictor::branchPredictor()
{
	this->BTB_size = 0;
	this->history_width = 0;
	this->tag_width = 0;
	this->fsm_default_state = 0;
	this->isGlobalHistoty = false;
	this->isGlobalTabled  = false;
	this->shared = 0;
	this->BTBtable = NULL;
	this->FSMtable = NULL;
}

branchPredictor::branchPredictor(unsigned BTB_size, 
					unsigned history_width, 
					unsigned tag_width,
					unsigned fsm_default_state,
					bool isGlobalHistoty,
					bool isGlobalTabled,
					int shared)
{
	this->BTB_size = BTB_size;
	this->history_width = history_width;
	this->tag_width = tag_width;
	this->fsm_default_state = fsm_default_state;
	this->isGlobalHistoty = isGlobalHistoty;
	this->isGlobalTabled = isGlobalTabled;
	this->shared = shared;
	//BTB
	int rows_BTB = BTB_size;  
	int column_BTB = 3; 
	int table_size = rows_BTB * column_BTB;
	this->BTBtable = new int(table_size);
	for(int i = 0; i < rows_BTB; i++)
	{	
		BTBtable[i][0] = ;/* please update */
		BTBtable[i][1] = ;/* please update */
		BTBtable[i][2] = ;/* please update */
	}
	//FSM
	int tables_FSM = (isGlobalTabled) ? 1 : BTB_size; //if global table there is only 1 table, else each BTB_row gots its own table
	int rows_in_each_table = 2**(history_width); //each key (history with or without fuction) got its own FSM
	table_size = rows_BTB * column_BTB;
	this->FSMtable = new int(table_size);
	for(int i = 0; i < rows_BTB; i++) // init the fsm for each row
	{	
		for (int j = 0 ; j < column_FSM ; < j++) {
			this->FSMtable[i][j] = fsm_default_state;
		}
	}
}

~branchPredictor::branchPredictor()
{
	if (this->BTBtable != NULL) {
		delete(this->BTBtable);
	}
	if (this->FSMtable != NULL) {
		delete(this->FSMtable);
	}
}


/*****************************************************
desc: return the tag value for a pc
******************************************************/
uint32_t get_tag(uint32_t pc)
{
	uint32_t mask = 2**(2 + bp.BTB_width + bp.tag_width) - 1;
	uint32_t tag = (pc & mask) >> (2 + bp.BTB_width); 
	return tag;
}

/*************************************************************
descriptoin: give you the  location of the proper
fsm according to pc for table with 
*************************************************************/
uint32_t get_BTB_row(uint32_t pc)
{
	uint32_t mask = 2**(2 + bp.BTB_width + 1) - 1;
	uint32_t BTB_row = (pc & mask) >> 2; 			
	return BTB_row;
}
/***************************************************************
desc = return row on the fsm array/table
***************************************************************/
uint32_t get_FSM_row( uint32_t pc)
{
	if(bp.shared == 0)//not using share
	{
		
	}
	//using share mid ()
	if(bp.shared == 1)
	{
		uint32_t mask = -1;
		mask = mask << 2
	}
	//using share lsb (Gshare) XOR of history with h LSBs of pc
	if(bp.shared == 2)
	{
		uint32_t mask = 2**(2 + bp.history_width + 1) - 1;
		uint32_t pc_low_bits = ( pc & mask) >> 2;

		return (bp.BTBtable[get_BTB_row(pc)][HISTORY] ^ pc_low_bits);
	}
	return -1;
}

/*********************************************************
description: update the history bits in the place needed
after the execute level 
*********************************************************/
void update_history(int place, bool isTaken)
{
	unsigned history = bp.BTBtable[place][HISTORY] << 1;
	history = (isTaken) history + 1 : history; 
	mask = 2**(history +1) - 1;
	history = history & mask;
	bp.BTBtable[place][HISTORY] = history;
}

//----------------------------------------------------------------------------

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
bool isGlobalHist, bool isGlobalTable, int Shared)
{
	//--------validate arguments 
	//btbSize
	switch (btbSize)
	{
		case 1:
			break;
		
		case 2:
			break;

		case 4:
			break;
		
		case 8:
			break;

		case 16:
			break;

		case 32:
			break;
		
		default:
			//btbSize not valid 
			return FAILED;
	}
	unsigned BTB_width = log2(btbSize);

	//historySize
	if (historySize < 1 || historySize > 8) {
		//historySize not valid
		return FAILED;
	}

	//tagSize
	unsigned max_tagsize = 32 - 2 - BTB_width;
	if (tagSize < 0 || tagSize > max_tagsize) {
		//tagSize not valid
		return FAILED;
	}

	//fsmState
	if (fsmState < SNT || fsmState > ST) {
		//fsmState default value not valid
		return FAILED;
	}
	
	//shared 
	if (Shared < 0 || Shared > 2) { //----------------------------------------------------------update condition
		//shared value not valid
		return FAILED
	}
	
	//creates branchPredictor
	bp = branchPredictor(BTB_size , 
						history_width, 
						tag_width,
						fsm_default_state,
						isGlobalHistoty,
						isGlobalTabled,
						shared);
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	//protections 
		if(bg.BTB_table == NULL || bp.FSMtable == NULL) {
			cout << "bp wasnt init, shouldnt happen" << endl;
			return false;
		}
		
		if (dst == NULL) {
			dst = new uint32_t[1];
		}
	
	//get BTB_row
		unsigned BTB_row = get_BTB_row(pc);
		if (get_tag(pc) != bg.BTB_table[BTB_row][TAG]) { //unknown branch, tag is not in BTB
			*dst = pc + 4;
			return NT;
		}

	//get FSM_table_index
		unsigned FSM_table_index = (bp.isGlobalTable) ? 0 : BTB_row;

	//get FSM row according to the history and share
		unsigned row_in_table = get_FSM_row();

	//get the curret guess
		bool isTaken = (bp.FSMtable[FSM_table_index][row_in_table] >= WT) ? 
						T : NT;
		
	//update *dest
		*dst = (isTaken) ? bg.BTB_table[BTB_row][DST] : pc + 4; 

	//return value 
		return isTaken;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	//protections 
		if(bg.BTB_table == NULL || bp.FSMtable == NULL) {
			cout << "bp wasnt init, shouldnt happen" << endl;
			return false;
		}
	//update stats

	
	//get BTB_row
		unsigned BTB_row = get_BTB_row(pc);
		//if not in BTB, branch isnt known , resets relevant table if needed 
			if (get_tag(pc) != bg.BTB_table[BTB_row][TAG]) {
				update_BTB(pc, targetPc);
				if(isGlobalTable) {
					update_history();
				}
				else {
					reset_table(BTB_row);
				}

			}
		//else, branch known
			else {
			//get FSM_table_index
				unsigned FSM_table_index = (bp.isGlobalTable) ? 0 : BTB_row;	
			//get FSM row according to the history and share
				unsigned row_in_table = get_FSM_row();
			//update the FSM
				update_history();
			}
}




