#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(){

    int pid = fork(); //this return's the child's PID
    
    if(pid == 0){
        printf("I am child. My PID is %d.\n",getpid());
        exit(0);
    }else{
        //waitid(pid);  // check why this behaves differently
        wait();
        printf("I am parent of %d. My pid is %d.\n",pid,getpid());
    }

}