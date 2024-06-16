#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(){

    int pid = fork();
    
    if(pid == 0){
        printf("I am child\n");
        exit(0);
    }else{
        wait();
        printf("I am parent \n");
    }

    //depending on where wait() is located the parent prints before or after child
}