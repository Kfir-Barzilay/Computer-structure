/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

//////stats size do not workkkk please repairrrrrrr!!!!!!!!!!!!!!!!!!!!!!/////////////////////////////
#include <iostream>
#include <math.h>
#include "bp_api.h"

#define MAXTABLESIZE 32
#define MAXBITS 5
#define T true
#define NT false
#define BTBWIDTH 4
#define FAILED -1
#define SUCCESS 0

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
	unsigned fsm_default_state; //when generating new machin this is the starting state
	bool isGlobalHistoty; //if true only BTBtable[0][HISTORY] is relevant
	bool isGlobalTable; //if true only FSMtable[0][FSM_Row] is relevant
	int shared;
	// BTBtable[rows][3];
	//rows = TBD
	//col[1] = tag , col[2] = target jump , col[3] = local history
	//if (isGlobalHistoty == true) then BTBtable[i][3] is equal for all i
	unsigned ** BTBtable;
	//FSMtable[col_in_btb][row_in_specific table]
	//rows = TBD , number of tables
	//col[i] , the relevant state machine acoording to history
	//in case of (globalTable == true) rows = 1 only 1 table.
	unsigned **FSMtable; 
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
	~branchPredictor(); // destructor
};

//---------------------------------------------------------------------------------------
branchPredictor::branchPredictor()
{
	cout << "was created" << endl;
	this->BTB_size = 0;
	this->BTB_width = 0;
	this->history_width = 0;
	this->tag_width = 0;
	this->fsm_default_state = 0;
	this->isGlobalHistoty = false;
	this->isGlobalTable  = false;
	this->shared = 0;
	this->BTBtable = NULL;
	this->FSMtable = NULL;
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
	cout << "was created" << endl;
	this->BTB_size = BTB_size;
	this->BTB_width = BTB_width;
	this->history_width = history_width;
	this->tag_width = tag_width;
	this->fsm_default_state = fsm_default_state;
	this->isGlobalHistoty = isGlobalHistoty;
	this->isGlobalTable = isGlobalTable;
	this->shared = shared;
	//BTB
	int rows_BTB = BTB_size;  
	int column_BTB = BTBWIDTH; 
	this->BTBtable = new unsigned*[rows_BTB];
	for (int i = 0 ; i < rows_BTB ; i++) {
		BTBtable[i] = new unsigned[column_BTB];	
	}
	for(int i = 0; i < rows_BTB; i++)
	{	
		BTBtable[i][TAG] = 0;
		BTBtable[i][DST] = 0;
		BTBtable[i][HISTORY] = 0;
		BTBtable[i][VALID] = 0;
	}
	//FSM
	int tables_FSM = (isGlobalTable) ? 1 : BTB_size; //if global table there is only 1 table, else each BTB_row gots its own table
	int rows_in_each_table = pow(2,history_width); //each key (history with or without fuction) got its own FSM
	this->FSMtable = new unsigned*[tables_FSM];
	for (int i = 0 ; i < tables_FSM ; i++) {
		FSMtable[i] = new unsigned[rows_in_each_table];	
	}

	for(int i = 0; i < rows_BTB; i++) // init the fsm for each row
	{	
		for (int j = 0 ; j < rows_in_each_table ; j++) {
			this->FSMtable[i][j] = fsm_default_state;
		}
	}
	this->stats.br_num =0;
	this->stats.flush_num =0;
	this->stats.size = 0;  /* please update */
}

branchPredictor::~branchPredictor()
{
	cout << "was destroyed" <<endl;
	if (this->BTBtable != nullptr) {
		for (int i = 0; i < this->BTB_size ; i++)
		{
			
			if ((this->BTBtable[i]) != nullptr) {
				delete(this->BTBtable[i]);
			}
		}
		delete(this->BTBtable);
	}
	
	unsigned tables_FSM = (this->isGlobalTable) ? 1 : BTB_size;
	if (this->FSMtable != NULL) {
		
		for (int i = 0; i < tables_FSM ; i++)
		{
			if (this->FSMtable[i] != NULL) {
				delete(this->FSMtable[i]);
			}
		}	
		delete(this->FSMtable);
	}
}

branchPredictor::branchPredictor(const branchPredictor& a)
{
	cout << "bp was copied"<<endl;
	this->BTB_size = a.BTB_size;
	this->BTB_width = a.BTB_width;
	this->history_width = a.history_width;
	this->tag_width = a.tag_width;
	this->fsm_default_state = a.fsm_default_state;
	this->isGlobalHistoty = a.isGlobalHistoty;
	this->isGlobalTable = a.isGlobalTable;
	this->shared = a.shared;
	
	//BTB
		int rows_BTB = BTB_size;  
		int column_BTB = BTBWIDTH; 
		this->BTBtable = new unsigned*[rows_BTB];
		for (int i = 0 ; i < rows_BTB ; i++) {
			BTBtable[i] = new unsigned[column_BTB];	
		}
		for(int i = 0; i < rows_BTB; i++)
		{	
			BTBtable[i][TAG] = a.BTBtable[i][TAG];
			BTBtable[i][DST] = a.BTBtable[i][DST];
			BTBtable[i][HISTORY] = a.BTBtable[i][HISTORY];
			BTBtable[i][VALID] = a.BTBtable[i][VALID];
		}
		
	//FSM
		int tables_FSM = (isGlobalTable) ? 1 : BTB_size; //if global table there is only 1 table, else each BTB_row gots its own table
		int rows_in_each_table = pow(2,history_width); //each key (history with or without fuction) got its own FSM
		this->FSMtable = new unsigned*[tables_FSM];
		for (int i = 0 ; i < tables_FSM ; i++) {
			FSMtable[i] = new unsigned[rows_in_each_table];	
		}

		for(int i = 0; i < rows_BTB; i++) // init the fsm for each row
		{	
			for (int j = 0 ; j < rows_in_each_table ; j++) {
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

void update_BTB(uint32_t pc, bool isTaken)
{
	uint32_t row = (bp.isGlobalHistoty) ? 0 : get_BTB_row(pc);
	unsigned curr_history = bp.BTBtable[row][HISTORY] << 1;
	curr_history = (isTaken) ? curr_history + 1 : curr_history; 
	uint32_t mask = pow(2,(bp.history_width)) - 1;
	curr_history = curr_history & mask;
	bp.BTBtable[row][HISTORY] = curr_history;
}


//-------------- The functions needed for implement ------------------------//

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
		if (Shared < 0 || Shared > 2) {
			//shared value not valid
			return FAILED;
		}
	/*
	cout << "btbSize = " << btbSize <<endl;
	cout << "BTB_width = " << BTB_width <<endl;
	cout << "historySize = " << historySize<<endl;
	cout << "tagSize =" << tagSize <<endl;
	cout << "fsmState = " <<fsmState <<endl;
	cout << "isGlobalHist = " << isGlobalHist <<endl;
	cout << "isGlobalTable = " << isGlobalTable<<endl;
	cout << "Shared = " << Shared <<endl;
	*/
	

	//creates branchPredictor
	branchPredictor* a = new branchPredictor(btbSize,
											BTB_width,
											historySize, 
											tagSize,
											fsmState,
											isGlobalHist,
											isGlobalTable,
											Shared);
	bp = *a;
	
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
		bool isNew = bp.BTBtable[BTB_row][VALID] && (get_tag(pc) != bp.BTBtable[BTB_row][TAG]);
		if (isNew) { //unknown branch, tag is not in BTB
			*dst = pc + 4;
			return (bp.fsm_default_state >= WT) ? T : NT ;
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
	cout << " *** ENTERED BP_UPDATE ***" << endl;
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
		cout << "branches are: " << bp.stats.br_num << " and flushes are: " << bp.stats.flush_num<< endl;

	//update table
		unsigned BTB_row = get_BTB_row(pc);
		bool isNew = bp.BTBtable[BTB_row][VALID] && (get_tag(pc) != bp.BTBtable[BTB_row][TAG]);
		if (isNew) {
			add_BTB(pc, targetPc);
			if(!bp.isGlobalTable) { //reset table
				for (int i = 0; i < pow(2, bp.history_width) ; i++) {
					bp.FSMtable[BTB_row][i] = bp.fsm_default_state;
				}
			}
		}
		update_FSM(pc, taken); 
		update_BTB(pc, taken);
		print_all();
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

void print_all() {
	cout << "is global table " << bp.isGlobalTable << endl;
	for (int i = 0 ; i < bp.BTB_size ; i++){
		if(bp.BTBtable != nullptr){
		cout << "tag: " << bp.BTBtable[TAG]  << " DST: " << bp.BTBtable[DST];
		cout << " history: " << bp.BTBtable[HISTORY]  << " valid: " << bp.BTBtable[VALID] <<endl;
		}
	}
	int table_FSM = (bp.isGlobalTable) ? 1 : bp.BTB_size;
	int table_rows = pow(2 , bp.history_width);
	for (int i = 0 ; i < table_FSM ; i++){
		for (int j = 0 ; i < table_rows ; i++){
			if(bp.FSMtable != nullptr)
				cout << bp.FSMtable[i][j] << "  ";
		}	
		cout << endl;
	}	
}