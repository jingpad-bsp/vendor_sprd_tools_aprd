#include "VendorSync.h"
#include "AprdCallback.h"

using vendor::sprd::hardware::aprd::V1_0::IAprdInfoSync;
using vendor::sprd::hardware::aprd::V1_0::IAprdCallback;
using vendor::sprd::hardware::aprd::V1_0::implementation::AprdCallback;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using ::android::hardware::hidl_death_recipient;

//using ::android::hardware::hidl_array;
//using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
//using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using android::sp;
using android::status_t;
using android::OK;
using android::wp;

using namespace std;

struct MyDeathRecipient : hidl_death_recipient {
    virtual void serviceDied(uint64_t cookie, const wp<::android::hidl::base::V1_0::IBase>& who) {
       // Deal with the fact that the service died
       ALOGE("IAprdInfoSync service died ,exit");
       exit(0);
    }
};

//class AprdCallback
AprdCallback::AprdCallback(void *p)
{
    VendorXml=(VendorXmlThread*)p;
}

AprdCallback::~AprdCallback()
{
    VendorXml=NULL;
}

Return<void> AprdCallback::triger(const hidl_string& cmd, const ::vendor::sprd::hardware::aprd::V1_0::e_info_V& ei)
{
    int pid, size;
    vendor::sprd::hardware::aprd::V1_0::e_info_V l_info=ei;
    pid=getpid();
    size=VendorXml->vendor_e_q.size();
    ALOGE("AprdCallback::triger cmd=%s, e_info_V=%d, data=%s", cmd.c_str(), ei.et, (char*)ei.private_data.c_str());
    while(size>30)
    {
        ALOGE("AprdCallback::triger queue.size=%d, waite for 5s",size);
        sleep(5);
    }
    VendorXml->vendor_e_q.push(l_info);
    sem_post(VendorXml->vsem); 
    return Void();
}

//class VendorSyncThread
VendorSyncThread::VendorSyncThread(void* VendorXml)
{
    mVendorXml = VendorXml;
}

VendorSyncThread::~VendorSyncThread()
{
    mVendorXml = NULL;
}

void VendorSyncThread::Setup()
{
    //To Do
    APR_LOGD("VendorSyncThread::Setup()\n");
}

void VendorSyncThread::Execute(void* arg)
{
    sp<IAprdInfoSync> service = NULL;
    configureRpcThreadpool(1, true /*callerWillJoin*/);

    sp<IAprdCallback> callback= new AprdCallback(VendorSyncThread::mVendorXml);
    sp<MyDeathRecipient>  deathRecipient= new MyDeathRecipient();
    while(true){
        ALOGE("IAprdInfoSync getService loop");
        service = IAprdInfoSync::getService();
        if (service != NULL){
            ALOGE("IAprdInfoSync have got Service pFunc");
            service->setCallback(callback);
            service->linkToDeath(deathRecipient, 1);
            break;
        }else{
            ALOGE("IAprdInfoSync getService failed");
            sleep(1);
            continue;
        }
    }
    ALOGE("IAprdInfoSync getService success");
    joinRpcThreadpool();
    return;
}
