#include <usyscall.h>
#include <libuser.h>
#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>


static char XXbuf[4][512];
int ubiq(int);

int k1(char *arg)
{
    ubiq(0);
    ubiq(3);
    ubiq(0);
    ubiq(5);
    ubiq(0);
    ubiq(5);
    ubiq(0); 
    Terminate(2);
    return 0;
}

int k2(char *arg)
{
    ubiq(5);
    ubiq(4);
    ubiq(1);
    ubiq(6);
    ubiq(1);
    ubiq(6);
    ubiq(1);
    Terminate(3);
    return 0;
}

int k3(char *arg)
{
    ubiq(9);
    ubiq(9);
    ubiq(2);
    ubiq(7);
    ubiq(2);
    ubiq(7);
    ubiq(2);
    Terminate(4);
    return 0;
}

int k4(char *arg)
{
    ubiq(2);
    ubiq(9);
    ubiq(3);
    ubiq(8);
    ubiq(3);
    ubiq(8);
    ubiq(3);
    Terminate(5);
    return 0;
}


int cksum=0;


int ubiq(int t)
{
    int status = -1;
    char buf[50];
    int z = t % 4;
    if (DiskWrite(XXbuf[z], 1, z, z, 1, &status) < 0) {
	printf("ERROR: DiskPut\n");
    } 
    
    if (status != 0) { 
	sprintf(buf,"disk_put returned error   %d\n",t);
	printf(buf);
    }  
    else {
	    cksum+=t;
    }
    return 0;
}

int start4(char *arg)
{
    int j,status, pid;
    
   printf("start4(): disk scheduling test, create 4 processes that write\n");
   printf("          various places on the disk.  After the 4 processes are\n");
   printf("          done, start4 reads the 4 locations & prints the\n");
   printf("          results.  Uses disk 1.\n");

    strcpy(XXbuf[0],"One flew East\n");
    strcpy(XXbuf[1],"One flew West\n");
    strcpy(XXbuf[2],"One flew over the coo-coo's nest\n");
    strcpy(XXbuf[3],"--did it work?\n");
    
    Spawn("k1", k1, NULL, USLOSS_MIN_STACK, 4, &pid);
    Spawn("k2", k2, NULL, USLOSS_MIN_STACK, 4, &pid);
    Spawn("k3", k3, NULL, USLOSS_MIN_STACK, 4, &pid);
    Spawn("k4", k4, NULL, USLOSS_MIN_STACK, 4, &pid);

    Wait(&pid, &status);
    Wait(&pid, &status);
    Wait(&pid, &status);
    Wait(&pid, &status);

    strcpy(XXbuf[0],"xxx");
    strcpy(XXbuf[1],"xxx");
    strcpy(XXbuf[2],"xxx");
    strcpy(XXbuf[3],"xxx");

    for (j = 0; j < 4; j++) {
	status = -1;
	if (DiskRead(XXbuf[j], 1, j, j, 1, &status) < 0) {
	    printf("start4(): ERROR: DiskGet\n");
	}
	if (status < 0)
	  printf("start4(): disk_get returned error");
	printf("start4(): read: %s", XXbuf[j]);
    } 
    
    printf("start4(): done %d\n",cksum);
    Terminate(2);
    return 0;
} /* start4 */
