#ifndef SYM_FINDER
#define SYM_FINDER

#define _GNU_SOURCE 1

#include <stdio.h>

#include <link.h>
#include <sys/auxv.h>
#include <unistd.h>


char* get_symbol(ElfW(Addr) address);




#endif /* ifndef SYM_FINDER */
