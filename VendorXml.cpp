#include "VendorXml.h"

using namespace std;
#define APRVENDORINFOLEN 384

VendorXmlThread::VendorXmlThread(AprData* aprData, sem_t *pvsem)
{
    vsem=pvsem;
    m_aprData=aprData;
}
VendorXmlThread::~VendorXmlThread()
{
    m_aprData=NULL;
}

void VendorXmlThread::Setup()
{
    APR_LOGD("VendorXmlThread::Setup()\n");
}

//unknown

void VendorXmlThread::VInfoSync(const char* prop, EExceptionType et, const char *arg, int* flash, struct e_info &m_ei)
{
    //To Do
    int ret=0, index=0;
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    string tmp;


    property_get(prop, value, default_value);
    tmp=value;
    index = tmp.find("@", 0);
    if(index == string::npos || tmp.compare(0, index, arg)!=0)  //vendor info first got,  sync xml and write prop@unsync
    {
        *flash=1;
        tmp=arg;
        //m_aprData->setGPSVersion(arg);
        tmp+="@unsync";
        property_set(prop, tmp.c_str());
        m_ei.et = et;
        m_aprData->setChanged();
        m_aprData->notifyObservers((void*)&m_ei);
    }
    else
    {
        if((index = tmp.find("@unsync", 0)) != string::npos)  //vendor info have got yet, sync xml only
        {
            APR_LOGD("hyli in unsync %s\n", prop);
            m_ei.et = et;
            m_aprData->setChanged();
            m_aprData->notifyObservers((void*)&m_ei);
        }
        else if((index = tmp.find("@sync", 0)) != string::npos)  //xml sync have finished in anther thread, discare this triger.
        {
            APR_LOGD("hyli in synced %s\n", prop);
            goto PROCESSFINISH;
        }
        else
        {
            property_set(prop, default_value);
        }
    }

PROCESSFINISH:
    return;
}

void VendorXmlThread::CPXExpSync(e_info_V &p, struct e_info &m_ei)
{

    char tmp[APRVENDORINFOLEN];
    m_ei.et=(EExceptionType)p.et;
    if(APRVENDORINFOLEN-1 < strlen(p.private_data.c_str()))
    {
        APR_LOGD("CPXExpSync truncate CP Exp Info!\n");
    }
    strncpy(tmp, p.private_data.c_str(), APRVENDORINFOLEN-1);
    tmp[APRVENDORINFOLEN-1]='\0';
    m_ei.private_data=(void*)tmp;
    m_aprData->setChanged();
    m_aprData->notifyObservers((void*)&m_ei);
}

void VendorXmlThread::Execute(void* arg)//m_eis
{
    e_info_V peiV;
    int flash=0;

    while(1)
    {
        sem_wait(vsem);
        APR_LOGD("Vendor event posted!\n");
        if(vendor_e_q.empty()!=true)
        {
            peiV=vendor_e_q.front();
            switch(peiV.et){
                case EExceptionType_V::I_MODEMVER_GET:
                #ifndef APR_CMCC_ONLY
                    VInfoSync("persist.apr.cpversion", I_MODEMVER_GET, peiV.private_data.c_str(), &flash, m_eis[0]);
                    if(flash==1)
                        m_aprData->setGPSVersion(peiV.private_data.c_str());
                #endif
                    goto NEXTVE;
                case EExceptionType_V::I_IMEI_GET: //Needed by pld and cmcc both
                    VInfoSync("persist.apr.imeis", I_IMEI_GET, peiV.private_data.c_str(), &flash, m_eis[1]);
                    if(flash==1)
                        m_aprData->setIMEIs(peiV.private_data.c_str());
                    goto NEXTVE;
                case EExceptionType_V::I_CP2VER_GET:
                #ifndef APR_CMCC_ONLY
                    VInfoSync("persist.apr.cp2version", I_CP2VER_GET, peiV.private_data.c_str(), &flash, m_eis[2]);
                    if(flash==1)
                        m_aprData->setCP2Version(peiV.private_data.c_str());
                #endif
                    goto NEXTVE;
                case EExceptionType_V::I_GPSVER_GET:
                #ifndef APR_CMCC_ONLY
                    VInfoSync("persist.apr.gpsversion", I_GPSVER_GET, peiV.private_data.c_str(), &flash, m_eis[3]);
                    if(flash==1)
                        m_aprData->setCPVersion(peiV.private_data.c_str());
                #endif
                    goto NEXTVE;
                case EExceptionType_V::I_MODEMMODE_FLASH:
                #ifndef APR_CMCC_ONLY
                    VInfoSync("persist.apr.radiomode", I_MODEMMODE_FLASH, peiV.private_data.c_str(), &flash, m_eis[4]);
                    /*if(flash==1)
                        m_aprData->setCPVersion(peiV.private_data.c_str());*/
                #endif
                    goto NEXTVE;
                case EExceptionType_V::E_MODEM_ASSERT:
                case EExceptionType_V::E_WCN_ASSERT:
                case EExceptionType_V::E_MODEM_BLOCKED:
                case EExceptionType_V::E_WCN_GE2:
                    if(*(int*)arg)
                        CPXExpSync(peiV, m_eis[5]);
                    goto NEXTVE;
            }
       }
NEXTVE:
       vendor_e_q.pop();
    }
}



