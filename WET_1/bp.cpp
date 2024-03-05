/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */


#include <iostream>
#include <math.h>
#include "bp_api.h"

#define MAXTABLESIZE 32
#define T true
#define NT false
#define BTBWIDTH 4
#define FAILED -1
#define SUCCESS 0
#define DSTWIDTH 30
#define FSMWIDTH 2
#define MAXHISTORY 8
#define MAXFSMSIZE 256 //2^8

using namespace std;
void print_all();

enum TBT_col {
	TAG = 0,
	DST,
	HISTORY,
	VALID
};

enum FSM_state { 
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
	unsigned fsm_default_state; 
	bool isGlobalHistoty; //if true only BTBtable[0][HISTORY] is relevant
	bool isGlobalTable; //if true only FSMtable[0][FSM_Row] is relevant
	int shared;
	// BTBtable[rows][3];
	//rows = TBD
	//col[1] = tag , col[2] = target jump , col[3] = local history
	//if (isGlobalHistoty == true) then BTBtable[i][3] is equal for all i
	unsigned BTBtable[MAXTABLESIZE][BTBWIDTH];
	//FSMtable[col_in_btb][row_in_specific table]
	//rows = TBD , number of tables
	//col[i] , the relevant state machine acoording to history
	//in case of (globalTable == true) rows = 1 only 1 table.
	unsigned FSMtable[MAXTABLESIZE][MAXFSMSIZE]; 
	SIM_stats stats;

	branchPredictor(); // default constructor
	branchPredictor(unsigned BTB_size,
					unsigned BTB_width, 
					unsigned history_width, 
					unsigned tag_width,
					unsigned fsm_default_state,
					bool isGlobalHistoty,
					bool isGlobalTable,
					int shared); // constructor
	branchPredictor(const branchPredictor& a);

};

//---------------------------------------------------------------------------------------
branchPredictor::branchPredictor()
{
	this->BTB_size = 0;
	this->BTB_width = 0;
	this->history_width = 0;
	this->tag_width = 0;
	this->fsm_default_state = 0;
	this->isGlobalHistoty = false;
	this->isGlobalTable  = false;
	this->shared = 0;
	this->stats.br_num =0;
	this->stats.flush_num =0;
	this->stats.size =0;
}

branchPredictor::branchPredictor(unsigned BTB_size,
								unsigned BTB_width,
								unsigned history_width, 
								unsigned tag_width,
								unsigned fsm_default_state,
								bool isGlobalHistoty,
								bool isGlobalTable,
								int shared)
{
	this->BTB_size = BTB_size;
	this->BTB_width = BTB_width;
	this->history_width = history_width;
	this->tag_width = tag_width;
	this->fsm_default_state = fsm_default_state;
	this->isGlobalHistoty = isGlobalHistoty;
	this->isGlobalTable = isGlobalTable;
	this->shared = shared;
	//BTB
	for (int i = 0 ; i < MAXTABLESIZE ; i++) {
		BTBtable[i][TAG] = 0;
		BTBtable[i][DST] = 0;
		BTBtable[i][HISTORY] = 0;
		BTBtable[i][VALID] = 0;	
	}
	
	//FSM
	for (int i = 0 ; i < MAXTABLESIZE ; i++) {
		for (int j = 0 ; j < MAXFSMSIZE ; j++) {
			this->FSMtable[i][j] = fsm_default_state;
		}		
	}

	this->stats.br_num =0;
	this->stats.flush_num =0;
	unsigned tag_bits = tag_width * BTB_size;
	unsigned dest_bits = DSTWIDTH * BTB_size;
	unsigned hist_bits = (isGlobalHistoty) ? 
						history_width : history_width * BTB_size;
	unsigned valid_bits = BTB_size;
	unsigned fsm_num = (isGlobalTable) ? 1 : BTB_size;
	unsigned fsm_bits = fsm_num * (FSMWIDTH * pow(2, history_width));

	this->stats.size = tag_bits + dest_bits + 
					   hist_bits + valid_bits + fsm_bits;
}

branchPredictor::branchPredictor(const branchPredictor& a)
{
	this->BTB_size = a.BTB_size;
	this->BTB_width = a.BTB_width;
	this->history_width = a.history_width;
	this->tag_width = a.tag_width;
	this->fsm_default_state = a.fsm_default_state;
	this->isGlobalHistoty = a.isGlobalHistoty;
	this->isGlobalTable = a.isGlobalTable;
	this->shared = a.shared;
	
	//BTB
		for (int i = 0 ; i < MAXTABLESIZE ; i++) {
			BTBtable[i][TAG] = a.BTBtable[i][TAG];
			BTBtable[i][DST] = a.BTBtable[i][DST];
			BTBtable[i][HISTORY] = a.BTBtable[i][HISTORY];
			BTBtable[i][VALID] = a.BTBtable[i][VALID];
		}
		
	//FSM
		for (int i = 0 ; i < MAXTABLESIZE ; i++) {
			for (int j = 0 ; j < MAXFSMSIZE ; j++) {
				this->FSMtable[i][j] = a.FSMtable[i][j];
			}
		}

	this->stats.br_num =a.stats.br_num;
	this->stats.flush_num =a.stats.flush_num;
	this->stats.size = a.stats.size;
}

branchPredictor bp;

/*****************************************************
desc: return the tag value for a pc
******************************************************/
uint32_t get_tag(uint32_t pc)
{
	uint32_t mask = pow(2, (2 + bp.BTB_width + bp.tag_width)) - 1;
	uint32_t tag = (pc & mask) >> (2 + bp.BTB_width); 
	return tag;
}

/*************************************************************
descriptoin: give you the  location of the proper
fsm according to pc for table with 
*************************************************************/
uint32_t get_BTB_row(uint32_t pc)
{
	uint32_t mask = pow(2, (2 + bp.BTB_width)) - 1;
	uint32_t BTB_row = (pc & mask) >> 2; 			
	return BTB_row;
}

/***************************************************************
desc = return row on the fsm array/table
***************************************************************/
uint32_t get_FSM_row(uint32_t pc)
{
	
	uint32_t BTB_row = (bp.isGlobalHistoty) ?  0 : get_BTB_row(pc);
	uint32_t curr_history = bp.BTBtable[BTB_row][HISTORY];
	if(!bp.isGlobalTable) { //irelevent
		return curr_history;
	}

	uint32_t pc_mask;
	switch (bp.shared)
	{
	case 1: //LSB
		{ 
		uint32_t mask = pow(2, (2 + bp.history_width)) -1;
		pc_mask = (pc & mask) >> 2 ;
		break;
		}
	case 2: //Mid bit
		{	
		uint32_t mask_low = pow(2, (16 + bp.history_width)) - 1;
		uint32_t mask_high = (pow(2,16) - 1);
		mask_high = ~mask_high;
		pc_mask = (pc & mask_low & mask_high) >> 16;
		break;
		}
	default: 
		pc_mask = pow(2,(bp.history_width)) -1;
		break;
	}
	return pc_mask ^ curr_history;
}


/***************************************************
desc: 
****************************************************/
void update_FSM(uint32_t pc ,bool isTaken) 
{
	uint32_t rowInBTB = get_BTB_row(pc);
	uint32_t rowInFSM= get_FSM_row(pc);
	uint32_t table_index = (bp.isGlobalTable) ? 0 : rowInBTB;
	unsigned state = bp.FSMtable[table_index][rowInFSM];
	if(isTaken) {
		state = (state < ST) ? state + 1 : ST;
	}
	else {
		state = (state > SNT) ? state - 1 : SNT;
	}
	bp.FSMtable[table_index][rowInFSM] = state;
}
/***********************************************************
desc: update a new pc (BR command) in the BTB table
************************************************************/
void add_BTB(uint32_t pc, uint32_t targetPc)
{
	uint32_t row = get_BTB_row(pc);
	bp.BTBtable[row][TAG] = get_tag(pc);
	bp.BTBtable[row][DST] = targetPc;
	bp.BTBtable[row][VALID] = 1;
	if(!bp.isGlobalHistoty) {
		bp.BTBtable[row][HISTORY] = 0;
	}
}

void update_BTB(uint32_t pc, bool isTaken, uint32_t targetPc )
{
	uint32_t BTB_row = get_BTB_row(pc);
	uint32_t table_index = (bp.isGlobalHistoty) ? 0 : BTB_row;
	unsigned curr_history = bp.BTBtable[table_index][HISTORY] << 1;
	curr_history = (isTaken) ? curr_history + 1 : curr_history; 
	uint32_t mask = pow(2,(bp.history_width)) - 1;
	curr_history = curr_history & mask;
	bp.BTBtable[table_index][HISTORY] = curr_history;
	bp.BTBtable[BTB_row][DST] = targetPc;
}


//-------------- The functions needed for implement ----------------------//

int BP_init(unsigned btbSize,
			unsigned historySize,
			unsigned tagSize,
			unsigned fsmState,
			bool isGlobalHist, 
			bool isGlobalTable, 
			int Shared)
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
		if (Shared < 0 || Shared > 2) {
			//shared value not valid
			return FAILED;
		}
	

	bp = branchPredictor(btbSize,
							BTB_width,
							historySize, 
							tagSize,
							fsmState,
							isGlobalHist,
							isGlobalTable,
							Shared);
	return SUCCESS;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	//protections 
		if(bp.BTBtable == NULL || bp.FSMtable == NULL) {
			cout << "bp wasnt init, shouldnt happen" << endl;
			return false;
		}
		
		if (dst == NULL) {
			dst = new uint32_t[1];
		}
		
	//get BTB_row
		unsigned BTB_row = get_BTB_row(pc);
		bool isNew = (!bp.BTBtable[BTB_row][VALID]) || 
					(get_tag(pc) != bp.BTBtable[BTB_row][TAG]);
		if (isNew) { //unknown branch, tag is not in BTB
			*dst = pc + 4;
			return NT ;
		}

	//get FSM_table_index
		unsigned FSM_table_index = (bp.isGlobalTable) ? 0 : BTB_row;

	//get FSM row according to the history and share
		unsigned row_in_table = get_FSM_row(pc);

	//get the curret guess
		bool isTaken = (bp.FSMtable[FSM_table_index][row_in_table] >= WT) ? 
						T : NT;

	//update *dest
		*dst = (isTaken) ? bp.BTBtable[BTB_row][DST] : pc + 4; 

	//return value 
		return isTaken;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	//protections 
		if(bp.BTBtable == NULL || bp.FSMtable == NULL) {
			cout << "bp wasnt init, shouldnt happen" << endl;
			return;
		}
	
	//update stats 
	
		bp.stats.br_num ++;
		if ((taken && (targetPc != pred_dst)) || 
			(!taken && (pc + 4 != pred_dst))) {
			bp.stats.flush_num++;
		}

	//update table
		unsigned BTB_row = get_BTB_row(pc);
		bool isNew = (!bp.BTBtable[BTB_row][VALID]) ||
					 (get_tag(pc) != bp.BTBtable[BTB_row][TAG]);
		if (isNew) {
			add_BTB(pc, targetPc);
			if(!bp.isGlobalTable) { //reset table
				for (int i = 0; i < pow(2, bp.history_width) ; i++) {
					bp.FSMtable[BTB_row][i] = bp.fsm_default_state;
				}
			}
		}
		update_FSM(pc, taken); 
		update_BTB(pc, taken, targetPc);
		
}
	
void BP_GetStats(SIM_stats *curStats) 
{
	if (curStats == NULL) {
		curStats = new SIM_stats[1];
	}
	(*curStats).flush_num = bp.stats.flush_num;           
	(*curStats).br_num= bp.stats.br_num;      	      
	(*curStats).size= bp.stats.size;
}

void print_all() { //function for debug
	cout << "is global table = " << bp.isGlobalTable << endl;
	cout << "bp,BTB_size value is: " << bp.BTB_size << endl;
	uint32_t size = bp.BTB_size;
	int table_rows = pow(2 , bp.history_width);
	for (uint32_t i = 0 ; i < size ; i++){
		if( bp.BTBtable[i] != NULL){
			cout << "tag: " << bp.BTBtable[i][TAG]  << " DST: " <<
			bp.BTBtable[i][DST];
			cout << " history: " << bp.BTBtable[i][HISTORY]  << " valid: " <<
			bp.BTBtable[i][VALID] <<endl;
		}
		for (int j = 0 ; j < table_rows ; j++){
			if(bp.FSMtable[i] != NULL)
				cout << bp.FSMtable[i][j] << "  ";
		}	
		cout << endl;
	}
	
}