/* ------------------------------------------------------------------------
   phase4.c

   University of Arizona South
   Computer Science 452

   @authors: Erik Ibarra Hurtado, Hassan Martinez, Victor Alvarez

   ------------------------------------------------------------------------ */

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <libuser.h>
#include <provided_prototypes.h>
#include "driver.h"

/* ------------------------- Prototypes ----------------------------------- */

int start3 (char *); 
static int	ClockDriver(char *);
static int	DiskDriver(char *);
static void sleep(sysargs *);
static void disk_read(sysargs *);
static void disk_write(sysargs *);
static void disk_size(sysargs *);
int insert_sleep_q(driver_proc_ptr);
int remove_sleep_q(driver_proc_ptr);
int insert_disk_q(driver_proc_ptr);
int remove_disk_q(driver_proc_ptr);
void check_kernel_mode(char *);
void bug_flag(char *, bool, int);

/* -------------------------- Globals ------------------------------------- */

static int debugflag4 = 0;

static int running; /*semaphore to synchronize drivers and start3*/

static struct driver_proc Driver_Table[MAXPROC];

struct driver_proc empty_proc = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

driver_proc_ptr SleepQ = NULL;
int sleep_number = 0;

driver_proc_ptr DQ = NULL;
int DQ_number = 0;

bool exit_logic = false;
static int diskpids[DISK_UNITS];
static int num_tracks[DISK_UNITS];
static int sleep_semaphore;
static int disk_semaphore;
static int DQ_semaphore;
static int driver_sync_semaphore;

/* -------------------------- Functions ----------------------------------- */


/* start3 */
int start3(char *arg)
{
    char	name[128];
    char    termbuf[10];
    int		i;
    int		clockPID;
    int		pid;
    int		status;

    /* Check kernel mode here */

    check_kernel_mode("start3(): Not in kernel mode");

    /* Assignment system call handlers */
    
    bug_flag("start3(): sys_vec assignment.", false, 0);

    sys_vec[SYS_SLEEP] = sleep;
    sys_vec[SYS_DISKREAD] = disk_read;
    sys_vec[SYS_DISKWRITE] = disk_write;
    sys_vec[SYS_DISKSIZE] = disk_size;
    
    /* Initialize the phase 4 process table */

    bug_flag("start3(): process table.", false, 0);

    for (int i = 0; i < MAXPROC; i++)
    {
        Driver_Table[i] = empty_proc;
        Driver_Table[i].private_sem = semcreate_real(0);
    }

    /*
     * Create clock device driver 
     * I am assuming a semaphore here for coordination. A mailbox can
     * be used insted -- your choice.
     */

    bug_flag("start3(): create semaphores.", false, 0);

    sleep_semaphore = semcreate_real(1);
    disk_semaphore = semcreate_real(0);
    DQ_semaphore = semcreate_real(1);
    running = semcreate_real(0);
    driver_sync_semaphore = semcreate_real(1);


    bug_flag("start3(): fork clock driver.", false, 0);

    clockPID = fork1("Clock driver", ClockDriver, NULL, USLOSS_MIN_STACK, 2);
    if (clockPID < 0) 
    {
	    console("start3(): Can't create clock driver\n");
	    halt(1);
    }

    /*
     * Wait for the clock driver to start. The idea is that ClockDriver
     * will V the semaphore "running" once it is running.
     */

    bug_flag("start3(): semp on running sem.", false, 0);
    semp_real(running);

    /*
     * Create the disk device drivers here.  You may need to increase
     * the stack size depending on the complexity of your
     * driver, and perhaps do something with the pid returned.
     */

    bug_flag("start3(): fork drisk drivers.", false, 0);
    for (i = 0; i < DISK_UNITS; i++) 
    {
        sprintf(termbuf, "%d", i); 
        sprintf(name, "DiskDriver%d", i);
        diskpids[i] = fork1(name, DiskDriver, termbuf, USLOSS_MIN_STACK, 2);
        if (diskpids[i] < 0) 
        {
           console("start3(): Can't create disk driver %d\n", i);
           halt(1);
        }
    }

    bug_flag("start3(): semp on running sem x2.", false, 0);

    semp_real(running);
    semp_real(running);

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case names.
     */

    bug_flag("start3(): spawn start4 and wait on it.", false, 0);
    
    pid = spawn_real("start4", start4, NULL,  8 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    /* Zap the device drivers */

    bug_flag("start3(): zap and join clock driver.", false, 0);

    zap(clockPID);  // clock driver
    join(&status); /* for the Clock Driver */

    bug_flag("start3(): 'zap' and join disk drivers", false, 0);

    for (i = 0; i < DISK_UNITS; i++)
    {
        
        exit_logic = true;

        bug_flag("start3(): semv disk_semaphore", false, 0);
        semv_real(disk_semaphore);

        bug_flag("start3(): join", false, 0);
        join(&status);
    }

    /* quit state to join next. */
    quit(0); 
    return 0;
} /* start3 */


/* ClockDriver */
static int ClockDriver(char *arg)
{
    int result;
    int status;
    int current_time;
    driver_proc_ptr proc_ptr, proc_to_wake;

    /*
     * Let the parent know we are running and enable interrupts.
     */

    bug_flag("ClockDriver(): semv on running semaphore.", false, 0);

    semv_real(running);
    psr_set(psr_get() | PSR_CURRENT_INT);
    while(! is_zapped()) 
    {
	    result = waitdevice(CLOCK_DEV, 0, &status);
	    if (result != 0) 
        {
	        return 0;
	    }
	    /*
	    * Compute the current time and wake up any processes
	    * whose time has come.
	    */
        current_time = sys_clock();
        proc_ptr = SleepQ;

        while (proc_ptr != NULL)
        {
            proc_to_wake = proc_ptr;
            proc_ptr = proc_ptr->next_ptr;

            /* Check current time to wake time. */
            if (current_time >= proc_to_wake->wake_time)
            { 
                bug_flag("ClockDriver(): removing proc from SleepQ.", false, 0);
                sleep_number = remove_sleep_q(proc_to_wake);

                bug_flag("ClockDriver(): waking proc.", false, 0);
                semv_real(proc_to_wake->private_sem);    
            } 
        }
    }

    /* quit state to join next. */
    quit(0);
} /* ClockDriver */


/* DiskDriver */
static int DiskDriver(char *arg)
{
    int unit = atoi(arg);
    device_request my_request;
    int result;
    int status;
    int counter;
    int current_track;
    int current_sector;

    bug_flag("DiskDriver(): semv on running semaphore.", true, unit);
    psr_set(psr_get() | PSR_CURRENT_INT);

    driver_proc_ptr current_req;

    bug_flag("DiskDriver(): started.", true, unit);

    /* Get the number of tracks for this disk */

    bug_flag("DiskDriver(): getting # of tracks for disk.", true, unit);

    my_request.opr  = DISK_TRACKS;
    my_request.reg1 = &num_tracks[unit];

    result = device_output(DISK_DEV, unit, &my_request);

    if (result != DEV_OK) 
    {
        console("DiskDriver (%d): did not get DEV_OK on DISK_TRACKS call\n", unit);
        console("DiskDriver (%d): is the file disk%d present???\n", unit, unit);
        halt(1);
    }

    waitdevice(DISK_DEV, unit, &status);
    if (DEBUG4 && debugflag4)
    {
        console("DiskDriver(%d): tracks = %d\n", unit, num_tracks[unit]);
    }

    /* start3 driver running. */ 
    semv_real(running);

    bug_flag("DiskDriver(): enter while loop till zapped.", true, unit);

    while (exit_logic != true)
    {

        /* Wait request from user. */
        bug_flag("DiskDriver(): semp on disk semaphore to wait for requests.", true, unit);
        semp_real(disk_semaphore);

        /* Set next request for DiskQ. */
        bug_flag("DiskDriver(): setting current_req = DQ.", true, unit);
        current_req = DQ;
            
        if (current_req != NULL)
        {
            /* Check disk_size request. */
            bug_flag("DiskDriver(): checking current_req operation type.", true, unit);
            if (current_req->operation == DISK_SIZE)
            {
                /* Critical region in. */
                bug_flag("DiskDriver(): size - semP on dq_sem to enter critical region.", true, unit);
                semp_real(DQ_semaphore);

                DQ_number = remove_disk_q(current_req);
            
                /* Critical region out. */
                bug_flag("DiskDriver(): size - semV on DQ_sem to leave critical region.", true, unit);
                semv_real(DQ_semaphore);

                /* Wake up user. */
                bug_flag("DiskDriver(): size - semV on private_sem to unblock proc.", true, unit);
                semv_real(current_req->private_sem);
            }
            else
            {
                /* Critical region in. */
                bug_flag("DiskDriver(): rw - semp on dq_sem to enter critical region.", true, unit);
                semp_real(DQ_semaphore);

                DQ_number = remove_disk_q(current_req); 

                /* Critical region out. */
                bug_flag("DiskDriver(): rw - semV on DQ_sem to leave critical region.", true, unit);
                semv_real(DQ_semaphore);

                /* Critical region in for disk operations. */
                bug_flag("DiskDriver(): rw - semp on driver_sync sem enter region.", true, unit);
                semp_real(driver_sync_semaphore);

                /* Adjust disk_seek. */
                my_request.opr  = DISK_SEEK;
                my_request.reg1 = current_req->track_start;
                result = device_output(DISK_DEV, current_req->unit, &my_request);

                if (result != DEV_OK) 
                {
                    console("DiskDriver (%d): did not get DEV_OK on 1st DISK_SEEK call\n", unit);
                    console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                    halt(1);
                }

                waitdevice(DISK_DEV, current_req->unit, &status);

                /* Set read/write loop variables. */
                current_track = current_req->track_start; 
                current_sector = current_req->sector_start; 
                counter = 0; 

                /* read/write operations. */
                while (counter != current_req->num_sectors)
                {
                    my_request.opr = current_req->operation;
                    my_request.reg1 = current_sector;
                    my_request.reg2 = &current_req->disk_buf[counter * DISK_SECTOR_SIZE];
                    result = device_output(DISK_DEV, current_req->unit, &my_request);

                    if (result != DEV_OK) 
                    {
                        console("DiskDriver (%d): did not get DEV_OK on %d call\n", unit, current_req->operation);
                        console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                        halt(1);
                    }

                    waitdevice(DISK_DEV, current_req->unit, &status);
                    current_req->status = status;

                    /* Check to finish reading/writing. */
                    counter++;
                    if (counter != current_req->num_sectors)
                    {

                        /* Update sector. */
                        current_sector++;

                        /* Test for sector's out of bound. */
                        if (current_sector >= DISK_TRACK_SIZE) 
                        {
                            current_track = (current_track + 1) % num_tracks[unit];
                            current_sector = 0;

                            my_request.opr  = DISK_SEEK;
                            my_request.reg1 = current_track;
                            result = device_output(DISK_DEV, current_req->unit, &my_request);

                            if (result != DEV_OK) 
                            {
                                console("DiskDriver (%d): did not get DEV_OK on DISK_SEEK call\n", unit);
                                console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                                halt(1);
                            }

                            waitdevice(DISK_DEV, current_req->unit, &status);
                        }
                    }
                }

                /* Critical region out. */
                bug_flag("DiskDriver(): rw - semv on driver_sync sem leave region.", true, unit);
                semv_real(driver_sync_semaphore);
            }
            /* Wake up user. */     
            bug_flag("DiskDriver(): rw - semv on private_sem to unblock proc.", true, unit);
            semv_real(current_req->private_sem);
        }    
    }

    /* quit state to joing next. */
    quit(0);
} /* DiskDriver */


/* sleep */
static void sleep(sysargs *args_ptr)
{
    /* Temps. */
    int seconds;
    int result;
    driver_proc_ptr current_proc;
    current_proc = &Driver_Table[getpid() % MAXPROC];

    /* Set value. */
    seconds = (int) args_ptr->arg1;

    /* Validating seconds can't have negative time. If so return.*/
    if (seconds < 0)
    {
        result = -1;
        args_ptr->arg4 = (void *) result;
        return;
    }

    /* Enter critical region. */
    bug_flag("sleep(): call semp on sleep_sem.", false, 0);
    semp_real(sleep_semaphore);

    bug_flag("sleep(): call insert_sleep_q.", false, 0);
    sleep_number = insert_sleep_q(current_proc);

    /* Save time for sleep. */
    current_proc->wake_time = sys_clock();
    current_proc->wake_time = (seconds * 1000000) + current_proc->wake_time;

    bug_flag("sleep(): call semv on sleep_sem.", false, 0);
    semv_real(sleep_semaphore);

    bug_flag("sleep(): call semp on proccesse private sem to block it.", false, 0);
    semp_real(current_proc->private_sem);

    /* Set result arg4 to 0. */
    result = 0;
    args_ptr->arg4 = (void *) result;

    return;
} /* sleep */


/* disk_read */
static void disk_read(sysargs *args_ptr)
{
    /* Temps. */
    int unit;
    int track;
    int first;
    int sectors;
    void *buffer;
    int status;
    int result = 0;
    driver_proc_ptr current_proc;
    current_proc = &Driver_Table[getpid() % MAXPROC];

    buffer = args_ptr->arg1;
    sectors = (int) args_ptr->arg2;
    track = (int) args_ptr->arg3;
    first = (int) args_ptr->arg4;
    unit = (int) args_ptr->arg5;

    /* Testing arguments. */
    if (unit < 0 || unit > 1)                   //disk unit #
    {
        result = -1;
    }
    if (sectors < 0)                            //# of sectors to read
    {
        result = -1;
    }
    if (track < 0 || track >= num_tracks[unit]) //starting track #
    {
        result = -1;
    }
    if (first < 0 || first > 15)                //starting sector #
    {
        result = -1;
    }

    /* Testing for error value. */
    if (result == -1)
    {
        bug_flag("disk_read(): error value return. ", false, 0);
        args_ptr->arg1 = (void *) result;
        args_ptr->arg4 = (void *) result;
        return;
    }

    /* Critical region in. */
    bug_flag("disk_read(): call semP on DQ_semaphore.", false, 0);
    semp_real(DQ_semaphore);

    /* Drive request. */
    current_proc->operation = DISK_READ;
    current_proc->unit = unit;              // which disk unit in read
    current_proc->track_start = track;      // starting track
    current_proc->sector_start = first;     // starting sector
    current_proc->num_sectors = sectors;    // # sectors
    current_proc->disk_buf = buffer;        // buffer

    /* Insert DQ. */
    DQ_number = insert_disk_q(current_proc);

    /* Critical region out. */
    bug_flag("disk_read(): call semV on DQ_sem.", false, 0);
    semv_real(DQ_semaphore);

    /* Send Disk Driver an entry in DQ. */
    semv_real(disk_semaphore);

    /* Wait Disk Driver to complete operations. */
    bug_flag("disk_read(): call semp on proc's private sem to block it.", false, 0);
    semp_real(current_proc->private_sem);
    
    /* Set arg's values. */
    status = current_proc->status;
    args_ptr->arg1 = (void *) status;
    args_ptr->arg4 = (void *) result;
    return;
} /* disk_read */


/* disk_write */
static void disk_write(sysargs *args_ptr)
{
    /* Temps. */
    int unit;
    int track;
    int first;
    int sectors;
    void *buffer;
    int status; 
    int result = 0;
    int flag;
    driver_proc_ptr current_proc;
    current_proc = &Driver_Table[getpid() % MAXPROC];

    buffer = args_ptr->arg1;
    sectors = (int) args_ptr->arg2;
    track = (int) args_ptr->arg3;
    first = (int) args_ptr->arg4;
    unit = (int) args_ptr->arg5;

    /* Testing arguments. */
    if (unit < 0 || unit > 1)                   //disk unit #
    {
        result = -1;
        flag = 0;
    }
    if (sectors < 0)                            //# of sectors to read
    {
        result = -1;
        flag = 1;
    }
    if (track < 0 || track >= num_tracks[unit]) //starting track #
    {
        result = -1;
        flag = 2;
    }
    if (first < 0 || first > 15)                //starting sector #
    {
        result = -1;
        flag = 3;
    }

    /* Testing for error value. */
    if (result == -1)
    {
        bug_flag("disk_write(): error (illegal value) = ", true, flag);
        args_ptr->arg1 = (void *) result;
        args_ptr->arg4 = (void *) result;
        return;
    }

    /* Critical region in. */
    bug_flag("disk_write(): call semp on DQ_sem.", false, 0);
    semp_real(DQ_semaphore);

    /* Drive request. */
    current_proc->operation = DISK_WRITE;
    current_proc->unit = unit;              // which disk unit in write
    current_proc->track_start = track;      // starting track
    current_proc->sector_start = first;     // starting sector
    current_proc->num_sectors = sectors;    // # of sectors
    current_proc->disk_buf = buffer;        // buffer

    /* Insert DQ. */
    DQ_number = insert_disk_q(current_proc);

    /* Critical region out. */
    bug_flag("disk_write(): call semV on DQ_sem.", false, 0);
    semv_real(DQ_semaphore);

    /* Send Disk Driver an entry in DQ. */
    semv_real(disk_semaphore);

    /* Wait Disk Driver to complete operations. */
    bug_flag("disk_write(): call semp on proc's private sem to block it.", false, 0);
    semp_real(current_proc->private_sem);

    /* Set arg's values. */
    status = current_proc->status;
    args_ptr->arg1 = (void *) status; 
    args_ptr->arg4 = (void *) result;
    return;
} /* disk_write */


/* disk_size */
static void disk_size(sysargs *args_ptr)
{
    /* Temps. */
    int unit;
    int sector;
    int track;
    int disk;
    int result;
    driver_proc_ptr current_proc;
    current_proc = &Driver_Table[getpid() % MAXPROC];
    
    /* Set value. */
    unit = (int) args_ptr->arg1;

    /* Testing input value. */
    if (unit < 0 || unit > 1)
    {
        result = -1;
        console("disk_size(): illegal value, unit < 0 or > 1.\n");
        args_ptr->arg4 = (void *) result;
        return;
    }

    /* Critical region in. */
    bug_flag("disk_size_real(): call semP on DQ_sem.", false, 0);
    semp_real(DQ_semaphore);

    current_proc->operation = DISK_SIZE;
    DQ_number = insert_disk_q(current_proc);

    /* Critical region out. */
    bug_flag("disk_size_real(): call semV on DQ_sem.", false, 0);
    semv_real(DQ_semaphore);

    /* Send Disk Driver an entry in DQ. */
    semv_real(disk_semaphore);

    /* Wait Disk Driver to complete operations. */
    bug_flag("disk_size_real(): call semp on proc's private sem to block it.", false, 0);
    semp_real(current_proc->private_sem);

    /* Set arg's values. */
    sector = DISK_SECTOR_SIZE;
    track = DISK_TRACK_SIZE;
    disk = num_tracks[unit];
    result = 0;
    args_ptr->arg1 = (void *) sector;
    args_ptr->arg2 = (void *) track;
    args_ptr->arg3 = (void *) disk;
    args_ptr->arg4 = (void *) result;
    return;
} /* disk_size */


/* insert_sleep_q */
int insert_sleep_q(driver_proc_ptr proc_ptr)
{
    int num_sleep_procs = 0;
    driver_proc_ptr walker;
    walker = SleepQ;

    if (walker == NULL) 
    {
        /* process goes at front of SleepQ */
        bug_flag("insert_sleep_q(): SleepQ was empty, now has 1 entry.", false, 0);
        SleepQ = proc_ptr;
        num_sleep_procs++;
    }
    else 
    {
        bug_flag("insert_sleep_q(): SleepQ wasn't empty, should have > 1.", false, 0);
        num_sleep_procs++; //starts at 1
        while (walker->next_ptr != NULL) //counts how many are in Q already
        {
            num_sleep_procs++;
            walker = walker->next_ptr;
        }
        walker->next_ptr = proc_ptr; //inserts proc to end of Q
        num_sleep_procs++; //counts the insert
    }

    return num_sleep_procs;
} /* insert_sleep_q */


/* remove_sleep_q */
int remove_sleep_q(driver_proc_ptr proc_ptr)
{
    int num_sleep_procs = sleep_number;
    driver_proc_ptr walker, previous;
    walker = SleepQ;

    /* Critical region in. */
    bug_flag("remove_sleep_q(): semp on sleep_sem.", false, 0);
    semp_real(sleep_semaphore);

    //if SleepQ is empty
    if(num_sleep_procs == 0)
    {
        //console("remove_sleep_q(): SleepQ empty. Return.\n");
    }
    
    //elseif SleepQ has one entry
    else if (num_sleep_procs == 1)
    {
        bug_flag("remove_sleep_q(): SleepQ had 1 entry, now 0.", false, 0);
        SleepQ = NULL;
        num_sleep_procs--;
    }
    
    //else SleepQ has > 1 entry
    else
    {
        bug_flag("remove_sleep_q(): SleepQ had > 1 entry.", false, 0);
        if (SleepQ == proc_ptr) //1st entry to be removed
        {
            SleepQ = walker->next_ptr;
            proc_ptr->next_ptr = NULL;
            num_sleep_procs--;
        }
        else //2nd entry or later to be removed
        {
            while (walker != proc_ptr)
            {
                previous = walker;
                walker = walker->next_ptr;
            }
            
            previous->next_ptr = walker->next_ptr;
            walker->next_ptr = NULL;
            num_sleep_procs--;
        }   
    }

    /* Critical region out. */
    bug_flag("remove_sleep_q(): semv on sleep_sem.", false, 0);
    semv_real(sleep_semaphore);

    return num_sleep_procs;
} /* remove_sleep_q */


/* insert_disk_q */
int insert_disk_q(driver_proc_ptr proc_ptr)
{
    int num_disk_procs = 0;
    driver_proc_ptr walker;
    walker = DQ;

    if (walker == NULL) 
    {
        /* process goes at front of DQ */
        bug_flag("insert_disk_q(): DQ was empty, now has 1 entry.", false, 0);
        DQ = proc_ptr;
        num_disk_procs++;
    }
    else 
    {
        bug_flag("inset_disk_q(): DQ wasn't empty, should have > 1.", false, 0);
        num_disk_procs++; //starts at 1
        while (walker->next_dq_ptr != NULL) //counts how many are in Q already
        {
            num_disk_procs++;
            walker = walker->next_dq_ptr;
        }
        walker->next_dq_ptr = proc_ptr; //inserts proc to end of Q
        num_disk_procs++; //counts the insert
    }

    return num_disk_procs;
} /* insert_disk_q */


/* remove_disk_q */
int remove_disk_q(driver_proc_ptr proc_ptr)
{
    int num_disk_procs = DQ_number;
    driver_proc_ptr walker, previous;
    walker = DQ;
    
    //if DQ has one entry
    if (num_disk_procs == 1)
    {
        bug_flag("remove_disk_q(): DQ had 1 entry, now 0.", false, 0);
        DQ = NULL;
        num_disk_procs--;
    }
    else
    {
        bug_flag("remove_disk_q(): DQ had > 1 entry.", false, 0);
        DQ = walker->next_dq_ptr;
        proc_ptr->next_dq_ptr = NULL;
        num_disk_procs--;
    }

    return num_disk_procs;
} /* remove_disk_q */


/* checkKernelMode */
void check_kernel_mode(char * displaymsn) 
{
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        console("%s (%d)\n", displaymsn, getpid());
        halt(1);
    }
} /* checkKernelMode */


/* bug_flag */
void bug_flag(char * displaymsn, bool valuelogic, int value )
{

    if(valuelogic == false)
    {
        if (DEBUG4 && debugflag4) {
            console("%s\n", displaymsn);
        }
    }

    if(valuelogic == true)
    {
        if (DEBUG4 && debugflag4) {
            console("%s (%d)\n", displaymsn, value);
        }
    }

} /* bug_flag */