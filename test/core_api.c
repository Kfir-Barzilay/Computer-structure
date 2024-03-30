/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <stdbool.h>

#define REGS_COUNT 8

// each thread holds its current instruction number, register file,
// for how many cycles it will be busy and did it halt.
typedef struct threads {
	tcontext t_regs;
	uint32_t cur_inst;
	int is_idle;
	bool halt;
} thread;

// this struct is responsible on blocked MT. 
// the variable is_switching holds for how many cycles the core is busy.
// the variable is_finished hold how many threads have been finished.
typedef struct blocked_mt{
	int cycles;
	int inst_num;
	int num_of_threads;
	thread* threads;
	int is_switching;
	int store_lat;
	int load_lat;
	int switch_lat;
	int finished;
} blocked_MT;

// this struct is responsible on fine grained MT.
// the variable is_finished hold how many threads have been finished.
typedef struct fine_grained_mt{
	int cycles;
	int inst_num;
	int num_of_threads;
	thread* threads;
	int store_lat;
	int load_lat;
	int finished;
} fine_grained_MT;



blocked_MT blocked;
fine_grained_MT fine_grained;

// checks if the blocked MT is on idle state (NOP), and lowers each thread idle cycles by one.
bool idle(){
	bool idle = true;
	for (int i = 0; i < blocked.num_of_threads; i++){
		if (blocked.threads[i].is_idle > 0){
			blocked.threads[i].is_idle--;	
		}
		else if (!blocked.threads[i].halt){
			idle = false;
		}
	}
	return idle;
}


// checks if the core is currently busy on switching thread,
// and lowers the number of switching cycles by one.
bool switching(){
	if (blocked.is_switching > 0){
		blocked.is_switching-=1;
		return true;
	}
	return false;
}

// initialize blocked MT
void blocked_init(){
	blocked.num_of_threads = SIM_GetThreadsNum();
	blocked.is_switching = 0;
	blocked.store_lat = SIM_GetStoreLat();
	blocked.load_lat = SIM_GetLoadLat();
	blocked.switch_lat = SIM_GetSwitchCycles();
	blocked.finished = 0;
	blocked.cycles = 0;
	blocked.inst_num = 0;
	blocked.threads = (thread*)malloc(sizeof(thread)*blocked.num_of_threads);
	if (!blocked.threads){
		return;
	}
	for (int i = 0; i < blocked.num_of_threads; i++){
		blocked.threads[i].cur_inst = 0;
		blocked.threads[i].is_idle = 0;
		blocked.threads[i].halt = false;
		for (int j = 0; j < REGS_COUNT; j++){
			blocked.threads[i].t_regs.reg[j] = 0;
		}
	}
	
}

// executes a command and updates the register file and the instruction index.
void preform_inst(thread *cur_thread,Instruction cur_inst){
	int opc = cur_inst.opcode;
	int dst = cur_inst.dst_index;
	int src1 = cur_inst.src1_index;
	int src2 = cur_inst.src2_index_imm;
	switch(opc){
		case CMD_NOP:
			break;
		case CMD_ADDI:
			cur_thread->t_regs.reg[dst] = cur_thread->t_regs.reg[src1] + src2;
			break;
		case CMD_ADD:
			cur_thread->t_regs.reg[dst] = cur_thread->t_regs.reg[src1] + cur_thread->t_regs.reg[src2];
			break;
		case CMD_SUBI:
			cur_thread->t_regs.reg[dst] = cur_thread->t_regs.reg[src1] - src2;
			break;
		case CMD_SUB:
			cur_thread->t_regs.reg[dst] = cur_thread->t_regs.reg[src1] - cur_thread->t_regs.reg[src2];
			break;
		case CMD_LOAD:
			if(cur_inst.isSrc2Imm){
				SIM_MemDataRead(cur_thread->t_regs.reg[src1] + src2,&(cur_thread->t_regs.reg[dst]));
			}
			else
				SIM_MemDataRead(cur_thread->t_regs.reg[src1] + cur_thread->t_regs.reg[src2],&(cur_thread->t_regs.reg[dst]));
			cur_thread->is_idle = SIM_GetLoadLat();
			break;
		case CMD_STORE:
			if(cur_inst.isSrc2Imm){
				SIM_MemDataWrite(cur_thread->t_regs.reg[dst] + src2,cur_thread->t_regs.reg[src1]);
			}
			else
				SIM_MemDataWrite(cur_thread->t_regs.reg[dst] + cur_thread->t_regs.reg[src2],cur_thread->t_regs.reg[src1]);
			cur_thread->is_idle = SIM_GetStoreLat();
			break;
		case CMD_HALT:
			cur_thread->halt = true;
			break;
	}
	cur_thread->cur_inst++;
}

// initialize the fine grained MT.
void fine_grained_init(){
	fine_grained.num_of_threads = SIM_GetThreadsNum();
	fine_grained.store_lat = SIM_GetStoreLat();
	fine_grained.load_lat = SIM_GetLoadLat();
	fine_grained.finished = 0;
	fine_grained.cycles = 0;
	fine_grained.inst_num = 0;
	fine_grained.threads = (thread*)malloc(sizeof(thread)*fine_grained.num_of_threads);
	if (!fine_grained.threads){
		return;
	}
	for (int i = 0; i < fine_grained.num_of_threads; i++){
		fine_grained.threads[i].cur_inst = 0;
		fine_grained.threads[i].is_idle = 0;
		fine_grained.threads[i].halt = false;
		for (int j = 0; j < REGS_COUNT; j++){
			fine_grained.threads[i].t_regs.reg[j] = 0;
		}
	}

}

// lowers the number of idle cycles that remained for every thread.
void fg_idle_update(){
	for (int j = 0; j < fine_grained.num_of_threads; j++){
		if (fine_grained.threads[j].is_idle > 0){
			fine_grained.threads[j].is_idle -= 1;
		}
	}
}

// this function simulates the Blocked MT Core
void CORE_BlockedMT() {
	blocked_init();
	Instruction cur_inst;
	int cur_thread = 0;
	//while there are still instructions that haven't been executed keep running.
	// the simultion
	while (blocked.finished < blocked.num_of_threads){
		//increase the number of cycles the simultion is running
		blocked.cycles++;
		//if the current thread is idle the core will enter a switching phase to free thread
		//if one exists, and moves to the next cycle.
		if (blocked.threads[cur_thread].is_idle > 0){
			int cur_num = cur_thread;
			cur_thread++;
			cur_thread = (cur_thread)%(blocked.num_of_threads);
			while (blocked.threads[cur_thread].is_idle > 0 && cur_num != cur_thread){
				cur_thread++;
				cur_thread = (cur_thread)%(blocked.num_of_threads);
			}
			idle();
			if(cur_num != cur_thread)
				blocked.is_switching = blocked.switch_lat - 1;
			continue;
		}
		
		//checks if all threads are idle and moves to next cycle if true
		bool is_idle = idle();
		if (is_idle ){
			continue;
		}
		
		//checks if the core is currently switching theards and move the cycle by one
		// if it is.
		if (switching()){
			continue;
		}
		
		cur_thread = (cur_thread)%(blocked.num_of_threads);
		// the core is flexbile so it will skip all halted threads.
		while (blocked.threads[cur_thread].halt){
			cur_thread++;
			cur_thread = (cur_thread)%(blocked.num_of_threads);
		}
		if(!blocked.threads[cur_thread].is_idle){
			//reads the next instruction in the current thread and executes it.
			SIM_MemInstRead(blocked.threads[cur_thread].cur_inst, &cur_inst, cur_thread);
			preform_inst(&blocked.threads[cur_thread],cur_inst);
			// if the current thread halts increases the number of finished threads and 
			// enters a switching phase.
			if(blocked.threads[cur_thread].halt){
				blocked.finished++;
				blocked.is_switching = blocked.switch_lat;
			}
			blocked.inst_num++;
		}
	}
}

// this function is responsible fo fine grained MT actions.
void CORE_FinegrainedMT() {
	fine_grained_init();
	// checks if we finished all the threads.
	while (fine_grained.finished < fine_grained.num_of_threads){
		// changed is a flag to see if we executes an instraction on specific clock cycle. 
		bool changed = false;
		for (int i = 0; i < fine_grained.num_of_threads; i++){
			Instruction inst;
			// checks if the thread is active and not busy.
			if (!fine_grained.threads[i].halt && !fine_grained.threads[i].is_idle){
				fg_idle_update();
				changed = true;
				fine_grained.cycles++;
				fine_grained.inst_num++;
				SIM_MemInstRead(fine_grained.threads[i].cur_inst, &inst, i);
				preform_inst(&(fine_grained.threads[i]), inst);
				if(fine_grained.threads[i].halt){
					fine_grained.finished++;
				}
			}
		}
		// in case changed is still false, it means that we need to add NOP. 
		if(!changed){
			fine_grained.cycles++;
			fg_idle_update();
		}
	}

}

// free the memory allocations and returns CPI.
double CORE_BlockedMT_CPI(){
	free(blocked.threads);
	return ((double)blocked.cycles/(double)blocked.inst_num);
}

// free the memory allocations and returns CPI.
double CORE_FinegrainedMT_CPI(){
	free(fine_grained.threads);
	return ((double)fine_grained.cycles/(double)fine_grained.inst_num);
	return 0;
}

// returns the register file for a specific thread.
void CORE_BlockedMT_CTX(tcontext* context, int threadid){
	for(int i = 0; i < REGS_COUNT; i++){
		context[threadid].reg[i] = blocked.threads[threadid].t_regs.reg[i];
	}
}

// returns the register file for a specific thread.
void CORE_FinegrainedMT_CTX(tcontext* context, int threadid){
	for(int i = 0; i < REGS_COUNT; i++){
		context[threadid].reg[i] = fine_grained.threads[threadid].t_regs.reg[i];
	}
}