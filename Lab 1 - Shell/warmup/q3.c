#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(){

    int pid = fork(); //this return's the child's PID
    
    if(pid == 0){
        printf("I am child. My PID is %d.\n",getpid());
        // char *myargs[2];

        // myargs[0] = "ls";
        // myargs[1] = NULL; 

        // execvp(myargs[0],myargs);

        char *myargs2[3];

        myargs2[0] = "ls";
        myargs2[1] = "-l";
        myargs2[2] = NULL; 

        execvp(myargs2[0],myargs2);

        exit(0);
    }else{
        //waitid(pid);  // check why this behaves differently
        wait();
        printf("I am parent of %d. My pid is %d.\n",pid,getpid());

    }

}