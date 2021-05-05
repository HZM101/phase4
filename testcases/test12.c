/* KILLER
 * Spawn 5 children to sleep
 * Spawn 2 children ... 1 to read 1 line from each terminal and one to write one line to each terminal
 * Spawn 4 children .... 
 * 1 to write to disk0 1 to read from disk0 
 * 1 to write to disk1 1 to read from disk1 
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <usloss.h>
#include <libuser.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usyscall.h>


int ChildS(char *arg)
{
   int tod1,tod2;
   int my_num = atoi(arg);
   int seconds;

   seconds = (5 - my_num);

   printf("ChildS(%d): Sleeping for %d seconds\n", my_num, seconds);
   GetTimeofDay(&tod1);
   Sleep(seconds);
   GetTimeofDay(&tod2);
   printf("ChildS(%d): After sleeping %d seconds, diff in sys_clock is %d\n",
          my_num, seconds, tod2-tod1);

  Terminate(10 + my_num);

  return 0;
}



int ChildDW0(char *arg)
{
   int status;
   char disk_buf_A[512];

   printf("\nChildDW0(): writing to disk 0, track 5, sector 0\n");
   sprintf(disk_buf_A, "ChildDW0(): A wonderful message to put on the disk...");
   DiskWrite(disk_buf_A, 0, 5, 0, 1, &status);
   printf("ChildDW0(): DiskWrite0 returned status = %d\n", status);
   return 0;
}

int ChildDW1(char *arg)
{
   int status;
   char disk_buf_A[512];

   printf("\nChildDW1(): writing to disk 1, track 5, sector 0\n");
   sprintf(disk_buf_A, "ChildDW1(): A wonderful message to put on the disk...");
   DiskWrite(disk_buf_A, 1, 5, 0, 1, &status);
   printf("ChildDW1(): DiskWrite1 returned status = %d\n", status);
   return 0;
} /* ChildDW1 */

int ChildDR0(char *arg)
{
   int status;
   char disk_buf_B[512];
   
   printf("\nChildR0(): reading from disk 0, track 5, sector 0\n");
   DiskRead(disk_buf_B, 0, 5, 0, 1, &status);
   printf("ChildR0(): DiskRead returned status = %d\n", status);
   printf("ChildR0(): disk_buf_B contains:\n%s\n", disk_buf_B);
   return 0;
} /* ChildDR0 */

int ChildDR1(char *arg)
{
   int status;
   char disk_buf_B[512];
   
   printf("\nChildR1(): reading from disk 1, track 5, sector 0\n");
   DiskRead(disk_buf_B, 1, 5, 0, 1, &status);
   printf("ChildR1(): DiskRead returned status = %d\n", status);
   printf("ChildR1(): disk_buf_B contains:\n%s\n", disk_buf_B);
   return 0;
} /* ChildDR1 */

int start4(char *arg)
{
   int  pid, status, i;
   char buf[12];
   char name[] = "ChildS";

   console("start4(): Spawning 5 children to sleep\n");
   for (i = 0; i < 5; i++) {
      sprintf(buf, "%d", i);
      name[5] = buf[0];
      status = Spawn(name, ChildS, buf, USLOSS_MIN_STACK,2, &pid);
   }


   console("start4(): Spawning 4 children to diskfuncs\n");
   status = Spawn("ChildDW0", ChildDW0, NULL, USLOSS_MIN_STACK,2, &pid);
   status = Spawn("ChildDW1", ChildDW1, NULL, USLOSS_MIN_STACK,2, &pid);
   status = Spawn("ChildDR0", ChildDR0, NULL, USLOSS_MIN_STACK,4, &pid);
   status = Spawn("ChildDR1", ChildDR1, NULL, USLOSS_MIN_STACK,4, &pid);

   for (i = 0; i < 9; i++) {
      Wait(&pid, &status);
   }

   printf("start4(): done.\n");
   Terminate(1);
   return 0;

} /* start4 */
