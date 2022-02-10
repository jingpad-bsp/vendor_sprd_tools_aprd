/* create by lina.yang 2016.9.30*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "RamUsedThread.h"
#include "AprData.h"

static     int ram_used_change = 0;

RamUsedThread::RamUsedThread(AprData* aprData)
{
    m_aprData = aprData;
}

RamUsedThread::~RamUsedThread()
{
    m_aprData = NULL;
}


void RamUsedThread::Setup()
{
    APR_LOGD("RamUsedThread::Setup()\n");
}

void RamUsedThread::Execute(void* arg)
{
    APR_LOGD("RamUsedThread::Execute()\n");

    char buf[256];
    int time_limit = 180;
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    int flag_time_changed = 0;
    int *sw=(int*)arg;
    if(sw[0]==0&&sw[1]==0){
        return;
    }
    while(1){
        flag_time_changed = m_aprData->getTimeChangedFromSocket();
        property_get("persist.sys.apr.timechanged", value, default_value);
        time_limit =atoi( value);
        //APR_LOGD("in if changed_flag_time = %d,time_limit",flag_time_changed,time_limit);
        
        if(sw[1]==1){
            m_aprData->updateLifeTime();
        }
        if(sw[0]==1){
            property_get("persist.sys.apr.reportlevel", value, default_value);
            if(strcmp(value, "0") == 0 )
                update_RamOversized();
        }
        sleep(time_limit);
    }

}

int  RamUsedThread::get_RamUsed()
{

    //struct timespec ts;
    int fd, result;
    int memtotal = 0,memfree = 0,buffers = 0,cached = 0,ramused = 0;
    int numFound = 0;
    char buffer[MAX_LINE_LEN];
    char string[64];

    fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0)
    {
        APR_LOGD("get_RamUsed,open /proc/meminfo failed!errno=%d:%s",errno,strerror(errno));
        //return fd;
    }

    int len = read(fd, buffer, sizeof(buffer) -1);
    if (len < 0) {
        APR_LOGE("get_RamUsed read meminfo failed!errno=%d:%s",errno,strerror(errno));
        close(fd);
        // return;
    }
    close(fd);
    buffer[len] = 0;

    static const char* const tags[] = {
        "MemTotal:",
        "MemFree:",
        "Buffers:",
        "Cached:",
        "Shmem:",
        "Slab:",
        "SwapTotal:",
        "SwapFree:",
        "ZRam:",
        "Mapped:",
        "VmallocUsed:",
        "PageTables:",
        "KernelStack:",
        NULL
    };
    static const int tagsLen[] = {
        9,
        8,
        8,
        7,
        6,
        5,
        10,
        9,
        5,
        7,
        12,
        11,
        12,
        0
    };
    long mem[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    char* p = buffer;
    while (*p && numFound < (sizeof(tagsLen) / sizeof(tagsLen[0]))) {
        int i = 0;
        while (tags[i]) {
            if (strncmp(p, tags[i], tagsLen[i]) == 0) {
                p += tagsLen[i];
                while (*p == ' ') p++;
                char* num = p;
                while (*p >= '0' && *p <= '9') p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                }
                mem[i] = atoll(num);
                numFound++;
                break;
            }
            i++;
        }
        while (*p && *p != '\n') {
            p++;
        }
        if (*p) p++;
    }

    memtotal = mem[0];
    memfree = mem[1];
    buffers = mem[2];
    cached = mem[3];
    ramused = memtotal - memfree - buffers - cached;
   // sprintf(string,"%s",ramused);
    return ramused;
}

int RamUsedThread::getRamMaxUsedApp(char* appinfo)
{
    char buffer[2048]={0};
    char system_cmd_string[125] = {0};
    int ret = -1 ,fd = -1;
    char *s1=NULL,*s2=NULL,*s3=NULL,*temp=NULL;
    char pathname[125]={0};
    pid_t pid;
    FILE *fp_src;

    fd = open("/data/sprdinfo/apr_procrank.log",O_RDWR|O_CREAT, 0660);
    if (fd == -1 && (errno != EEXIST)) {
        APR_LOGE("creat /data/sprdinfo/apr_procrank.log failed.");
        close(fd);
        return -1;
    }
    if (fd >= 0 )
        close(fd);

    sprintf(pathname , "%s","/data/sprdinfo/apr_procrank.log");
    sprintf(system_cmd_string,  "%s %s","dumpsys meminfo >",pathname);
    // APR_LOGD("system_cmd_string =%s",system_cmd_string);
    apr_system_procrank(system_cmd_string);

    sleep(1);
    fp_src =fopen(pathname,"r");

    if (fp_src == NULL) {
        APR_LOGD("open file = %s  error=%s!",pathname,strerror(errno));
        return -1;
    }

    ret = fread(buffer,1 , sizeof(buffer),fp_src);
    if (ret < 0) {
        APR_LOGD("read   error!");
        fclose(fp_src);
        return -1;
    }
    //APR_LOGD("buffer = %s",buffer);
    if((s1 = strstr(buffer,"Total PSS")) != NULL)
    {
         APR_LOGD("Find Total PSS");
         if ((s1 = strstr(s1,"K:")) != NULL)
         {
             APR_LOGD("K:");
             if ((s1 = strstr(s1," ")) != NULL)
             {
                 APR_LOGD("Find buffer finished s1=%s", s1);
                 if((s2 = strtok_r(s1,")", &temp))!= NULL)
                 {
                     sprintf(s3,"%s%s",s2,")");
                 }
                 else
                 {
                     APR_LOGD("find buffer unfinished ! ");
                     fclose(fp_src);
                     return -1;
                 }
             }
             else
             {
                 APR_LOGD("getRamMaxUsedApp:Can not find blank");
                 fclose(fp_src);
                 return -1;
             }
         }
         else
         {
             APR_LOGD("getRamMaxUsedApp:Can not find K:");
             fclose(fp_src);
             return -1;
         }
    }
    else
    {
         APR_LOGD("getRamMaxUsedApp:Can not find Total PSS");
         fclose(fp_src);
         return -1;
    }
    fclose(fp_src);
    strcpy(appinfo, s3);
    return 0;

}

int RamUsedThread::update_RamOversized()
{
    int ram_limit = 800 ;
    char string[2048] = {0};
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";

    property_get("persist.sys.apr.rlchanged", value, default_value);
    ram_limit =atoi( value);
    int ram_used = get_RamUsed();
    //APR_LOGD("ram_used is %d",ram_used);
    if(ram_used > ram_limit*1024)// >ram_limit
    {
        ram_used_change++;
        APR_LOGD("ram_used_change is %d",ram_used_change);
        if (ram_used_change > 3)
        {
            if(getRamMaxUsedApp(string)<0){
                ram_used_change=0;
                return -1;
            }
            // Ram OverSize
            if(strlen(string)<2000)
                sprintf(m_buffer,"%d~%s~",ram_used,string);
            m_ei.et = E_RAM_USED;
            //APR_LOGD("RAM used oversize!ram_used = %s,ram_used_change= %d",string,ram_used_change);
            m_ei.private_data = (void*)m_buffer;
            m_aprData->setChanged();
            m_aprData->notifyObservers((void*)&m_ei);
            ram_used_change=0;
        }
    }
    return 0;
}
