#define UTIL__C


/* Some generally useful routines */
/* The majority of the non-portable code is in here. */

/*

Copyright (c) 2001, Richard Krehbiel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

o Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

o Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"                      /* own defintions */

#ifdef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#define stat _stat
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static void my_searchenv1(
    char *name,
    char *envname,
    char *hitfile,
    int hitlen);

static void my_searchenv2(
    char *name,
    char *envname,
    char *hitfile,
    int hitlen);

/* Sure, the library typically provides some kind of
    ultoa or _ultoa function.  But since it's merely typical
    and not standard, and since the function is so simple,
    I'll write my own.

    It's significant feature is that it'll produce representations in
    any number base from 2 to 36.
*/

char           *my_ultoa(
    unsigned long val,
    char *buf,
    unsigned int base)
{
    static char     digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char           *strt = buf,
            *end;

    do {
        *buf++ = digits[val % base];
        val /= base;
    } while (val != 0);

    *buf = 0;                          /* delimit */
    end = buf + 1;

    /* Now reverse the bytes */

    while (buf > strt) {
        char            temp;

        temp = *--buf;
        *buf = *strt;
        *strt++ = temp;
    }

    return end;
}

/* Ditto my_ultoa.  This actually emits
    a signed representation in other number bases. */

char           *my_ltoa(
    long val,
    char *buf,
    unsigned int base)
{
    unsigned long   uval;

    if (val < 0)
        uval = -val, *buf++ = '-';
    else
        uval = val;

    return my_ultoa(uval, buf, base);
}

/*
  _searchenv is a function provided by the MSVC library that finds
  files which may be anywhere along a path which appears in an
  environment variable.  I duplicate that function for portability.
  Note also that mine avoids destination buffer overruns.

  Added functionality to lowercase the file name and to remove the
  device name and directory.
*/

void my_searchenv(
    char *name,
    char *envname,
    char *hitfile,
    int hitlen)
{
    my_searchenv1(name, envname, hitfile, hitlen);
    if (*hitfile) {
        return;
    }

    char *copy = memcheck(strdup(name));
    downcase(copy);
    my_searchenv1(copy, envname, hitfile, hitlen);
    free(copy);
}

static void my_searchenv1(
    char *name,
    char *envname,
    char *hitfile,
    int hitlen)
{
    my_searchenv2(name, envname, hitfile, hitlen);
    if (*hitfile) {
        return;
    }

    /*
     * Parse DEV:[DIR]NAME
     * while re-trying the search after DEV: and after [DIR].
     *
     * Let's not be too critical about the characters we allow in those.
     */
    char *p = name;
    char c;

    while ((c = *p++)) {
        if (c == ':') {
            my_searchenv2(p, envname, hitfile, hitlen);
            if (*hitfile) {
                return;
            }
        }
        if (c == '[') {
            char *enddir = strchr(p, ']');
            if (enddir == NULL) {
                return; /* weird syntax */
            }
            p = enddir + 1;
            my_searchenv2(p, envname, hitfile, hitlen);
            return;
        }
    }

    return;
}

static void my_searchenv2(
    char *name,
    char *envname,
    char *hitfile,
    int hitlen)
{
    char           *env;
    char           *envcopy;
    char           *envcopy2;
    char           *cp;
    char           *last;

    *hitfile = 0;                      /* Default failure indication */

    /* Note: If the given name is absolute, then don't search the
       path, but use it as is. */

    if (
#ifdef WIN32
           strchr(name, ':') != NULL || /* Contain a drive spec? */
           name[0] == '\\' ||          /* Start with absolute ref? */
#endif
           name[0] == '/') {           /* Start with absolute ref? */
        strncpy(hitfile, name, hitlen); /* Copy to target */
        return;
    }

    env = getenv(envname);
    if (env == NULL) {                 /* If not defined, search in */
        env = ".";                     /* current directory */
    }

    envcopy = strdup(env);             /* strtok destroys its text
                                          argument.  I don't want the return
                                          value from getenv destroyed. */

    envcopy2 = envcopy;
    while ((cp = strtok_r(envcopy2, PATHSEP, &last)) != NULL) {
        struct stat     info;
        char           *concat = malloc(strlen(cp) + strlen(name) + 2);

        if (concat == NULL) {
            free(envcopy);
            return;
        }
        strcpy(concat, cp);
        if (concat[strlen(concat) - 1] != '/')
            strcat(concat, "/");
        strcat(concat, name);
        if (!stat(concat, &info)) {
            /* Copy the file name to hitfile.  Assure that it's really
               zero-delimited. */
            strncpy(hitfile, concat, hitlen - 1);
            hitfile[hitlen - 1] = 0;
            free(envcopy);
            return;
        }
        envcopy2 = NULL;
    }

    /* If I fall out of that loop, then hitfile indicates no match,
       and return. */
    free(envcopy);
}




/* memcheck - crash out if a pointer (returned from malloc) is NULL. */

void           *memcheck(
    void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/* upcase turns a string to upper case */

void upcase(
    char *str)
{
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

/* downcase turns a string to lower case */

void downcase(
    char *str)
{
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}

/* padto adds blanks to the end of a string until it's the given
   length. */

void padto(
    char *str,
    int to)
{
    int             needspace = to - strlen(str);

    str += strlen(str);
    while (needspace > 0)
        *str++ = ' ', needspace--;
    *str = 0;
}
