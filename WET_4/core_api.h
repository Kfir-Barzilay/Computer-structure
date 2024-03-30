/* 046267 Computer Architecture - HW #4 */

#ifndef CORE_API_H_
#define CORE_API_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#define REGS_COUNT 8

typedef enum {
	CMD_NOP = 0,
    CMD_ADD,     // dst <- src1 + src2
    CMD_SUB,     // dst <- src1 - src2
    CMD_ADDI,    // dst <- src1 + imm
    CMD_SUBI,    // dst <- src1 - imm
    CMD_LOAD,    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
    CMD_STORE,   // Mem[dst + src2] <- src1  (src2 may be an immediate)
	CMD_HALT,
} cmd_opcode;

/**
 * @field opcode - the function we need to execute
 * @field dst_index - the dest for the operation
 * @field src1_index - source one for the operation
 * @field src2_index_imm - second source for the operation, can be immidiate
 * @field isSrc2Imm - '1' if the second argument is immidiate, '0' not imm.
*/
typedef struct _inst {
	cmd_opcode opcode;
	int dst_index;
	int src1_index;
	int src2_index_imm;
	bool isSrc2Imm; // if the second argument is immediate
} Instruction;

/**
 * @field reg - array of registers that belong to the thread.
*/
typedef struct _regs {
	int reg[REGS_COUNT];
} tcontext;

/**
 * @field regs - The registers of the thread.
 * @field current_line - The current instruction line.
 * @field load_counter - In case of load holds the number of cycles in which
						the load takes place in. 0 when thers no load.
 * @field store_counter - In case of load holds the number of cycles in which
						the load takes place in. 0 when thers no store.
*/
typedef struct thread{
	tcontext* regs;
	int current_line;
	int busy_counter; 
	bool running;
} thread_t;

/* Simulates blocked MT and fine-grained MT behavior, respectively */
/**
 * @brief the function will simulate the Fine Grained MT operation
 * 		  on the core 
*/
void CORE_BlockedMT();

/**
 * @brief the function will simulate the Fine Grained MT operation
 * 		  on the core
*/
void CORE_FinegrainedMT();

/* -- Get thread register file through the context pointer -- */

/**
 * @brief the function will switch between the register file
 * 		  according to the context switched that occur.
*/
void CORE_BlockedMT_CTX(tcontext context[], int threadid);

/**
 * @brief the function will switch between the register file
 * 		  according to the context switched that occur.
*/
void CORE_FinegrainedMT_CTX(tcontext context[], int threadid);

/* ----- Return performance in CPI metric ----- */

/**
 * @brief calculate the preformance in CPI metric
 * 		  for Blocked cpi
 * @return the cycles per instruction it took
*/
double CORE_BlockedMT_CPI();

/**
 * @brief calculate the preformance in CPI metric
 * 		  for fine grained cpi
 * @return the cycles per instruction it took
*/
double CORE_FinegrainedMT_CPI();


void threads_init(int thread_num);

void threads_destroy(int thread_num);

void do_op(int thread_id , Instruction *inst);

int get_next_thread2(int curr_thread);

bool all_busy();

void update_busy();

bool is_busy(int thread_id);

int get_next_thread(int curr_thread);

bool isRunning();




#ifdef __cplusplus
}
#endif

#endif /* CORE_API_H_ */
