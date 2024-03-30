#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <bits/stdc++.h>
#include "cache.hpp"

#define EMPTY -1
#define FAIL -1
using namespace std;
extern bool write_allocate;

block_t::block()
{
    this->address = EMPTY;
    this->tag = EMPTY;
    this->Bsize = 0; //in bytes
    this->set = 0;
    this->counter = EMPTY;
    this->dirty = false;
    this->valid = false;
}

block_t::block(int tag, int Bsize, int set , uint32_t address)
{
    this->address =EMPTY;
    this->tag = tag;
    this->Bsize = Bsize;
    this->set = set;
    this->counter = 0;
    this->dirty = false;
    this->valid = true;
}

block_t::block(const block_t& other)
{
    this->address = other.address;
    this->tag = other.tag;
    this->Bsize = other.Bsize;
    this->set = other.set;
    this->counter = other.counter;
    this->dirty = other.dirty;
    this->valid = other.valid;
}



cache::cache()
{
    this->Bsize = 0;
    this->Csize = 0;
    this->Ssize = 0;
    this->assoc = 0;
    this->MemCyc= 0;

    /*** init the cache table ***/
    this->table = NULL;
}

cache::cache(int BSize, int Csize,int assoc,int MemCyc)
{
    this->Bsize = BSize;
    this->Csize = Csize;
    this->Ssize = Csize - BSize - assoc;
    this->assoc = assoc;
    this->MemCyc= MemCyc;

    /*** init the cache table ***/
    this->table = new block_t*[int(pow(2,Ssize))];
    for (int i = 0 ; i < pow(2,Ssize) ; i++) {
        this->table[i] = new block_t[int(pow(2,assoc))];
    }
}

cache::~cache()
{
    if (this->table != NULL) {
        block_t* p_block = this->table[0]; 
        while (p_block != NULL) {
            block_t* p_block_temp = p_block;
            p_block++;
            delete(p_block_temp);
        }
        delete(this->table);
    }
}

int cache::get_set(uint32_t address)
{
    int Sbits = this->Ssize;
    int Bbits = this->Bsize;
    uint32_t set_mask = (1 << (Sbits + Bbits)) - (1 << (Bbits));
    int set = (address & set_mask) >> (Bbits);
    return set;
}

int cache::get_tag(uint32_t address)
{
    int Sbits = this->Ssize;
    int Bbits = this->Bsize;
    uint32_t tag_mask = 0 - pow(2,(Sbits + Bbits));
    int tag = (address & tag_mask) >> (Bbits + Sbits);
    return tag;
}

bool cache::has(uint32_t address)
{
    int tag = this->get_tag(address);
    int set = this->get_set(address);
    for(int i=0 ; i < pow(2,(this->assoc)); i++)
    {
        if(this->table[set][i].valid && this->table[set][i].tag == tag)
        return true;
    }
    return false;
}

/********************
desc: update the counter in the line of the set
it gets the tag of the block that we read or write from
********************/
void cache::update_LRU_counter(uint32_t address)
{
    int tag = get_tag(address);
    int set = get_set(address);
    if(this->table == NULL || this->table[set] == NULL)
        return;
    for(int i=0; i < pow(2,this->assoc);i++)
    {
        block_t b = this->table[set][i];
        if(b.valid && (b.tag == tag))
            this->table[set][i].counter = 0;
        else
            this->table[set][i].counter++;
    }
}

block_t cache::emptySpaceIfNeeded( uint32_t address,
                                cache_t *L1 , 
                                cache_t *L2, 
                                int remove_from) 
{
    if (remove_from == 1)
    {
        /*remove from L1*/
        int set = L1->get_set(address);
        block_t popped_block;
        if (L1->isFull(set)) {
            popped_block = L1->pop_highest(set);
        }
        return popped_block;
        
    }
    else /*remove from == 2*/
    {
        /*remove from L2*/
        int set = L2->get_set(address);
        block_t popped_block;
        if (L2->isFull(set)) {
            popped_block = L2->pop_highest(set);            
        }
        else {
            return popped_block;
        }
        /*remove from L1 if present*/
        int tag = L1->get_tag(popped_block.address);
        set = L1->get_set(popped_block.address);
        for (int i = 0 ; i < pow(2,L1->assoc) ; i++)
        {
            if (tag == L1->table[set][i].tag) {
                L1->table[set][i] = block();
            }
        }

        return popped_block;
    }
}   

/*********************
desc: return the way number
that need to be deleted
*********************/

void cache::insert(uint32_t address)
{
    int tag = this->get_tag(address);
    int set = this->get_set(address);
    
    for(int i = 0; i < pow(2,this->assoc); i++)
    {
        if(!this->table[set][i].valid)
        {
            table[set][i].address = address;
            table[set][i].tag = tag;
            table[set][i].set = set;
            table[set][i].counter = 0;
            table[set][i].dirty = false;
            table[set][i].valid = true;
            return;
        }
    }
}

void cache::dirty(uint32_t address)
{
    int tag = this->get_tag(address);
    int set = this->get_set(address);
    for(int i = 0; i < pow(2,(this->assoc));i++)
    {
        if(this->table[set][i].valid && this->table[set][i].tag == tag)
            this->table[set][i].dirty = true;
    }
}

void cache::printTable()
{
    if(this->table == NULL)
    {
        cout<< "NO TABLE :( " <<endl;
        return;
    }
    int rows = int(pow(2,this->Ssize));
   
    int columns = int(pow(2,this->assoc));
    for (int i = 0 ; i < rows; i++) {
        cout << "set :" << i << endl;
        if(this->table[i] == NULL)
        {
            cout<< "NO ROW :( " <<endl;
            return;
        }
        for (int j = 0; j < columns; j++)
        {
            if (this->table[i][j].valid)
            {
                cout <<" " <<hex << this->table[i][j].tag << " ";
                cout <<" " <<hex << this->table[i][j].counter<< " ";
                if (this->table[i][j].dirty)
                    cout<< " T ";
                else
                    cout <<" F ";
            }

        }
        cout << endl;
        
    }
    
}

bool cache::isFull(int set)
{
    if(this->table == NULL)
        return true;
    int ways = int(pow(2,this->assoc));
    for(int i = 0;i < ways; i++)
    {
        if(this->table[set][i].valid == false)
            return false;
    }
    return true;
}

block_t cache::pop_highest(int set)
{
    if(this->table == NULL)
    {
        cout << "THE TABLE IS NOT GOOD" << endl;
    }
    int ways = int(pow(2,this->assoc));
    int max = 0;
    int index = 0;
    for(int i = 0; i < ways;i++)
    {
        if(this->table[set][i].counter > max)
        {
            max = this->table[set][i].counter;
            index = i;
        }
    }
    block_t popped = this->table[set][index];
    this->table[set][index] = block();
    return popped;
}