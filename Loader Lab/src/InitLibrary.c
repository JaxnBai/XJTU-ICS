#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <stdint.h>

#include "Link.h"
#include "LoaderInternal.h"

void InitLibrary(LinkMap *l)
{
    /* Your code here */
     Elf64_Dyn **dynInfo=l->dynInfo;
    Elf64_Rela *rela=NULL;
    int relacount=0;
    void (*init)()=NULL;
    void (**initarr)()=NULL;
    int initarrsz=0;
    if(dynInfo[DT_RELA])
        rela=(Elf64_Rela*)dynInfo[DT_RELA]->d_un.d_ptr;
    if(dynInfo[DT_RELACOUNT])
        relacount=dynInfo[DT_RELACOUNT]->d_un.d_val;
    if(dynInfo[DT_INIT])
        init=(typeof(init))dynInfo[DT_INIT]->d_un.d_ptr;
    if(dynInfo[DT_INIT_ARRAY])
        initarr=(typeof(initarr))dynInfo[DT_INIT_ARRAY]->d_un.d_ptr;
    if(dynInfo[DT_INIT_ARRAYSZ])
        initarrsz=dynInfo[DT_INIT_ARRAYSZ]->d_un.d_val/sizeof(void*);
    for(int i=0;i<relacount;++i,++rela){
        *(uint64_t*)(l->addr+rela->r_offset)=l->addr+rela->r_addend;
    }
    init();
    for (int i = 0; i < initarrsz; i++)
    {
        initarr[i]();
    }
}
