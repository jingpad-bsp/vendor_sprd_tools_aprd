#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "common.h"
#include "AprData.h"
#include <cutils/properties.h>

pthread_mutex_t tel_mutex = PTHREAD_MUTEX_INITIALIZER;

string get_et_name(EExceptionType et)
{
    switch (et) {
    case E_ANR:
        return "anr";
    case E_NATIVE_CRASH:
        return "native crash";
    case E_JAVA_CRASH:
        return "java crash";
    case E_MODEM_BLOCKED:
        return "modem blocked";
    case E_MODEM_ASSERT:
        return "modem assert";
    case E_WCN_ASSERT:
        return "wcn assert";
    case E_WCN_GE2:
        return "wcn ge2";
    case E_RAM_USED:
        return "ram oversize";
    case E_SYSTEM_REBOOT:
        return "system reboot";
    case E_BOOTMODE:
        return "reboot";
    case I_MODEMVER_GET:
    case I_IMEI_GET:
    case I_CP2VER_GET:
    case I_GPSVER_GET:
    case I_MODEMMODE_FLASH:
        return "Info notify";
    default:
        return "unknown";
    }
}

AprData::AprData()
{
    init();
}

AprData::~AprData()
{

}

void AprData::getSubjectInfo()
{
    APR_LOGD("AprData::getSubjectInfo()\n");
    return;
}

//new GetBootMode for bug1165752
int AprData::GetBootMode(char* p, int len)
{ 
    char *default_value = (char*)"unknown";
    char value[PROPERTY_VALUE_MAX];
    char *ret;
    int ret1;
    int i;
	
    property_get("sys.boot.reason", value, default_value);
    APR_LOGD("sys.boot.reason: %s", value);
    ret = strstr(value,"," );
    if ( ret == NULL )
    {
        APR_LOGD("no reboot happened");
		return -1;

    }
    APR_LOGD("last reboot reason: %s", ret+1);
    char *normalreboot[] = 
    {
        "userrequested",
        "dialogreboot",
        "powerkey",
        "alarm",
        "shell",
        "adb",
    };
    for(i = 0; i < (sizeof( normalreboot )/sizeof(char*)); i++ )
    {
        ret1=strcmp(ret+1, normalreboot[i]);
        if (ret1==0)
        {
            APR_LOGD("last reboot is normalboot,rebootreason: %s", normalreboot[i]);
            return -1;
        }
    }
			
    if(len<=strlen(ret+1))
    {
        APR_LOGD("rebootreason is too long.\n");
        return -1;
    }
    
    strcpy(p, ret+1);
    return 0;
}


/*
int AprData::GetBootMode(char* p, int len)
{   
    char *cmdline;
    string tmp, tmp1("androidboot.mode=");
    regex tReg("\\s+");
    int num=0, ret=0, index=0, indexend=0;

    cmdline=read_Procfile_FreeBuffOut("/proc/cmdline", &num);
    if(cmdline==NULL)
    {
        APR_LOGD("Read cmdline file failed");
        return -1;
    }
    //APR_LOGD("get cmdline = %s\n", cmdline);
    tmp=cmdline;
    free(cmdline);

    if((index = tmp.find(tmp1, 0)) != string::npos)
    {
        index+=tmp1.length();
        if((indexend = tmp.find(' ', index)) != string::npos) //|| (indexend = tmp.find('\n', index)) != string::npos
        {
            tmp=tmp.substr(index, indexend-index);
        }
        else
        {
            tmp=tmp.substr(index);
        }
        tmp1=regex_replace(tmp.c_str(),tReg, "");
        APR_LOGD("androidboot.mode=%s", tmp1.c_str());
    }
    else
    {
        APR_LOGD("cmdline doesn't content bootmode!");
        return -1;
    }
    if(len<=tmp1.length())
    {
        APR_LOGD("get serialno dest not long enough\n");
        return -1;
    }
    //tmp1.copy(p, tmp1.length());
    strcpy(p, tmp1.c_str());
    return 0;
}
*/


int AprData::GetSerialno(char* p, int len)
{   
    char *cmdline;
    string tmp, tmp1("serialno=");
    regex tReg("[a-zA-Z\\d]+");//\\s+
    smatch s_result;
    int num=0, ret=0, index=0, indexend=0;

    cmdline=read_Procfile_FreeBuffOut("/proc/cmdline", &num);
    if(cmdline==NULL)
    {
        APR_LOGD("Read cmdline file failed");
        return -1;
    }
    //APR_LOGD("get cmdline = %s\n", cmdline);
    tmp=cmdline;
    free(cmdline);

    if((index = tmp.find(tmp1, 0)) != string::npos)
    {
        index+=tmp1.length();
        if((indexend = tmp.find(' ', index)) != string::npos) //|| (indexend = tmp.find('\n', index)) != string::npos
        {
            tmp=tmp.substr(index, indexend-index);
        }
        else
        {
            tmp=tmp.substr(index);
        }
        if(regex_search(tmp, s_result, tReg)==true)
        {
            tmp1=s_result[0].str();
            APR_LOGD("Serialno=%s", tmp1.c_str());
        }
        else{
            APR_LOGD("Can't match [a-zA-Z\\d]+ after \'Serialno=\'!");
            return -1;
        }
    }
    else
    {
        APR_LOGD("cmdline doesn't content Serialno!");
        return -1;
    }
    if(len<=tmp1.length())
    {
        APR_LOGD("get serialno dest not long enough\n");
        return -1;
    }
    //tmp1.copy(p, tmp1.length());
    strcpy(p, tmp1.c_str());
    return 0;
}

void AprData::init()
{
    APR_LOGD("AprData::init()\n");
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    string tmp;
    int index, ret=-1;

    //     <hardwareVersion> </hardwareVersion>
    property_get("ro.product.hardware", value, default_value);

    m_hardwareVersion.assign(value);
    //     <SN> </SN>
    ret=GetSerialno(value, PROPERTY_VALUE_MAX);
    if (0 == ret)
    {
        m_SN.assign(value);
    }
    else
    {
        m_SN.assign(default_value);
        APR_LOGD("APR get SN fail: %s\n", strerror(errno));
    }
    /*property_get("ro.boot.serialno", value, default_value);
    m_SN.assign(default_value);
    APR_LOGD("APR get SN fail: %s\n", strerror(errno));*/

    //     <buildNumber> </buildNumber>
    property_get("ro.build.description", value, default_value);
    m_buildNumber.assign(value);

    //     <CPVersion> </CPVersion>
    property_get("persist.apr.cpversion", value, default_value);
    tmp=value;
    if((index = tmp.find('@', 0)) != string::npos)
        tmp.erase(index);
    m_CPVersion.assign(tmp.c_str());
    
    //     <imei> </imei>
    property_get("persist.apr.imeis", value, default_value);
    tmp=value;
    if((index = tmp.find('@', 0)) != string::npos)
        tmp.erase(index);
    m_IMEIs.assign(tmp.c_str());

    //     <CP2Version> </CP2Version>
    property_get("persist.apr.cp2version", value, default_value);
    tmp=value;
    if((index = tmp.find('@', 0)) != string::npos)
        tmp.erase(index);
    m_CP2Version.assign(tmp.c_str());

    //     <GPSVersion> </GPSVersion>
    property_get("persist.apr.gpsversion", value, default_value);
    tmp=value;
    if((index = tmp.find('@', 0)) != string::npos)
        tmp.erase(index);
    m_GPSVersion.assign(tmp.c_str());

    property_get("ro.debuggable",value,"");
    m_isuserdebug = atoi(value);

    //update m_lifetime reboot only
    property_get("apr.flag.bootmode", value, default_value);
    //APR_LOGD("apr.flag..bootmode=%s",value);
    if (strcmp(value, "1") == 0) {
        property_get("persist.sys.apr.reload", value, (char*)"0");
        m_lifetime = atoi(value);
        //APR_LOGD("apr reload persist.sys.apr.reload=%s,m_lifetime=%d",value,m_lifetime);
    }else{
        property_get("persist.sys.apr.lifetime", value, (char*)"0");
        m_lifetime = atoi(value);
        property_set("persist.sys.apr.reload", value);
        //APR_LOGD("persist.sys.apr.lifetime=%s,m_lifetime=%d",value,m_lifetime);
    }
    if (m_lifetime == 0){
        updateLifeTime();
        APR_LOGD("lifetime will update");
    }

    property_get("ro.sprd.extrainfo", value, default_value);
    m_extraInfo.assign(value);

    // get ro.boot.mode
    property_get("apr.flag.bootmode", value, default_value);
    if (strcmp(value, "1")) {
        ret=GetBootMode(value, PROPERTY_VALUE_MAX);
        if (0 == ret)
        {
            m_bootMode.assign(value);
        }
        else
        {
            m_bootMode.assign(default_value);
            APR_LOGD("APR get bootMode fail: %s\n", strerror(errno));
        }
        property_set("apr.flag.bootmode", "1");
    }else{
        m_bootMode.assign(default_value);
    }
    m_swBM = false;
    uuid_generation();
}

string AprData::getHardwareVersion()
{
    return m_hardwareVersion;
}

string AprData::getSN()
{
    return m_SN;
}

string AprData::getGEVersion()
{
    return m_GPSVersion;
}

string AprData::getBuildNumber()
{
    return m_buildNumber;
}

string AprData::getCPVersion()
{
    return m_CPVersion;
}

string AprData::getIMEIs()
{
    return m_IMEIs;
}

string AprData::getCP2Version()
{
    return m_CP2Version;
}

void AprData::setCPVersion(const char* p)
{
    m_CPVersion.assign(p);
}

void AprData::setIMEIs(const char* p)
{
    m_IMEIs.assign(p);
}

void AprData::setCP2Version(const char* p)
{
    m_CP2Version.assign(p);
}

void AprData::setGPSVersion(const char* p)
{
    m_GPSVersion.assign(p);
}

string AprData::getExtraInfo()
{
    return m_extraInfo;
}

int AprData::getStartTime(char* strbuf)
{
    int ret = -1;
    if(strbuf) {
        sprintf(strbuf, "%lld", m_sTime);
        ret = 0;
    }

    return ret;
}
int AprData::getTimeChangedFromSocket()
{
    return getTimeChanged();

}

int AprData::SetTimeChangedFlag(int flag)
{
    setTimeChanged(flag);
    return 0;

}

int AprData::getRamLimitChangedFromSocket()
{
   return getRamLimitChanged();

}

int AprData::SetRamLimitChangedFlag(int max)
{
    setRamLimitChanged(max);
    return 0;

}

int AprData::APR_system(char* cmd_string)
{
    //APR_LOGD("cmd_string =%s",cmd_string);
    apr_system_procrank(cmd_string);
    return 0;
}
int AprData::getUpTime(char* strbuf)
{
    int64_t ut;
    uptime(strbuf);

    return 0;
}

int AprData::getWallClockTime(char* strbuf, size_t max, const char* format)
{
    getdate(strbuf, max, format);
    return 0;
}
#if 0
int AprData::getRunTime(char* strbuf, const char* format)
{
    gettime(strbuf, format);
    return 0;

}
#endif //unused
string AprData::getBootMode()
{
    string tmp;
    if (m_swBM) {
     APR_LOGD("m_bootMode %s!",m_bootMode.c_str());
        return m_bootMode;
    } else {
        APR_LOGD("m_swBM =0!");
        tmp.assign("unknown");
        return tmp;
    }
}

string AprData::getReload()
{
    return m_Reload;
}
void AprData::updateLifeTime()
{
    char value[PROPERTY_VALUE_MAX];
    int ret = 0;
    char buffer[MAX_LINE_LEN];

    memset((void*)buffer, 0, MAX_LINE_LEN);
    if(uptime(buffer)<0)
        return;

    sprintf(value,"%d",(m_lifetime+atoi(buffer)));
    //lifetime = (atoi(value)+atoi(strbuf));
    //APR_LOGD("value = %s",value);
    ret = property_set("persist.sys.apr.lifetime",value);
    if (ret){
        APR_LOGE("property_set failed ,ret = %d,error = %s!", ret, strerror(errno));
    }
}

int AprData::isUserdebug()
{

    return m_isuserdebug;

}
void AprData::setBootModeSW(bool sw)
{
    m_swBM = sw;
}

void AprData::uuid_generation()
{
    unsigned char  uuid_tmp[16];
    uuid_generate(uuid_tmp);
    int i;
    int tmp = 0;
    for (i = 0; i < 16; i ++)
    {
    tmp = uuid_tmp[i]/16;
    if (tmp > 9)
         m_uuid[i*2] = 65 + tmp - 10;
    else
        m_uuid[i*2] = 48 + tmp;

    tmp = uuid_tmp[i]%16;
    if (tmp > 9)
         m_uuid[i*2 + 1] = 65 + tmp - 10;
    else
        m_uuid[i*2 + 1] = 48 + tmp;

    /*
    if (i*3 + 2 == 47)
        m_uuid[i*3 + 2] = '\0';
    else
        m_uuid[i*3 + 2] = '-';
        */
    }
    m_uuid[AprData::UUID_LEN-1] = '\0';
    APR_LOGD("dong %s", m_uuid);
}

unsigned char * AprData::getUUID()
{
    return m_uuid;
}

void AprData::getRadioMode(char * value)
{
    string tmp;
    int index;
    char tmp1[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    property_get("persist.apr.radiomode", tmp1, default_value);
    tmp=tmp1;
    if((index = tmp.find('@', 0)) != string::npos)
        tmp.erase(index);
    strcpy(value, tmp.c_str());
}

//#define Miscdata_Path  "/dev/block/platform/soc/soc:ap-ahb/20600000.sdio/by-name/miscdata"
//#define Miscdata_Path    "/dev/block/platform/sdio_emmc/by-name/miscdata"
void AprData::getPAC_Time(char* retbuf, int retlen)
{
    char pTimeStr[128];
    FILE *fd = NULL;
    int try_access=0;
    int ret=-1;
    //0x01d2fccb82c9f580;
    unsigned long long time;
    char miscdata_path[128] = {0};
    char* tmp;

    if(retlen<=strlen("unknown"))
    {
        APR_LOGD("PAC_Time str buf too short");
        return;
    }

    if (-1 == property_get("ro.frp.pst", miscdata_path, "")){
        APR_LOGD("%s: get partitionpath fail\n", __FUNCTION__);
        strcpy(retbuf, "unknown");
        return;
    }
    
    if((tmp=strstr(miscdata_path, "by-name/"))!=NULL)
    {
        *(tmp+8)='\0'; //8=length of "by-name/", cut tail
        strcat(miscdata_path, "miscdata"); // xxxx/by-name/miscdata
    }
    else{
        APR_LOGD("%s: fail, may prop ro.frp.pst isn't include Miscdata_Path, should read from vendor prop\n", __FUNCTION__);
        return;
    }

    APR_LOGD("apr: miscdata: %s", miscdata_path);
    while(ret)
    {
        ret = access(miscdata_path,F_OK);
        APR_LOGD("apr: ret %d path %s",ret, miscdata_path);
        APR_LOGD("apr: %d",try_access);
        sleep(5);
        if((try_access++)>3)
        {
            strcpy(retbuf, "unknown");
            return;
        }
    }
    fd=fopen(miscdata_path,"r");
    if(NULL != fd)
    {
        fseek(fd,517*1024,SEEK_SET);
        fread(&time, sizeof(unsigned long long), 1, fd);
        fclose(fd);
        APR_LOGD("apr : trans time %lld",time);
        //time = 0x01d2fccb82c9f580;
        trans_time(time, retbuf, retlen);
        return;
    }
    else{
        APR_LOGD("open file failed\n");
    }
    strcpy(retbuf, "unknown");
    return;
}
