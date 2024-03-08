#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include "cache.hpp"

#define SUCCESS 0

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

/* globals */
bool write_allocate = false; 
double RAMcycles = 0;
double hitL1=0, hitL2=0, missL1=0, missL2=0 ,cycles= 0 , calls = 0;
double L1MissRate = 0;
double L2MissRate = 0;
double avgAccTime = 0;

/*func decleration*/
void cache_init( cache_t *&L1,
				cache_t *&L2, 
				int BSize, 
				int L1Size,   
				int L2Size,
				int L1Assoc,
				int L2Assoc, 
				int L1Cyc, 
				int L2Cyc
);

int read_op(cache_t *L1, cache_t *L2 ,int address);
int write_op(cache_t *L1, cache_t *L2 ,int address);
void update_results(int stat, int cycles_needed);



int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
			cout << "L1assoc " << L1Assoc << endl;
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
			cout << "L2assoc " << L2Assoc << endl;
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	/*initate the cache starcts*/
	write_allocate = bool(WrAlloc);
	RAMcycles = MemCyc;
	cache_t *L1;
	cache_t *L2;
	cache_init(	L1, 
				L2,  
				BSize, 
				L1Size, 
				L2Size,
				L1Assoc,
				L2Assoc, 
				L1Cyc, 
				L2Cyc	
	);
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		
		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		if(operation == 'r')read_op(L1,L2,num);
		else if(operation == 'w') write_op(L1,L2,num);
		else{cout << "error in operation input" << endl;}
		if (operation == 'r' || write_allocate) cout << "update is needed" <<endl;
		cout << "------------L1 cache table------------------" << endl;
		cout << "Tag is: " << L1->get_tag(num) << " , Set is: " << L1->get_set(num) << endl;
		
		L1->printTable();
		cout << "------------L2 cache table------------------" << endl;
		cout << "Tag is: " << L2->get_tag(num) << " , Set is: " << L2->get_set(num) << endl;
		L2->printTable();
		cout << endl << endl;
	}



	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

/*************
 init L1 and L2 cache structs
***************/
void cache_init(cache_t *&L1,
				cache_t *&L2, 
				int BSize, 
				int L1Size,   
				int L2Size,
				int L1Assoc,
				int L2Assoc, 
				int L1Cyc, 
				int L2Cyc) 
{
    L1 = new cache(BSize,L1Size,L1Assoc,L1Cyc);
	L2 = new cache(BSize,L2Size,L2Assoc,L2Cyc);
}

/*executes read operation*/
int read_op(cache_t *L1, cache_t *L2 ,int address)
{
	if (L1->has(address)) { /*In L1 cache*/ 
		/*fill*/
		L1->update_LRU_counter(address);
		L2->update_LRU_counter(address);//the question is if we need to update L2 and if so do we need a different calc for tag and set?
		update_results(1 , L1->MemCyc);
		return SUCCESS;
	}
	
	L1->emptySpaceIfNeeded(address, L1 , L2 , 1);
	if (L2->has(address)) { /*In L2 cache*/ 
		L1->insert(address);
		L2->update_LRU_counter(address);
		update_results(2,  L1->MemCyc +  L2->MemCyc);
		return SUCCESS;
	}	
	
	L2->emptySpaceIfNeeded(address, L1 , L2 , 2);

	/*not in any cache*/
	update_results(3, L1->MemCyc +  L2->MemCyc + RAMcycles);
	L1->insert(address);
	L2->insert(address);
	L1->update_LRU_counter(address);
	L2->update_LRU_counter(address);
	return SUCCESS;	
}

/*executes write operation*/
int write_op(cache_t *L1, cache_t *L2 ,int address) 
{
	if (L1->has(address)) { /*In L1 cache*/ 
		L1->dirty(address);
		L1->update_LRU_counter(address);
		L2->update_LRU_counter(address);
		update_results(1 , L1->MemCyc);
		return SUCCESS;
	}

	if (write_allocate) {
		L1->emptySpaceIfNeeded(address, L1 , L2 , 1);
	}
	if (L2->has(address)) { /*In L2 cache*/ 
		if(write_allocate) {
			L1->insert(address);
			L1->dirty(address);
			L1->update_LRU_counter(address);
		} 
		else {
			L2->dirty(address);
		}
		L2->update_LRU_counter(address);
		update_results(2, L1->MemCyc +  L2->MemCyc);
		return SUCCESS;
	}

	if (write_allocate) {
		L2->emptySpaceIfNeeded(address, L1 , L2 , 2);
	}

	/*not in any cache*/
	update_results(3 , L1->MemCyc +  L2->MemCyc + RAMcycles);
	if(write_allocate) { //?????????
		L1->insert(address);
		L1->dirty(address);
		L2->insert(address);
		L1->update_LRU_counter(address);
		L2->update_LRU_counter(address);
	}
	return SUCCESS;	
}


void update_results(int stat , int cycles_needed)
{	
	calls++;
	cycles += cycles_needed;
	switch (stat)
	{
	case 1://hit L1
		hitL1 += 1;
		break;
	
	case 2://miss L1, hit L2
		missL1 += 1;
		hitL2 += 1;
		break;
		
	case 3://miss L1,L2
		missL1 += 1;
		missL2 += 1;
		break;

	default:
		break;
	}
	L1MissRate = (missL1/(missL1+hitL1));
	L2MissRate= (missL2/(missL2+hitL2));
	avgAccTime = (cycles / calls);
}