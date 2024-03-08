#include <iostream>
#include <cstdint>

typedef class block {
public:
    uint32_t address;
    int tag;
    int Bsize; //in bytes
    int set;
    int counter;
    bool dirty;
    bool valid;
   
    block();
    block(int tag,int Bsize,int set,uint32_t address);
    block(const block& other);
} block_t;

//typedef class cache{} cache_t;
typedef class cache {
public:
    int Bsize; //block size
    int Csize; //cache size
    int Ssize;
    int assoc;
    int MemCyc;
    block_t **table;

    /**/
    cache();
    cache(int BSize, int Csize,int assoc,int MemCyc);
    ~cache();
    int get_set(uint32_t address);
    int get_tag(uint32_t address);
    bool has(uint32_t address);
    void update_LRU_counter(uint32_t address);
    void emptySpaceIfNeeded(uint32_t address,
                            cache *L1,
                            cache *L2,
                            int remove_from);
    block_t pop_highest(int set);
    bool isFull(int set);
    void insert(uint32_t address);
    void dirty(uint32_t address);
    void printTable();
}cache_t;
