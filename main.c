//
//  main.c
//  iOSFuckDenyAttach
//
//  Created by q7654 on 2022/7/27.
//  Copyright (c) 2022 q7654. All rights reserved.
//

#include <assert.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/proc.h>
#include <sys/param.h>

#include "find_kernel_base_under_checkra1n.h"
#include "proc.h"

#define P_LNOATTACH     0x00001000      /* */

/* Macros to clear/set/test flags. */
#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) &= ~(f)
#define ISSET(t, f)     ((t) & (f))

//iphone8plus  ios 14.6  kernel 
#define TARGET_KERNELCACHE_VERSION_STRING "@(#)VERSION: Darwin Kernel Version 20.5.0: Sat May  8 02:21:43 PDT 2021; root:xnu-7195.122.1~4/RELEASE_ARM64_T8015"

int main (int argc, const char * argv[])
{
    kernel_task_init();
    uint64_t kb = kernel_base_init();
    for (size_t i = 0; i < 8; i++) {
        printf("kernel base + 0x%02x:%016llx\n", (int)(8*i), kernel_read64(kb + 8 * i));
    }
    
    uint64_t versionstraddr = kb + 0x39924;
    char versionstr[256];
    if(!kernel_read(versionstraddr, (void *)&versionstr, sizeof(versionstr))){
        printf("failed to read kernel version string\n");
        return -1;
    }
    
    if(strcmp(TARGET_KERNELCACHE_VERSION_STRING,versionstr) != 0){
        printf("kernel cache version mismatch\n");
        return -1;
    }
    
    printf("kernel cache hit\n");
    //0x2253a98  kernproc
    uint64_t kernel_proc0 = kb + 0x2253a98;
    
    struct proc * proc0 =  (void *)malloc(sizeof(struct proc));
    
    if(!kernel_read(kernel_proc0, (void *)proc0, sizeof(struct proc)))
    {
        printf("proc0 read failed\n");
        return -1;
    }
    
    printf("uniqueid offset 0x%llx  comm offset 0x%llx \n",(int64_t)&(proc0->p_uniqueid) - (int64_t)proc0, (int64_t)&(proc0->p_comm)- (int64_t)proc0);
    
    struct proc * proc1 =  (struct proc *)malloc(sizeof(struct proc));
    uint64_t preptr = (uint64_t)(proc0->p_list.le_prev);
    
    while(preptr){
        if(!kernel_read(preptr, (void *)proc1, sizeof(struct proc)))
        {
            printf("procnext read failed\n");
            return -1;
        }
        
        if(LIST_NEXT(proc1, p_list) == NULL)
        {
            printf("proc1->p_list.le_prev == 0\n");
            break;
        }
        int64_t lflagoffset = (int64_t)&(proc1->p_lflag) - (int64_t)proc1;
        int lflagvalue = proc1->p_lflag;
        printf("(%llu)%s  proc = 0x%llx   lflag = 0x%x  lflag offset = 0x%llx"
               ,proc1->p_uniqueid,
               proc1->p_comm,//(char *)((int64_t)proc1 + 0x258),
               preptr,lflagvalue,lflagoffset);
        
        if(ISSET(lflagvalue, P_LNOATTACH))
        {
            printf(" !!!P_LNOATTACH set");
            CLR(lflagvalue, P_LNOATTACH);
            KERNEL_WRITE32(preptr + lflagoffset, lflagvalue);
        }
        printf("\n");
        
        preptr = (uint64_t)(proc1->p_list.le_prev);
        
    }
    
    printf("end\n");
    free(proc0);
    free(proc1);
    return 0;
}

