/*
 * Copyright (C) 2001-2006 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */
 
/*
 * Author:  Steven Wallace
 */


#ifndef _MOPEN_
#define _MOPEN_

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

/********************************************************************
 *
 * Controller calls
 *
 *******************************************************************/

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef MAP_VARIABLE
#define MAP_VARIABLE 0
#endif
#ifndef MAP_NOSYNC
#define MAP_NOSYNC 0
#endif

#define MOPEN_ADDR_DEFAULT 0x2ef000000uL
#define MOPEN_LEN_DEFAULT  0x42000

static inline void *
open_mpipe(char *pipename, size_t len)
{
    int   mpipe_fd;
    void *mpipe_addr;

    if(!len)
        len = MOPEN_LEN_DEFAULT;

//    mpipe_fd = mkstemp(pipename);
    mpipe_fd = open(pipename, O_RDWR|O_CREAT, 0600);
    if(mpipe_fd < 0) {
        perror("open mpipe");
        return NULL;
    }

    // lseek() and write() to end of file (len).
    lseek(mpipe_fd, len-sizeof(len), SEEK_SET);
    write(mpipe_fd, &len, sizeof(len));

    mpipe_addr = mmap(0, len, PROT_READ|PROT_WRITE, MAP_VARIABLE|MAP_FILE|MAP_SHARED|MAP_NOSYNC, mpipe_fd, 0);
    if(mpipe_addr == (caddr_t)-1) {
        perror("mmap mpipe");
	close(mpipe_fd);
        return NULL;
    }

    close(mpipe_fd);
    return mpipe_addr;
}


static inline void *
open_mpipe_anon(size_t len)
{
    void *mpipe_addr;

    if(!len)
        len = MOPEN_LEN_DEFAULT;

    mpipe_addr = mmap((void *)MOPEN_ADDR_DEFAULT, len, PROT_READ|PROT_WRITE, MAP_VARIABLE|MAP_SHARED|MAP_ANONYMOUS|MAP_INHERIT, -1, 0);
    if(mpipe_addr == (caddr_t)-1) {
        perror("mmap mpipe");
        return NULL;
    }

    return mpipe_addr;
}

#endif /* _MOPEN_ */
