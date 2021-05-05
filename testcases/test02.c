#include <stdio.h>
#include <libuser.h>
#include <usloss.h>
#include <phase4.h>


int start4(char *arg)
{
   int unit, sectorSize, trackSize, diskSize;

   printf("start4(): started\n");

   unit = 0;
   DiskSize(unit, &sectorSize, &trackSize, &diskSize);

   printf("start4(): unit %d, sector size %d, track size %d, disk size %d\n",
          unit, sectorSize, trackSize, diskSize);

   unit = 1;
   DiskSize(unit, &sectorSize, &trackSize, &diskSize);

   printf("start4(): unit %d, sector size %d, track size %d, disk size %d\n",
          unit, sectorSize, trackSize, diskSize);

   printf("start4(): calling Terminate\n");
   Terminate(0);

   printf("start4(): should not see this message!\n");
   return 0;

} /* start4 */
