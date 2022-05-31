/*  syscall.s - syscall_initiator , syscall_handler */
        .text 
        .globl syscall_initiator
        .globl syscall_handler

/*------------------------------------------------------------------------
 * syscall_initiator - enter syscall interrupt handler
 * the call is syscall_initiator( funcaddr , argc , args , retv_addr )
 *------------------------------------------------------------------------
 */
syscall_initiator:
        pushl   %ebp
        movl    %esp, %ebp
        pushal
        /* pass arguments by registers */
        movl    8(%ebp), %eax
        movl    12(%ebp), %ebx
        movl    16(%ebp), %ecx
        movl    20(%ebp), %edx
        /* syscall interrupt : 0x11 */
        int     $0x2e
        popal
        pop     %ebp
        ret

/*------------------------------------------------------------------------
 * syscall_handler - pass argument to corresponding syscall functions ,
 * then call. args: eax(funcaddr) , ebx(argc) , ecx(args) , edx(retv_addr)
 *------------------------------------------------------------------------
 */
/* note : callee saved ebx, esi, edi */
syscall_handler:
        pushl   %ebp
        movl    %esp, %ebp
        pushal 
        pushl %ds
        pushl %es 
        pushl %fs
        pushl %gs

        movl $(0x2 << 3), %esi
        movw %si, %ds
        movw %si, %es
        movw %si, %fs
        movw %si, %gs        

        subl    $4, %esp
        movl    %edx, (%esp)     /* store retv_addr */
        lea     (,%ebx,4), %esi
        subl    %esi, %esp       /* stack for args */

    /* for loop :  
        for( edx = 0 ; edx < ebx ; edx ++ ){
            tmpr = *(ecx+edx*4) ;
            *(esp+edx*4) = tmpr ;
        }
    */
        movl    $0, %edx
        cmpl    %edx, %ebx
        jle     .LFOR_END
    .LFOR_LABEL:
        movl    (%ecx,%edx,4), %esi
        movl    %esi, (%esp,%edx,4)
        addl    $1, %edx
        cmpl    %edx, %ebx /* calculate ebx - ebx */
        jg      .LFOR_LABEL /* if greater, goto loop */
    .LFOR_END:

        call    *%eax  /* call syscall function */

        lea     (,%ebx,4), %esi
        addl    %esi, %esp
        movl    (%esp), %edx
        movl    %eax, (%edx)
        addl    $4, %esp

        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        popal
        popl    %ebp
        iret 