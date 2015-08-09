#include <stdio.h>  //sprintf()
#include <sys/shm.h>    //shmXX()
#include <stdint.h> //uintXX_t
#include <pthread.h>
#include <unistd.h> //getpid()
#include <sys/time.h>   //gettimeofday()
#include "RingBuffer.h"

int main(int argc, char **argv)
{
    //Create a new share memory by IPCKey and size.
    key_t IPCKEY = 1;   //Use to identify different share memory.
    //size_t SHMSIZE = 134217728; //128MB
    //createShareMemory(IPCKEY, SHMSIZE);

    //Create a new ringbuffer and attach to it.
    RingBuffer *rbuf = new RingBuffer;
    rbuf->createNewRingbuf(IPCKEY, "stock1", 1024);
    rbuf->bufAttach(IPCKEY, "stock1");

    //Test for wait time.
    struct timeval start, stop;
/*
    gettimeofday(&start, NULL);
    while(true)
    {
        static volatile int32_t cnt = 0;
//        if(cnt++ > 0x10000000)
        {
            break;
        }
    }
    gettimeofday(&stop, NULL);
    printf("Timeout %ld s + %ld us.\n", stop.tv_sec-start.tv_sec, stop.tv_usec-start.tv_usec);
*/

    //Push test.
    //rbuf->indexClean();
    MsgBase *m = new MsgBase;
    m->from = 1;
    m->to = 2;
    m->msgType = 1;
    sprintf(m->data, "Pid %d.", getpid());
    //gettimeofday(&start, NULL);
    for(int i = 0; i < 500000; i++)
    {
        gettimeofday(&start, NULL);
        m->timetag = start.tv_usec;
        rbuf->push(m);
        gettimeofday(&stop, NULL);
        printf("Once push time %ld us.\n ", stop.tv_usec-start.tv_usec);
        usleep(50000);
    }
    //gettimeofday(&stop, NULL);
    //printf("500000 push time %ld s + %ld us. ", stop.tv_sec-start.tv_sec, stop.tv_usec-start.tv_usec);


/*
    //Pull test
    //rbuf->indexClean();
    MsgBase *m_ = new MsgBase;
    while(true)
    {
        //gettimeofday(&start, NULL);
        rbuf->pull(m_);
        gettimeofday(&stop, NULL);
        printf("%s. Once transfer time %ld us.\n", m_->data, stop.tv_usec-m_->timetag);
    }
*/
    //rbuf->indexClean();
    return 0;
}
