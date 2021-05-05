#include <stdio.h>
#include <libuser.h>
#include <usloss.h>
#include <phase4.h>


int start4(char *arg)
{
   int unit, sectorSize, trackSize, diskSize;
   char disk_buf_A[512];
   char disk_buf_B[512];
   int status;

   printf("start4(): started\n");

   unit = 0;
   DiskSize(unit, &sectorSize, &trackSize, &diskSize);

   printf("start4(): unit %d, sector size %d, track size %d, disk size %d\n",
          unit, sectorSize, trackSize, diskSize);

   unit = 1;
   DiskSize(unit, &sectorSize, &trackSize, &diskSize);

   printf("start4(): unit %d, sector size %d, track size %d, disk size %d\n",
          unit, sectorSize, trackSize, diskSize);

   printf("\nstart4(): writing to disk 0, track 5, sector 0\n");
   sprintf(disk_buf_A, "A wonderful message to put on the disk...");
   DiskWrite(disk_buf_A, 0, 5, 0, 1, &status);
   printf("start4(): DiskWrite returned status = %d\n", status);

   printf("\nstart4(): reading from disk 0, track 5, sector 0\n");
   DiskRead(disk_buf_B, 0, 5, 0, 1, &status);
   printf("start4(): DiskRead returned status = %d\n", status);
   printf("start4(): disk_buf_B contains:\n%s\n", disk_buf_B);
   
   printf("start4(): calling Terminate\n");
   Terminate(0);

   printf("start4(): should not see this message!\n");
   return 0;

} /* start4 */
