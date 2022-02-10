#ifndef ANRTHREAD_H
#define ANRTHREAD_H
#include "Thread.h"
#include "AprData.h"

struct log_device_t;

class AnrThread : public Thread
{
public:
    AnrThread(AprData* aprData);
    ~AnrThread();

protected:
    log_device_t* m_devices;
    int m_devCount;
    struct logger_list* m_logger_list;
    void processBuffer(log_device_t* dev, struct log_msg *buf);

    virtual void Setup();
    virtual void Execute(void* arg);
    virtual void Dispose();
    char m_buffer[2048];

private:
    AprData* m_aprData;
    struct e_info m_ei;

};

#endif

