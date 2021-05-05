/*
 * Test Invalid disk and Terminal.
 *

Expected o/p:

All processes completed.


 * 
 */
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>



int start4(void *arg)
{
   int  status;
   char buffer[80];

   printf("start4(): Attempt to write to a non-existant disk, disk 3\n");

   if (DiskWrite(buffer, 3, 1, 1, 1, &status) >= 0) {
      printf("start4(): Disk : Should not see this!!!\n");
   }

   if (DiskWrite(buffer, 0, 17, 1, 1, &status) >= 0) {
      printf("start4(): Disk : Should not see this!!!\n");
   }

   if (DiskWrite(buffer, 0, 1, 17, 1, &status) >= 0) {
      printf("start4(): Disk : Should not see this!!!\n");
   }

   printf("start4(): done\n");

   Terminate(8);
   return 0;

} /* start4 */
