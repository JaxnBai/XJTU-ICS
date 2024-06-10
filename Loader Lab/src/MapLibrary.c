#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <unistd.h> //for getpagesize
#include <sys/mman.h>
#include <fcntl.h>

#include "Link.h"
#include "LoaderInternal.h"

#define ALIGN_DOWN(base, size) ((base) & -((__typeof__(base))(size)))
#define ALIGN_UP(base, size) ALIGN_DOWN((base) + (size)-1, (size))

static const char *sys_path[] = {
    "/usr/lib/x86_64-linux-gnu/",
    "/lib/x86_64-linux-gnu/",
    ""
};

static const char *fake_so[] = {
    "libc.so.6",
    "ld-linux.so.2",
    ""
};

static void setup_hash(LinkMap *l)
{
    uint32_t *hash;

    /* borrowed from dl-lookup.c:_dl_setup_hash */
    Elf32_Word *hash32 = (Elf32_Word *)l->dynInfo[DT_GNU_HASH]->d_un.d_ptr;
    l->l_nbuckets = *hash32++;
    Elf32_Word symbias = *hash32++;
    Elf32_Word bitmask_nwords = *hash32++;

    l->l_gnu_bitmask_idxbits = bitmask_nwords - 1;
    l->l_gnu_shift = *hash32++;

    l->l_gnu_bitmask = (Elf64_Addr *)hash32;
    hash32 += 64 / 32 * bitmask_nwords;

    l->l_gnu_buckets = hash32;
    hash32 += l->l_nbuckets;
    l->l_gnu_chain_zero = hash32 - symbias;
}

static void fill_info(LinkMap *lib)
{
    Elf64_Dyn *dyn = lib->dyn;
    Elf64_Dyn **dyn_info = lib->dynInfo;

    while (dyn->d_tag != DT_NULL)
    {
        if ((Elf64_Xword)dyn->d_tag < DT_NUM)
            dyn_info[dyn->d_tag] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_RELACOUNT_)
            dyn_info[DT_RELACOUNT] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_GNU_HASH_)
            dyn_info[DT_GNU_HASH] = dyn;
        ++dyn;
    }
    #define rebase(tag)                             \
        do                                          \
        {                                           \
            if (dyn_info[tag])                          \
                dyn_info[tag]->d_un.d_ptr += lib->addr; \
        } while (0)
    rebase(DT_SYMTAB);
    rebase(DT_STRTAB);
    rebase(DT_RELA);
    rebase(DT_JMPREL);
    rebase(DT_GNU_HASH); //DT_GNU_HASH
    rebase(DT_PLTGOT);
    rebase(DT_INIT);
    rebase(DT_INIT_ARRAY);
}
int getprot(Elf64_Phdr * p)
{
    int prot = 0;
        prot |= (p->p_flags & PF_R) ? PROT_READ : 0;
        prot |= (p->p_flags & PF_W) ? PROT_WRITE : 0;
        prot |= (p->p_flags & PF_X) ? PROT_EXEC : 0;
        return prot;
}
void *MapLibrary(const char *libpath)
{
    /*
     * hint:
     * 
     * lib = malloc(sizeof(LinkMap));
     * 
     * foreach segment:
     * mmap(start_addr, segment_length, segment_prot, MAP_FILE | ..., library_fd, 
     *      segment_offset);
     * 
     * lib -> addr = ...;
     * lib -> dyn = ...;
     * 
     * fill_info(lib);
     * setup_hash(lib);
     * 
     * return lib;
    */
   
    /* Your code here */
    
    LinkMap *lib=(LinkMap *)malloc(sizeof(LinkMap));
    lib->name=libpath;
    lib->dyn = NULL;
    if(strcmp(libpath,"libc.so.6")==0){
        lib->fake = 1;
        lib->fakeHandle = (void*)-1;
        return lib;
    }
    char str[100]={0}; 
    Elf64_Ehdr header;
    sprintf(str,libpath[0]=='.'?"%s":"./test_lib/%s",libpath);
    int fd = open(str, O_RDONLY);
    if (fd == -1)
        exit(0); 
    if (pread(fd,&header,sizeof(Elf64_Ehdr),0)==-1) 
        exit(0);
    Elf64_Phdr *p = malloc(sizeof(Elf64_Phdr));
    pread(fd, p, sizeof(Elf64_Phdr), header.e_phoff);
    int prot=getprot(p);   
    void *address =  mmap(NULL, (uint64_t)(header.e_phnum* getpagesize()), prot,
                        MAP_PRIVATE, fd, ALIGN_DOWN(p->p_offset, getpagesize()));
    lib->addr = (uint64_t)address + p->p_offset - ALIGN_DOWN(p->p_offset, getpagesize());
            free(p);    

    for (int i = 1; i < header.e_phnum; i++)
    {
        Elf64_Phdr *p = malloc(sizeof(Elf64_Phdr));
        pread(fd, p, sizeof(Elf64_Phdr), header.e_phoff + i * sizeof(Elf64_Phdr));
        int prot=getprot(p);
        switch(p->p_type)
        {   
        case 1:
            mmap(p->p_vaddr + address - p->p_offset + ALIGN_DOWN(p->p_offset, getpagesize()), 
            ALIGN_UP(p->p_memsz + p->p_offset - ALIGN_DOWN(p->p_offset, getpagesize()), getpagesize()),  prot,
                 MAP_PRIVATE | MAP_FIXED, fd, ALIGN_DOWN(p->p_offset, getpagesize()));
                  break;
        case 2:
            lib->dyn = address + p->p_vaddr; 
            break;
        default:
            break;
        }
     free(p);
    }
    fill_info(lib);
    setup_hash(lib);
    Elf64_Dyn *dyn = lib->dyn;    
    lib->depcnt=0;
    lib->dep = malloc(sizeof(LinkMap) * (100)); 
    while (dyn->d_tag != DT_NULL)
    {
        if (dyn->d_tag == DT_NEEDED)
            lib->dep[lib->depcnt++] = MapLibrary((char *)lib->dynInfo[DT_STRTAB]->d_un.d_ptr + dyn->d_un.d_ptr);
        dyn++;
    } 
     close(fd);

    return lib;
}


