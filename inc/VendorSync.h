#ifndef VENDORSYNC_H
#define VENDORSYNC_H
#include <vendor/sprd/hardware/aprd/1.0/IAprdInfoSync.h>
#include <vendor/sprd/hardware/aprd/1.0/IAprdCallback.h>
#include <vendor/sprd/hardware/aprd/1.0/types.h>
#include <hidl/LegacySupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/HidlSupport.h>
#include "AprdCallback.h"
//#include "AprdInfoSync.h"
#include "Thread.h"

class VendorSyncThread : public Thread
{
public:
    VendorSyncThread(void* VendorXml);
    ~VendorSyncThread();
    void* mVendorXml;
protected:
    virtual void Setup();
    virtual void Execute(void* arg);
};

#endif
