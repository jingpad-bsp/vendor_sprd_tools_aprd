#ifndef VENDOR_SPRD_HARDWARE_APRD_V1_0_APRDCALLBACK_H
#define VENDOR_SPRD_HARDWARE_APRD_V1_0_APRDCALLBACK_H

#include <vendor/sprd/hardware/aprd/1.0/IAprdCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <vendor/sprd/hardware/aprd/1.0/types.h>
#include "VendorXml.h"
namespace vendor {
namespace sprd {
namespace hardware {
namespace aprd {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using vendor::sprd::hardware::aprd::V1_0::EExceptionType_V;

class AprdCallback : public IAprdCallback {
    // Methods from ::vendor::sprd::hardware::aprd::V1_0::IAprdCallback follow.
public:
    Return<void> triger(const hidl_string& cmd, const ::vendor::sprd::hardware::aprd::V1_0::e_info_V& ei) override;
    VendorXmlThread* VendorXml;
    AprdCallback(void *p);
    ~AprdCallback();
    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IAprdCallback* HIDL_FETCH_IAprdCallback(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace aprd
}  // namespace hardware
}  // namespace sprd
}  // namespace vendor

#endif  // VENDOR_SPRD_HARDWARE_APRD_V1_0_APRDCALLBACK_H
