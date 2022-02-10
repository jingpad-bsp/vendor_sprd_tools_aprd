#ifndef RamUsedThread_H
#define RamUsedThread_H

#include "Thread.h"
#include "AprData.h"


class RamUsedThread : public Thread
{
public:
    RamUsedThread(AprData* aprData);
    ~RamUsedThread();


protected:
    virtual void Setup();
    virtual void Execute(void* arg);
    int get_RamUsed();
    int getRamMaxUsedApp(char* appinfo);
    int update_RamOversized();
    char m_buffer[2048];

private:
    AprData* m_aprData;
    struct e_info m_ei;
};

#endif

