/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <stdlib.h>


#define FAIL -1
#define DEBUG 0
double Blocked_CPI = 0;
double Fine_CPI = 0;
thread_t* threads;

void threads_init(int thread_num);

void threads_destroy(int thread_num);

void do_op(int thread_id , Instruction *inst);

int get_next_thread2(int curr_thread);

bool all_busy();

void update_busy();

bool is_busy(int thread_id);

int get_next_thread(int curr_thread);

bool isRunning();


void CORE_BlockedMT() 
{
	double inst_counter = 0;
	double clk_counter = 0;
	int threads_num = SIM_GetThreadsNum();
	threads = malloc(sizeof(thread_t)*threads_num);
	threads_init(threads_num);
	Instruction *inst = malloc(sizeof(Instruction));
	int curr_thread = 0;
	

	while (isRunning())
	{
		if (DEBUG) printf("cycle num: %f, Thread: %d\n", clk_counter , curr_thread);
		if(is_busy(curr_thread) || (!threads[curr_thread].running))//every thread is occupy
		{
			int next_thread = get_next_thread(curr_thread);
			if (next_thread != curr_thread) { //someones rdy
				clk_counter += SIM_GetSwitchCycles(); //swiching penalty
				update_busy(SIM_GetSwitchCycles());
				curr_thread = next_thread;
			}
			else { //no one rdy lets wait 1 clock
				update_busy(1);
				clk_counter++;
			}
			continue;
		}
		/*do op*/
		update_busy(1);
		SIM_MemInstRead(threads[curr_thread].current_line , inst, curr_thread);
		do_op(curr_thread , inst);
		clk_counter++;
		inst_counter++;
		threads[curr_thread].current_line++;
		
	}
	Blocked_CPI = clk_counter / inst_counter;	
	free(inst);
}

void CORE_FinegrainedMT() 
{
	
	double inst_counter =0;
	double clk_counter = 0;
	int threads_num = SIM_GetThreadsNum();
	threads_init(threads_num);
	Instruction *inst = malloc(sizeof(Instruction));
	int curr_thread = 0;
	while(isRunning())
	{
		if(DEBUG) printf("cycle num: %f, Thread: %d\n", clk_counter , curr_thread);
		/* ------- all the threads are busy ----- */
		if(all_busy())
		{
			update_busy(1);
			clk_counter++;
			curr_thread = get_next_thread2(curr_thread);
			continue;
		}
		update_busy(1);
		SIM_MemInstRead(threads[curr_thread].current_line , inst, curr_thread);
		do_op(curr_thread, inst);
		threads[curr_thread].current_line++;
		inst_counter++;
		clk_counter++;
		curr_thread = get_next_thread2(curr_thread);
		
	}
	free(inst);
	Fine_CPI = clk_counter / inst_counter;
}

double CORE_BlockedMT_CPI()
{
	threads_destroy(SIM_GetThreadsNum());
	return Blocked_CPI;
}

double CORE_FinegrainedMT_CPI()
{
	threads_destroy(SIM_GetThreadsNum());
	return Fine_CPI;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) 
{
	for (int i = 0 ; i < REGS_COUNT ; i++)
	{
		context[threadid].reg[i] = threads[threadid].regs->reg[i];
		}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) 
{
	for (int i = 0 ; i < REGS_COUNT ; i++)
	{
		context[threadid].reg[i] = threads[threadid].regs->reg[i];
	}
}

void threads_init(int threads_num)
{
	threads = malloc(sizeof(thread_t)*threads_num);
	for(int j=0; j < threads_num; j++)
	{
		threads[j].regs	 = malloc(sizeof(tcontext));
		for(int i = 0; i < REGS_COUNT; i++)
		{
			threads[j].regs->reg[i] = 0;
		}
		threads[j].current_line = 0;
		threads[j].busy_counter = 0;
		threads[j].running = true;
	}
}

void threads_destroy(int thread_num)
{
	for(int j=0; j < thread_num; j++)
	{
		if(threads[j].regs != NULL)
			free(threads[j].regs);
	}
	if(threads != NULL)
		free(threads);
}

void do_op (int thread_id , Instruction *inst)
{
	thread_t *thread = &(threads[thread_id]);
	cmd_opcode opcode = inst->opcode;
	int dst_index = inst->dst_index;
	int src1_index = inst->src1_index;
	int src2_index_imm = inst->src2_index_imm;
	bool isSrc2Imm = inst->isSrc2Imm;
	uint32_t addr;
	switch (opcode) {
		case CMD_NOP:
			break;

		case CMD_ADD:  // dst <- src1 + src2
			thread->regs->reg[dst_index] = 	thread->regs->reg[src1_index] + 
										thread->regs->reg[src2_index_imm]; 
			break;

		case CMD_SUB:  // dst <- src1 - src2
			thread->regs->reg[dst_index] = 	thread->regs->reg[src1_index] - 
										thread->regs->reg[src2_index_imm]; 
			break;

		case CMD_ADDI: // dst <- src1 + imm
			thread->regs->reg[dst_index] = 	thread->regs->reg[src1_index] + 
										src2_index_imm; 
			break;

		case CMD_SUBI: // dst <- src1 - imm
			thread->regs->reg[dst_index] = 	thread->regs->reg[src1_index] -
										src2_index_imm; 
			break;

		case CMD_LOAD: // dst <- Mem[src1 + src2]  (src2 may be an immediate)
			addr = (isSrc2Imm) ? 
				thread->regs->reg[src1_index] + src2_index_imm :
				thread->regs->reg[src1_index] + thread->regs->reg[src2_index_imm];
			int32_t dst = 0;
			SIM_MemDataRead(addr , &dst);
			thread->regs->reg[dst_index] = dst;	
			thread->busy_counter = SIM_GetLoadLat();
			break;

		case CMD_STORE:// Mem[dst + src2] <- src1  (src2 may be an immediate)
			addr = (isSrc2Imm) ? 
				thread->regs->reg[dst_index] + src2_index_imm :
				thread->regs->reg[dst_index] + thread->regs->reg[src2_index_imm];  
			SIM_MemDataWrite(addr , thread->regs->reg[src1_index]);	
			thread->busy_counter = SIM_GetStoreLat();
			break;

		case CMD_HALT:
			thread->running = false;
			break;
						
		default:
			break;
	}
}

int get_next_thread(int curr_thread)
{
	int th_num = SIM_GetThreadsNum();
	int index;
	for(int i = 0 ; i < th_num ;i++)
	{
		index =(curr_thread + i) % th_num;
		if((threads[index].busy_counter== 0) && (threads[index].running)) 
		{
			return (index);
		}
	}
	return curr_thread;
}

int get_next_thread2(int curr_thread)
{
	int th_num = SIM_GetThreadsNum();
	int index;
	for(int i = 1 ; i <= th_num ;i++)
	{
		index =(curr_thread + i) % th_num;
		if((threads[index].busy_counter== 0) && (threads[index].running)) 
		{
			return (index);
		}
	}
	return curr_thread;
}


bool is_busy(int thread_id)
{
	return (threads[thread_id].busy_counter != 0);
}

bool all_busy()
{
	int th_num = SIM_GetThreadsNum();
	for(int i = 0; i < th_num; i++)
	{
		if(!is_busy(i) && threads[i].running)
			return false;
	}
	return true;
}

void update_busy(int num_of_clocks)
{
	int th_num = SIM_GetThreadsNum();
	for(int i=0; i < th_num; i++)
	{
		if(is_busy(i))
		{
			threads[i].busy_counter -= num_of_clocks;
			threads[i].busy_counter = (threads[i].busy_counter < 0) ?
										0 : 
										threads[i].busy_counter;
		}

	}
}

bool isRunning() 
{
	int th_num = SIM_GetThreadsNum();
	for(int i = 0; i < th_num ;i++)
	{
		if(threads[i].running)
		{
			return true;
		}
	}
	return false; 
}

