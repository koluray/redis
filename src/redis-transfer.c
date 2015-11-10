/* Redis CLI (command line interface)
 *
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "fmacros.h"
#include "version.h"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>

#include "hiredis.h"
#include "sds.h"
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include "redis-transfer.h"
#include "redis.h"
#include <semaphore.h>

pthread_t tid;
sem_t mutex;


/*------------------------------------------------------------------------------
 * thread entry()
 *--------------------------------------------------------------------------- */
void* redis_transfer()
{
    redisContext *c = redisConnect(server.masterhost, server.masterport);
    if (c != NULL && c->err) {
        printf("Error: %s\n", c->errstr);
        // handle error
    }
    
    char in[BUFFER];
    sem_post(&mutex);
    int in_fd=open(FIFO, O_RDONLY);
    
    if (in_fd==-1) {
        perror("open error");
    }
    
    while (1) {
        if (read(in_fd, in, BUFFER)>0) {
            printf("Transfer raw command  %s to server.\n", in);
            sdsfree(c->obuf);
            c->obuf = sdsnew(in);
            void* reply = NULL;
            if (c->flags & REDIS_BLOCK) {
                if (redisGetReply(c,&reply) != REDIS_OK)
                    continue;
            }
        }

    }
    void redisFree(redisContext *c);
    pthread_exit(0);
    return NULL;
}

int redis_transfer_thread() {
    //only create one thread for all the command transfer.
    //and this thread will not close till the program exit, and file FIFO will not close too.
    if (tid == 0) {
        mkfifo(FIFO, 0666);
        sem_init(&mutex, 0, 0);
        int err = pthread_create(&tid, NULL, &redis_transfer, NULL);
        if (err != 0){
            printf("\ncan't create thread :[%s]", strerror(err));
            sem_destroy(&mutex);
            return -1;
        }else
            printf("\n Thread created successfully\n");
    }
    
    static int fd = -1;
    if (fd <0) {
        sem_wait(&mutex);
        fd = open(FIFO, O_WRONLY);
        if (fd==-1) {
            perror("File FIFO open error");
            //someerror occured, do not creat thread.
            sem_destroy(&mutex); /* destroy semaphore */
            return fd;
        }
    }
    
    return fd;
}
