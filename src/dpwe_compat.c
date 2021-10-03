/*
 * dpwe_compat.c
 *
 * emulate missing library fns for snack/mac
 * 1998nov15 dpwe@icsi.berkeley.edu
 * $Header: $
 */

#include <stdio.h>
#include <string.h>
#include <Memory.h>

void * memcpy (void * dst, const void * src, size_t len) {
    BlockMove(src, dst, len);
    return dst;
}

int strncmp(const char * str1, const char * str2, size_t len) {
   int i;
   char c1, c2;
   
   for (i = 0; i<len; ++i) {
       c1 = *str1++;
       c2 = *str2++;
       if (c1 < c2) return -1;
       else if (c1 > c2) return 1;
       else if (c1 == 0) break;  /* hit EOS while same */
   }
   return 0;
}

int strcmp(const char * str1, const char * str2) {
   char c1 = ' ', c2 = ' ';
   
   while (c1 != '\0') {
       c1 = *str1++;
       c2 = *str2++;
       if (c1 < c2) return -1;
       else if (c1 > c2) return 1;
   }
   return 0;
}

size_t strlen(const char * str) {
    int i = 0;
    
    while (*str++) ++i;
    return i;
}


