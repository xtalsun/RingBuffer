#include "RingBuffer.h"

bool createShareMemory(key_t IPCKey, int64_t shmSize)
{
    printf("Memory page size = %d\n", getpagesize());
    int shmid, ret;
    void *mem;
    shmid = shmget(IPCKey, shmSize, 0666|IPC_CREAT);
    if(shmid > 0)
    {
        printf("Create share memory id is %d\n", shmid);
    }
    else
    {
        printf("Create share memory fail, change IPCKey and try again.\n");
        return false;
    }
    struct shmid_ds shmidDS;
    ret = shmctl(shmid, IPC_STAT, &shmidDS);
    if(ret == 0)
    {
        printf("Size of share memory is %d\n", (int)shmidDS.shm_segsz);

        mem = shmat(shmid, (const void *)0, 0);
        if((int64_t)mem != -1)
        {
            printf("Share memory is attached.\n");

            SHMHead *head = new SHMHead;
            int i;
            for(i = 0; i < BUFNUM; i++)
            {
                head->bufIndex[i] = 0;
            }
            head->currenTaiLength = 0;
            memcpy(mem, (const void *)head, sizeof(SHMHead));
            printf("SHMHead created.\n");

            ret = shmdt(mem);
            if(ret == 0)
            {
                printf("Share memory detach ok.\n");
            }
            else
            {
                printf("Share memory detach fail.\n");
            }
        }
        else
        {
            printf("shmat() fail.\n");
        }
    }
    else
    {
        printf("shmctl() fail\n");
    }
    return true;
}

bool RingBuffer::createNewRingbuf(key_t IPCkey, char *name, uint32_t length)
{
    BUFHead *rbhead = new BUFHead;
    rbhead->rIndex = 0;
    rbhead->wIndex = 0;
    rbhead->readyIndex = 0;
    rbhead->bufLength = length;

    int shmid = shmget(IPCkey, 0, 0);   //Attach to share memory by IPCKey.
    if(shmid < 0)
    {
        printf("Create ringbuffer:\nshmget() fail. Please call createShareMemory() first.\n");
        return false;
    }
    SHMHead *head = (SHMHead *)shmat(shmid, (const void *)0, 0);
    if((int64_t)head == -1)
    {
        printf("Create ringbuffer:\nshmat() fail, try again.\n");
        return false;
    }
    //If attach success, currenTaiLength will print normally.
    printf("Create ringbuffer:\nAttach to shM ok, tail length is %d.\n", head->currenTaiLength);

    //Find the new address to create a new ringbuffer.
    int i;
    for(i = 0; i < BUFNUM; i++)
    {
        if(strcmp(head->bufName[i], name) == 0)
        {
            printf("Ringbuffer name %s exists, change name and try again.\n", name);
            return false;
        }
        if(head->bufIndex[i] == 0)  //0 means no ring buffer use this index, so can create a new.
        {
            //Index value is base of 1 byte.
            if(i == 0)
            {
                head->bufIndex[i] = sizeof(SHMHead);    //First ringbuffer's head index.
            }
            else
            {
                //Second, third, forth...ringbuffer's head index.
                head->bufIndex[i] = head->bufIndex[i-1] + sizeof(BUFHead) + head->currenTaiLength;
            }
            strcpy(head->bufName[i], name);
            head->currenTaiLength = length;
            //Calculate whether new ringbuffer is out of memory.
            /*if(head->bufIndex[i] + head->currenTaiLength + sizeof(BUFHead) >= SHMSIZE)
            {
                printf("Out of memory.\n");
                return false;
            }*/
            break;
        }
        //Program run here means the last ringbuffer's index != 0, so it is used.
        if(i == BUFNUM-1)
        {
            shmdt((const void *)head);  //Detach share memory.
            printf("Out of bufnum, no more buffer in shM.\n");
            return false;
        }
    }
    void *dest = (void *)head + head->bufIndex[i];  //Calculate head address of ringbuffer.
    memcpy(dest, rbhead, sizeof(BUFHead));  //Copy rbhead into share memory.
    delete rbhead;
    printf("Create ringbuffer NO.%d ok, name is %s.\n", i+1, name);
    shmdt((const void *)head);  //Detach share memory.
    return true;
}

bool RingBuffer::bufAttach(key_t IPCkey, char *name)
{
    int shmid = shmget(IPCkey, 0, 0);   //Attach to share memory by IPCKey.
    SHMHead *head = (SHMHead *)shmat(shmid, (const void *)0, 0);
    //If attach success, currenTaiLength will print normally.
    printf("Attach ringbuffer:\nAttach to shm, tail is %d\n", head->currenTaiLength);

    //Get NO.0 ringbuffer's head address.
    for(int i = 0; i < BUFNUM; i++)
    {
        if(strcmp(head->bufName[i], name) == 0)
        {
            BUFHead *rbhead = (BUFHead *)((void *)head + head->bufIndex[i]);    //Get buffer head's address.
            printf("Get ringbuffer's head in shM ok. %d %ld %ld %ld\n", rbhead->bufLength, rbhead->rIndex, rbhead->wIndex, rbhead->readyIndex);

            rIndex = &rbhead->rIndex;   //Get rIndex's address in share memory.
            wIndex = &rbhead->wIndex;   //Get wIndex's address in share memory.
            readyIndex = &rbhead->readyIndex;   //Get redayIndex's address in share memory.
            bufLength = rbhead->bufLength;
            bufAddr = (char *)(rbhead + 1);    //Get buffer first unit's address.
            printf("RB init ok, length is %d, index are %ld %ld %ld.\n", bufLength, *rIndex, *wIndex, *readyIndex);
            break;
        }
        if(i == BUFNUM -1)
        {
            printf("Ringbuffer name %s not found.\n", name);
            return false;
        }
    }
    return true;
}

char * RingBuffer::getAddr(uint32_t index)
{
    return (bufAddr + (index&(bufLength-1)));
}

bool RingBuffer::push(MsgBase *m)
{
    //Wait for previous push() done. Time out about 1.0sec.
    while((*readyIndex) != (*wIndex))
    {
        BARRIER();
        static volatile int32_t cnt = 0;
        if(cnt++ > 0x10000000)
        {
            *readyIndex = *wIndex;
            cnt = 0;
            break;
        }
        pthread_yield();
    }

    int32_t slots = m->slotNum;
    BARRIER();
    ATOMIC_ADD(wIndex,slots);
    while((*wIndex) - (*rIndex) >= bufLength)
    {
        pthread_yield();
    }
    memcpy(getAddr(*wIndex-slots), m, slots);
    BARRIER();
    ATOMIC_ADD(readyIndex, slots);
    //printf("%s %d %d. ", m->data, *wIndex, *readyIndex);
    return true;
}


bool RingBuffer::pull(MsgBase *m)
{
    while((*rIndex >= *readyIndex) || (*rIndex >= *wIndex))
    {
        pthread_yield();
    }
    MsgBase *tmp = (MsgBase *)getAddr(*rIndex);
    memcpy(m, tmp, tmp->slotNum);
    *rIndex += m->slotNum;
    return true;
}

void RingBuffer::indexClean()
{
    *rIndex = 0;
    *wIndex = 0;
    *readyIndex = 0;
}

void RingBuffer::rIndexClean()
{
    *rIndex = 0;
}

uint32_t RingBuffer::getLength()
{
    return bufLength;
}

uint64_t RingBuffer::getwIndex()
{
    return *wIndex;
}

uint64_t RingBuffer::getrIndex()
{
    return *rIndex;
}

uint64_t RingBuffer::getReadyIndex()
{
    return *readyIndex;
}
