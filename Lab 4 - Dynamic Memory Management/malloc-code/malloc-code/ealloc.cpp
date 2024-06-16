#include "ealloc.h"
#include <map>
#include <iostream>

using namespace std;

#define NUMPAGES 4
#define NUMBLOCKS PAGESIZE/MINALLOC


char* addr[NUMPAGES];
int bitmap[NUMPAGES][NUMBLOCKS];
map<unsigned long, pair<int,int>> ptr_to_size;
int last_active_page = -1;

void init_alloc(){

    for(int i=0; i<NUMPAGES; i++){
        addr[i] = NULL;
    }

    //initialize the bitmap
    for(int i = 0; i< NUMPAGES; i++){
        for(int j = 0; j<NUMBLOCKS; j++){
            bitmap[i][j] = 1;
        }
    }

    return;
}

void cleanup(){
    ptr_to_size.clear();

    for(int i =0; i<NUMPAGES; i++){
        for(int j = 0; j<NUMBLOCKS; j++){
            bitmap[i][j] = 1;
        }
    }

    return;
}

char* alloc(int n){
    
    if(n % MINALLOC != 0){
        return NULL;
    }
    int blocks = n/MINALLOC;

    // edge case where initially there is no defined page
    if(last_active_page == -1){
        addr[0] = (char*)mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if(addr[0] == MAP_FAILED){
            printf("Failed to map!\n");
            return NULL;
        }
        last_active_page++;

        // we block the spots in the bitmap
        for(int i=0; i<blocks; i++){
            bitmap[0][i] = 0;
        }

        // we fill the endpoint in the map
        ptr_to_size[(unsigned long) addr[0]] = make_pair(0,blocks);
        // cout<<"allocated successfully base case"<<endl;
        return addr[0];
    }

    // first we will check in all the active pages if there is space, if there is space we allocate memory
    // if there is no space we add a new page. If this would exceed the max number of pages then we return NULL

    for(int page = 0; page <= last_active_page; page++){
        //we check if there is space in the last active page
        for(int i = 0; i < NUMBLOCKS; i++){
            // if there is free space we check if there is a long enough empty space to store the word
            // cout<<"base address of page "<<page<<" "<<(unsigned long)addr[page]<<endl;
            if(bitmap[page][i] == 1){
                // cout<<"hello page "<<page<<" block "<<i<<endl;
                int j = 0;
                int flag = 0;
                for(j = i; j < i+blocks && j < NUMBLOCKS; j++){
                    if(j >= NUMBLOCKS){
                        j++;
                        continue;
                    }
                    if(bitmap[page][j] == 0){
                        cout<<"position "<<(unsigned long)addr[page]+j<<" is occupied!"<<endl;
                        flag = -1;
                        break;
                    }
                }
                if(flag == -1){ //invalid memory segment
                    i = j+1;
                    continue;
                }
                else{ //free and valid memory segment
                    char* ptr = (addr[page]+i*MINALLOC);
                    // printf("Memory location found in page at %p!\n",ptr);
                    for(j = i; j < i+blocks; j++){
                        bitmap[page][j] = 0;
                    }
                    ptr_to_size[(unsigned long)ptr] = make_pair(page,blocks);
                    // cout<<"allocated successfully in here at page "<<page<<" and block "<<i<<endl;
                    return ptr;
                }
            } 
        }
    }

    //if we reach here we need a new page. We will allocate space at the very start of the page
    if(last_active_page == 3){
        cout<<"Cannot allocate more pages!"<<endl;
        return NULL;
    }   
    else{
        last_active_page++;
        addr[last_active_page] = (char*)mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if(addr == MAP_FAILED){
            cout<<"Failed to map!\n";
            return NULL;
        }

        // we block the spots in the bitmap
        for(int i=0; i<blocks; i++){
            bitmap[last_active_page][i] = 0;
        }

        // we fill the endpoint in the map
        ptr_to_size[(unsigned long) addr[last_active_page]] = make_pair(last_active_page,blocks);
        //cout<<"allocated successfully outside"<<endl;
        return addr[last_active_page];
    }
    
}

void dealloc(char* ptr){
    // if the pointer is not the base pointer of any memory block
    if(ptr_to_size.count((unsigned long)ptr) == 0){
        return;
    }
    
    pair<int,int> page_blocksize = ptr_to_size[(unsigned long)ptr];

    int ptr_block_index = ((unsigned long)(ptr - addr[page_blocksize.first])) / MINALLOC;
    for(int i = ptr_block_index; i < ptr_block_index + page_blocksize.second; i++){
        // cout<<"page :"<<page_blocksize.first<<" page base addr :"<<(unsigned long) addr[page_blocksize.first]<<" pointer :"<< (unsigned long) ptr<<" number of blocks : "<<page_blocksize.second<<" offset : "<<ptr_block_index<<" deallocated "<<endl;
        bitmap[page_blocksize.first][i] = 1;
    }
    ptr_to_size.erase((unsigned long)ptr);
    // cout<<"deallocated "<<page_blocksize.first<<" base ptr "<<(unsigned long)ptr<<" size "<<page_blocksize.second<<endl;
    memset(ptr, 0, page_blocksize.second * MINALLOC);
}