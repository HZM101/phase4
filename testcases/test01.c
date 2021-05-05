#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>



int Child(char *arg)
{
   int tod1,tod2;
   int my_num = atoi(arg);
   int seconds;

   seconds = (10 - my_num) * 3;

   printf("Child%d(): Sleeping for %d seconds\n", my_num, seconds);
   GetTimeofDay(&tod1);
   Sleep(seconds);
   GetTimeofDay(&tod2);
   printf("Child%d(): After sleeping %d seconds, diff in sys_clock is %d\n",
           my_num, seconds,tod2-tod1);

  Terminate(10 + my_num);

  return 0;
}


int start4(char *arg)
{
   int i,cpid,id,result;

   char carg[10];
   char name[] = "Child0";

   for(i = 0; i < 10; i++) {
      sprintf(carg, "%d", i);
      name[5] = carg[0];

      console("start4(): Spawning Child(%d)\n", i);
      Spawn(name, Child, carg, USLOSS_MIN_STACK, 3, &cpid);
   }


   for(i=0; i < 10; i++) {
      console("start4(): Waiting on Child\n");
      result = Wait(&cpid, &id);
      console("start4(): Wait returned %d, pid:%d, status %d\n",
              result,cpid,id);
   }

   Terminate(0);

   return 0;
}
