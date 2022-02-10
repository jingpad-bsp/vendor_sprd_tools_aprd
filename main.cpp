#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include "common.h"
#include "confile.h"
#include "AprData.h"
#include "XmlStorage.h"
#include "InotifyThread.h"
#include "JavaCrashThread.h"
#include "AnrThread.h"
#include "NativeCrash.h"
//#include "ModemThread.h"
#include "RamUsedThread.h"
#include "SSRListener.h"
//#include "CPxInfoThread.h"
#include "VendorSync.h"

pthread_t  command_tid;
pthread_t  ramused_tid;

sem_t Gvsem;
int main(int argc, char *argv[])
{
    int retval;
    int swAnr = 1;
    int swNativeCrash = 1;
    int swJavaCrash = 0;
    int swModemAssert = 1;
    int swCrashClass = 1;
    int swRamUsed = 0;
    int swLifetime = 1;
    int swv[2]={0,1};
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";

    sem_init (&Gvsem, 0, 0);

    property_get("persist.sys.apr.enabled", value, default_value);
    APR_LOGD("MAIN: %s", value);
    //sleep(30); //data 分区？？ hyli delete 
    if (strcmp(value, "1"))
    {
        APR_LOGD("MAIN: persist.sys.apr.enabled is not 1.");
        property_set("persist.sys.apr.enabled", "0");
        while(1)
        {
            sleep(30);
            APR_LOGD("MAIN:set persist.sys.apr.enabled to 0. wait for exit");
        }
    }
    umask(0);
    //self_inspection_apr_enabled();

    INI_CONFIG* pIniCfg;
    parse_config();
    property_get("ro.debuggable", value, "");
    pIniCfg = ini_config_create_from_file(TMP_APR_CONFIG, 0);

    if (pIniCfg) {
        swAnr = ini_config_get_bool(pIniCfg, "exceptions", "anr", 1);
        swNativeCrash = ini_config_get_bool(pIniCfg, "exceptions", "nativeCrash", 1);
        swJavaCrash = ini_config_get_bool(pIniCfg, "exceptions", "javaCrash", 0);
        swModemAssert = ini_config_get_bool(pIniCfg, "exceptions", "modemAssert", 1);
        swCrashClass = ini_config_get_bool(pIniCfg, "exceptions", "crashClass", 1);
        swRamUsed = ini_config_get_bool(pIniCfg, "exceptions", "ramUsed", 0);
        swLifetime = ini_config_get_bool(pIniCfg, "exceptions", "lifeTime", 1);
        ini_config_destroy(pIniCfg);
    }
    //APR_LOGD("MAIN:swAnr%d,swNativeCrash%d,swJavaCrash%d,swModemAssert%d,swCrashClass%d,swRamUsed%d,swLifetime%d",swAnr,swNativeCrash,swJavaCrash,swModemAssert,swCrashClass,swRamUsed,swLifetime);
    AprData aprData;
    //Anr anr(&aprData);
    //InotifyThread anrThread(&anr);
    VendorXmlThread VendorXml(&aprData, &Gvsem);
    VendorSyncThread VendorSync((void*)&VendorXml);
    //ModemInfoThread modemInfo(&aprData);
    //WCNInfoThread wcnInfo(&aprData);
    //GeInfoThread geInfo(&aprData);

    AnrThread anrThread(&aprData);

    //JavaCrash javacrash(&aprData);
    //InotifyThread JavaCrashThread(&javacrash);

    NativeCrash nativeCrash(&aprData);
    InotifyThread nativeCrashThread(&nativeCrash);

    //ModemThread modemThread(&aprData);
    JavaCrashThread javaCrashThread(&aprData);
    SSRListener ssrListener(&aprData);
    RamUsedThread ramoversizeThread(&aprData);

    /* switch CrashClass */
    if (swCrashClass) {
        aprData.setBootModeSW(true);
    }

    XmlStorage xmlStorage(&aprData, (char*)"/data/sprdinfo", (char*)"apr.xml");
    aprData.addObserver(&xmlStorage);
    VendorXml.Start((void*)&swModemAssert);
    VendorSync.Start((void*)&swModemAssert);

    //modemInfo.Start(NULL);
    //wcnInfo.Start(NULL);
    //geInfo.Start(NULL);

    /* switch anr */
    if (swAnr) {
        anrThread.Start(NULL);
    }
    /* switch nativeCrash */
    if (swNativeCrash) {
        nativeCrashThread.StartWithPipe(NULL);
    }
    /* switch modemAssert
    if (swModemAssert) {
        modemThread.StartWithPipe(NULL);
    }*/
    /* switch javaCrash */
    if (swJavaCrash) { // hyli delete
        javaCrashThread.Start(NULL);
        ssrListener.Start(NULL);
    }
    /*switch ramoversizeThread when debug */
    if (swRamUsed || swLifetime) {
        swv[0]=swRamUsed;
        swv[1]=swLifetime;
        ramoversizeThread.Start((void*)swv);
    }
	memset_fosfns();
	traversal_Filename_size ("/data/tombstones", 1,1);
	APR_LOGD("MAIN:traversal_Filename_size\n");

     /*command handler for change time&limit */
     pthread_create(&command_tid, NULL, command_handler, NULL);

    while(1)
    {
        // waiting 60 seconds.
        sleep(1200);
        /* update the <endTime></endTime> for all Observers */
        aprData.setChanged();
        aprData.notifyObservers(NULL);
    }

    aprData.deleteObserver(&xmlStorage);
    return 0;
}

