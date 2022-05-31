/* create.c - create, newpid */

#include <xinu.h>

extern tss_t global_tss ;
extern void ret_k2u(void) ;
local	int newpid();

/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr = (uint32* ) SYSERR ;		/* Stack address		*/
	uint32      *usersaddr = (uint32* ) SYSERR ;   /* User Stack address */
    // uint32      *tssaddr = SYSERR ;     /* tss address */  

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) || 
		 ((usersaddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR )) {
        if( saddr != (uint32* )SYSERR ) {
            freestk( saddr , ssize ) ;
        }
        if( usersaddr != (uint32*)SYSERR ) {
            freestk( usersaddr , ssize ) ;
        }
		restore(mask);
		return SYSERR;
	}


	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->pruserstkbase = ( char *)usersaddr ;
	prptr->pruserstklen = prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize user stack as if the process was in a interrupt */
	*usersaddr = STACKMAGIC ;
    /* Push arguments */
	a = (uint32 *)( &nargs + 1 ) ; /* Start of args */
	a += nargs - 1 ; /* Last argument */ 
	for( ; nargs > 0 ; nargs -- )  /* copy args */
		*--usersaddr = *a-- ;  /* onto created user process' stack */
	*--usersaddr = (long)INITRET ; /* Push on return address */
	prptr->pruserstkptr = (char *)usersaddr ; 

	/* Initialize kernel stack as if the kernel is returning from a intr */
	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;
	*--saddr = (long)INITRET;	/* Push on return address	*/
	prptr->esp0 = (long)saddr ; /* TSS.esp0 = (long)saddr */
	*--saddr = (long)0x33 ; /* SS at User Stack segment 0000000000110 0 11 */
	*--saddr = (long)usersaddr ; /* ESP at now User Stack pointer */
    *--saddr = (long)0x200 ; /* EFLAGS New process run with interrupts enabled */
    *--saddr = (long)0x23 ; /* CS at User Code segment 0000000000100 0 11 */
    *--saddr = (long)funcaddr ; /* EIP store funcaddr */

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/
	*--saddr = (long)ret_k2u;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0;	/* ?? assume new interrupt runs with */
					/*   interrupts disabled		*/
	/* Basically, the following emulates an x86 "pushal" instruction*/
	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
