#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h> //uintXX_t
#include <stdio.h>
#include <sys/shm.h>    //shmXX()
#include <pthread.h>    //pthread_yield()
#include <string.h> //memcpy()
#include <unistd.h> //getpid()
#include <sys/types.h>  //size_t, key_t

#define BUFNUM 128  //Mostly 128 ring buffers in a share memory.
#define BARRIER() asm volatile("":::"memory")   //Update register and cache from memory.
#define ATOMIC_ADD(p,v) __sync_add_and_fetch((p),(v))   //Lock cache and add, seems like atomic operation.

#define DEFAULTCONF
//SHMHead is used to manage share memory's content,
//including index(to calculate address), buffer's name(to identify different ringbuffer).
typedef struct
{
    int32_t bufIndex[BUFNUM];   //Each ringbuffer's address index, such as address = shm's head address + bufIndex[i].
    int32_t currenTaiLength;    //Current ringbuffer's length of unit, used to calculate new ringbuffer's address.
                                //Such as bufIndex[i+1] = bufIndex[i] + currenTaiLength * sizeof(SlotType).
    char bufName[BUFNUM][32];
}SHMHead;

//BUFHead manage its ringbuffer's content,
//including read, write, and ready index, and buffer's length.
typedef struct
{
    volatile uint64_t rIndex;   //Read index, point to slot to be read.
    volatile uint64_t wIndex;   //Write index, point to slot to be writen.
    volatile uint64_t readyIndex;   //After write done, readyIndex == wIndex.
    uint32_t bufLength; //Total number of ringbuffer's slots.
}BUFHead;

bool createShareMemory(key_t IPCKey, int64_t shmSize);

class MsgBase
{
public:
    int32_t slotNum;    //slotNum = sizeof(MsgBase)
    int64_t timetag;    //Current time of sending Msg.
    int32_t from;
    int32_t to;
    int16_t msgType;
    char data[32];
    int16_t errFlag;

    MsgBase()
    {
        slotNum = getSize();
    }

    size_t getSize()
    {
        size_t thisSize = sizeof(*this);
        //thisSize must be i*64, get the closest one.
        for(uint16_t i = 1; i < 100; i++)
        {
            if(thisSize <= 64*i)
            {
                thisSize = 64*i;
                break;
            }
        }
        return thisSize;
    }
};

class RingBuffer
{
private:
    volatile uint64_t *rIndex;  //Point to rIndex, for read controling.
    volatile uint64_t *wIndex;  //Point to wIndex, for write controling.
    volatile uint64_t *readyIndex;  //Point to readyIndex, for write controling. After write done, readyIndex == wIndex.
    uint32_t bufLength;
    char *bufAddr; //Point to ringbuffer first unit's address.

public:
    RingBuffer()
    {

    }

    ~RingBuffer()
    {

    }

    //Create a new ringbuffer in shM, initialize its head, msg type, and length.
    //Important!!! length must be 2^n, which makes buffer like a ring.
    //All RingBuffer<T> object can access the head in shM, use pointer to manage rIndex, wIndex, readyIndex.
    bool createNewRingbuf(key_t IPCkey, char *name, uint32_t length);

    //Use rbhead's value to initialize RingBuffer<T> object which call this function.
    bool bufAttach(key_t IPCkey, char *name);

    //Get address by index.
    char * getAddr(uint32_t index);

    bool push(MsgBase *m);
    bool pull(MsgBase *m);

    //Some assistant function.
    void indexClean();
    void rIndexClean();
    uint32_t getLength();
    uint64_t getwIndex();
    uint64_t getrIndex();
    uint64_t getReadyIndex();

};

#endif // RINGBUFFER_H
