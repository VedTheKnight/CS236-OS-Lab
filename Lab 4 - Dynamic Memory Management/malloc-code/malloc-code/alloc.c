#include "alloc.h"

char *addr;
int image[PAGESIZE] = {0};
int end_point_tracker[PAGESIZE] = {0};

int init_alloc(){

    addr = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if(addr == MAP_FAILED){
        printf("Failed to map!\n");
        return 1;
    }

    printf("Memory mapped at address : %p\n",addr);
    return 0;
}

int cleanup(){
    int f = munmap(addr, PAGESIZE);
    if(f == 0){
        printf("Unmapped memory\n");
        return 0;
    }
    else{
        printf("Unmap failed\n");
        return 1;
    }
}

char* alloc(int n){

    if(n%MINALLOC != 0){
        return NULL;
    }

    for(int i = 0; i<PAGESIZE; i++){
        if(image[i] == 1){
            continue;
        }
        else{
            int j = 0;
            for(j = i; j < i+n; j++){
                if(image[j] == 1){
                    j = -1;
                    break;
                }
            }
            if(j == -1){ //invalid memory segment
                i = i+n;
                continue;
            }
            else{ //free and valid memory segment
                void* ptr = (addr+i);
                printf("Memory location found in page at %p!\n",ptr);
                for(j = i; j < i+n; j++){
                    image[j] = 1;
                }
                end_point_tracker[i] = i+n-1;
                return ptr;
            }
        }
    }

    //if we reach here that means that there is no space available
    printf("Not enough memory!\n");
    return NULL;
}

void dealloc(char *ptr){
    for(int i=0; i<PAGESIZE; i++){
        if(addr+i == ptr){
            // we dealloc the memory
            int end_point = end_point_tracker[i];
            for(int j = i; j<=end_point; j++){
                *(addr + j) = 0;
                image[j] = 0;
            }
            end_point_tracker[i] = 0; // we reset this since the memory is freed
            printf("Memory freed\n");
            return;
        }
    }

    //if we reach here that means the address passed is invalid
    printf("Invalid address passed!\n");
    return;
}