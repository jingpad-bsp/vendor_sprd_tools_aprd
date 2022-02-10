
#ifndef SSHLISTENER_H
#define SSHLISTENER_H
#include "Thread.h"
#include "AprData.h"

class SSRListener : public Thread
{
    static const int PID_LEN = 10;
public:
    SSRListener(AprData* aprData);
    ~SSRListener();

protected:
    virtual void Setup();
    virtual void Execute(void* arg);
    virtual void Dispose();
    char m_buffer[2048];

private:
    char m_system_pid_old[PID_LEN];
    char m_system_pid_cur[PID_LEN];
    char m_persist_old[PROPERTY_VALUE_MAX];

    AprData *m_aprData;
    struct e_info m_ei;

    void UpdateXML();
    void GetSystemServerPid(int isFirst);
};

#endif

