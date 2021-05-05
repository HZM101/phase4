#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>


static char XXbuf[4][512];
int ubiq0(int);
int ubiq1(int);

int k1(char *arg)
{
    ubiq0(5);
    Terminate(2);
    return 0;
}

int k2(char *arg)
{
    ubiq0(3);
    Terminate(3);
    return 0;
}

int k3(char *arg)
{
    ubiq0(9);
    Terminate(4);
    return 0;
}

int k4(char *arg)
{
    ubiq1(4);
    Terminate(5);
    return 0;
}

int k5(char *arg)
{
    ubiq1(2);
    Terminate(6);
    return 0;
}

int k6(char *arg)
{
    ubiq1(8);
    Terminate(7);
    return 0;
}

int cksum=0;

int ubiq0(int t)
{
    int status = -1;
    char buf[50];
    int z = t % 4;
    console("ubiq0: going to write to track %d\n",t);
    if (DiskWrite(XXbuf[z], 0, t, 4, 1, &status) < 0) {
	printf("ERROR: DiskPut\n");
    } 
    
    if (status != 0) { 
	sprintf(buf,"disk_put returned error   %d\n",t);
	printf(buf);
    }  
    else {
            console("ubiq0: after writing to track %d\n",t);
	    cksum+=t;
    }
    return 0;
}

int ubiq1(int t)
{
    int status = -1;
    char buf[50];
    int z = t % 4;

    console("ubiq1: going to write to track %d\n",t);
    if (DiskWrite(XXbuf[z], 1, t, 4, 1, &status) < 0) {
	printf("ERROR: DiskPut\n");
    } 
    
    if (status != 0) { 
	sprintf(buf,"disk_put returned error   %d\n",t);
	printf(buf);
    }  
    else {
            console("ubiq1: after writing to track %d\n",t);
	    cksum+=t;
    }
    return 0;
}

int start4(char *arg)
{
    int status, pid;
    
   printf("start4(): disk scheduling test, create 6 processes that write\n");
   printf("          3 to disk0         \n");
   printf("          3 to disk1         \n");

    strcpy(XXbuf[0],"One flew East\n");
    strcpy(XXbuf[1],"One flew West\n");
    strcpy(XXbuf[2],"One flew over the coo-coo's nest\n");
    strcpy(XXbuf[3],"--did it work?\n");
    
    Spawn("k1", k1, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k2", k2, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k3", k3, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k4", k4, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k5", k5, NULL, USLOSS_MIN_STACK, 1, &pid);
    Spawn("k6", k6, NULL, USLOSS_MIN_STACK, 1, &pid);

    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);
    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);
    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);
    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);
    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);
    Wait(&pid, &status);
    printf("process %d quit with status %d\n", pid, status);

    printf("start4(): done %d\n",cksum);
    Terminate(2);
    return 0;
} /* start4 */
