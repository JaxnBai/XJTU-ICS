#include <dlfcn.h> //turn to dlsym for help at fake load object
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <elf.h>
#include <link.h>
#include <string.h>

#include "Link.h"

// glibc version to hash a symbol
static uint_fast32_t
dl_new_hash(const char *s)
{
    uint_fast32_t h = 5381;
    for (unsigned char c = *s; c != '\0'; c = *++s)
        h = h * 33 + c;
    return h & 0xffffffff;
}

// find symbol `name` inside the symbol table of `dep`
void *symbolLookup(LinkMap *dep, const char *name)
{
    if(dep->fake)
    {
        void *handle = dlopen(dep->name, RTLD_LAZY);
        if(!handle)
        {
            fprintf(stderr, "relocLibrary error: cannot dlopen a fake object named %s", dep->name);
            abort();
        }
        dep->fakeHandle = handle;
        return dlsym(handle, name);
    }

    Elf64_Sym *symtab = (Elf64_Sym *)dep->dynInfo[DT_SYMTAB]->d_un.d_ptr;
    const char *strtab = (const char *)dep->dynInfo[DT_STRTAB]->d_un.d_ptr;

    uint_fast32_t new_hash = dl_new_hash(name);
    Elf64_Sym *sym;
    const Elf64_Addr *bitmask = dep->l_gnu_bitmask;
    uint32_t symidx;
    Elf64_Addr bitmask_word = bitmask[(new_hash / __ELF_NATIVE_CLASS) & dep->l_gnu_bitmask_idxbits];
    unsigned int hashbit1 = new_hash & (__ELF_NATIVE_CLASS - 1);
    unsigned int hashbit2 = ((new_hash >> dep->l_gnu_shift) & (__ELF_NATIVE_CLASS - 1));
    if ((bitmask_word >> hashbit1) & (bitmask_word >> hashbit2) & 1)
    {
        Elf32_Word bucket = dep->l_gnu_buckets[new_hash % dep->l_nbuckets];
        if (bucket != 0)
        {
            const Elf32_Word *hasharr = &dep->l_gnu_chain_zero[bucket];
            do
            {
                if (((*hasharr ^ new_hash) >> 1) == 0)
                {
                    symidx = hasharr - dep->l_gnu_chain_zero;
                    /* now, symtab[symidx] is the current symbol.
                       Hash table has done its job */
                    const char *symname = strtab + symtab[symidx].st_name;
                    if (!strcmp(symname, name))
                    {    
                        Elf64_Sym *s = &symtab[symidx];
                        // return the real address of found symbol
                        return (void *)(s->st_value + dep->addr);
                    }
                }
            } while ((*hasharr++ & 1u) == 0);
        }
    }
    return NULL; //not this dependency
}

void RelocLibrary(LinkMap *lib, int mode);
void fun_test5(LinkMap** p,int mode)
{
    LinkMap *lib=*p;
    if (strcmp(lib->name, "./test_lib/IndirectDep.so"))
            return ;
        Elf64_Dyn *dyn = lib->dyn;
        int j = 0;
       for(int j=0;j<lib->depcnt;++j){
            RelocLibrary(lib->dep[j], mode);
            dyn++;   
        }
    return;
}
extern void trampoline(void);

void RelocLibrary(LinkMap *lib, int mode)
{
    /* Your code here */
    if(strcmp(lib->name,"lib.so.6")==0)
        return;
    Elf64_Xword relse = 0;
    Elf64_Rela *rela = NULL;
    Elf64_Sym *sym = NULL;
    Elf64_Rela *rela_dyn = NULL;
    Elf64_Word relacount = 0;
    char *str = NULL;
    int restsize =0;
    if(lib->dynInfo[DT_PLTRELSZ])
        relse = lib->dynInfo[DT_PLTRELSZ]->d_un.d_val / sizeof(Elf64_Rela);
    if(lib->dynInfo[DT_JMPREL])
        rela = (Elf64_Rela *)lib->dynInfo[DT_JMPREL]->d_un.d_ptr;
    if(lib->dynInfo[DT_SYMTAB])
        sym = (Elf64_Sym *)lib->dynInfo[DT_SYMTAB]->d_un.d_ptr;
    if(lib->dynInfo[DT_RELA])
        rela_dyn = (Elf64_Rela *)lib->dynInfo[DT_RELA]->d_un.d_ptr;
    if( lib->dynInfo[DT_RELACOUNT])
        relacount = lib->dynInfo[DT_RELACOUNT]->d_un.d_val;
    if(lib->dynInfo[DT_STRTAB])
        str = (char *)lib->dynInfo[DT_STRTAB]->d_un.d_ptr;
    if(lib->dynInfo[DT_RELASZ]&&lib->dynInfo[DT_RELAENT])
    restsize = lib->dynInfo[DT_RELASZ]->d_un.d_val / lib->dynInfo[DT_RELAENT]->d_un.d_val - relacount;

    
    for (int i = 0; i < relse; i++, rela++)
    {
        if(ELF64_R_TYPE(rela->r_info) != R_X86_64_JUMP_SLOT )
            continue;
        if(mode == RTLD_LAZY)
            continue;
            void *res = NULL;
            for(int j=0;j<lib->depcnt;++j)
            {
                res= symbolLookup(lib->dep[j], &str[sym[ELF64_R_SYM(rela->r_info)].st_name]);
                if (res)
                {
                    res += rela->r_addend;
                    break;
                }
            }
           *(uint64_t *)(lib->addr + rela->r_offset)= (uint64_t)res; 
        
    } 
    for (int i = relacount; i >0; i--)
    {
        if(ELF64_R_TYPE(rela_dyn->r_info) == R_X86_64_RELATIVE)
        {
            void* add=(void*)lib->addr;
            *(uint64_t *)(lib->addr + rela_dyn->r_offset) = (uint64_t)(lib->addr + rela_dyn->r_addend);
            
        }
        rela_dyn++;
    }

    for(int i = 0; i < restsize; i++)
    {
        if (ELF64_ST_BIND(sym[ELF64_R_SYM(rela_dyn->r_info)].st_info) != STB_WEAK)
        {
        void * res = NULL;
        void* add=(void*)lib->addr;
        if(ELF64_R_TYPE(rela_dyn->r_info) == R_X86_64_GLOB_DAT)
            res = symbolLookup(lib, &str[sym[(rela_dyn->r_info) >> 32].st_name]);
        *(uint64_t *)(lib->addr + rela_dyn->r_offset) = (uint64_t)(res + rela_dyn->r_addend);
        }
        rela_dyn++;
    }

   fun_test5(&lib,mode);
     if(lib->dynInfo[DT_PLTGOT]){
        uint64_t *GOT=(typeof(GOT))lib->dynInfo[DT_PLTGOT]->d_un.d_ptr;
        GOT[1]=(uint64_t)lib;
        GOT[2]=(uint64_t)&trampoline;
    }

}


   
   
