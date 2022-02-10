#ifndef APRDATA_H
#define APRDATA_H

#include "Observable.h"
#include "Thread.h"
#include <string>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <uuid.h>
#include <common.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <libxml/parser.h>  
#include <libxml/xpath.h>

#define TOM_FILE_PATH "/data/tombstones/"
#define TOM_FILE_PATH1 "/data/tombstones/tombstone_00"
#define ANR_FILE_PATH "/data/anr"
#define BUF_LEN 64

enum EExceptionType {
    E_ANR,
    E_NATIVE_CRASH,
    E_JAVA_CRASH,
    E_MODEM_BLOCKED,
    E_MODEM_ASSERT,
    E_WCN_ASSERT,
    E_WCN_GE2,
    E_BOOTMODE,
    E_RAM_USED,
    E_SYSTEM_REBOOT,

    I_MODEMVER_GET,
    I_IMEI_GET,
    I_CP2VER_GET,
    I_GPSVER_GET,
    I_MODEMMODE_FLASH,
    E_MAXNUM,
};

struct e_info {
    EExceptionType et;
    void *private_data;
};

string get_et_name(EExceptionType et);

class AprData:public Observable
{
public:
    AprData();
    ~AprData();
    static const int UUID_LEN = 32;
    virtual void getSubjectInfo();

protected:
    void init();

public:
    // <hardwareVersion> </hardwareVersion>
    string getHardwareVersion();
    // <SN> </SN>
    string getSN();
    // <GPSVersion> <GPSVersion>
    string getGEVersion();
    // <buildNumber> </buildNumber>
    string getBuildNumber();
    // <CPVersion> </CPVersion>
    string getCPVersion();
    // <imei> </imei>
    string getIMEIs();
    // <CP2Version> </CP2Version>
    string getCP2Version();
    // <extraInfo> </extraInfo>
    string getExtraInfo();
    // <xxxTime> </xxxTime>
    int getStartTime(char *strbuf);
    int getUpTime(char* strbuf);
    // <timestamp></timestamp>
    int getWallClockTime(char* strbuf, size_t max, const char* format="%Y-%m-%d %H:%M:%S");
    /*int getRunTime(char* strbuf, const char* format="%Y-%m-%d %H:%M:%S");*/
    int getTimeChangedFromSocket();
    int SetTimeChangedFlag(int flag);

    int getRamLimitChangedFromSocket();
    int SetRamLimitChangedFlag(int max);

    // get ro.boot.mode
    string getBootMode();
    //update lifetime
    void updateLifeTime();
    //get ro.debuggable
    int isUserdebug();
    // set BootMode switch
    void setBootModeSW(bool sw);
    void setCPVersion(const char* p);
    void setIMEIs(const char* p);
    void setCP2Version(const char* p);
    void setGPSVersion(const char* p);

    //Get kinds of info from ext system:
    unsigned char  * getUUID();
    void getRadioMode(char *);
    int GetSerialno(char* p, int len);
    int GetBootMode(char* p, int len);
    void getPAC_Time(char* retbuf, int retlen);

    string getReload();
    int APR_system(char* cmd_string);

private:
    string m_hardwareVersion;
    string m_SN;
    string m_GPSVersion;
    string m_buildNumber;
    string m_CPVersion;
    string m_IMEIs;
    string m_CP2Version;
    string m_extraInfo;
    string m_bootMode;
    string m_Reload;
    int m_isuserdebug;
    int m_lifetime;
    bool m_swBM;
    int64_t m_sTime;

    unsigned char  m_uuid[UUID_LEN];
    void uuid_generation();
};

#endif
