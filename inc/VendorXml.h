#ifndef VENDORXML_H
#define VENDORXML_H
#include <vendor/sprd/hardware/aprd/1.0/types.h>
#include <queue>
#include <semaphore.h>
#include "Thread.h"
#include "AprData.h"
using vendor::sprd::hardware::aprd::V1_0::e_info_V;
using vendor::sprd::hardware::aprd::V1_0::EExceptionType_V;
class VendorXmlThread : public Thread
{
public:
    VendorXmlThread(AprData* aprData, sem_t *pvsem);
    ~VendorXmlThread();
    sem_t *vsem;
    queue<e_info_V> vendor_e_q;
protected:
    virtual void Setup();
    virtual void Execute(void* arg);
    void VInfoSync(const char* prop, EExceptionType et, const char *arg, int* flash, struct e_info &m_ei);
    void CPXExpSync(e_info_V &p, struct e_info &m_ei);

private:
    AprData* m_aprData;
    struct e_info m_eis[6];
};
#endif
