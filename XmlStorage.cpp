#include <stdlib.h>
#include<string.h>
#include <stdio.h>
#include "common.h"
#include "XmlStorage.h"
#include "AprData.h"
#include <string>

#define MY_ENCODING "utf-8"
#define EXPNUMCLR 0
#define EXPNUMSYNC 1
pthread_mutex_t xmlcount_mutex = PTHREAD_MUTEX_INITIALIZER;

XmlStorage::XmlStorage(Observable *o, char* pdir, char* pfname):Observer(o)
{
    APR_LOGD("XmlStorage::XmlStorage()\n");
    strcpy(m_dir, pdir);
    strcpy(m_pathname, pdir);
    strcat(m_pathname, "/");
    strcat(m_pathname, pfname);
    strcpy(m_bakFile, pdir);
    strcat(m_bakFile, "/");
    strcat(m_bakFile, "apr.xml.bak");


    pthread_mutex_init(&m_mutex, NULL);
    this->init();
}

XmlStorage::~XmlStorage()
{
    APR_LOGD("XmlStorage::~XmlStorage()\n");
    m_dir[0] = '\0';
    m_pathname[0] = '\0';
    m_bakFile[0] = '\0';
    pthread_mutex_destroy(&m_mutex);
}

xmlNodePtr XmlStorage::adjExpNode(xmlNodePtr aprNode)
{
#ifdef APR_CMCC_ONLY
    return aprNode;
#else
    xmlNodePtr curNode=xmlLastElementChild(aprNode);
    if(curNode==NULL){
        return NULL;
    }
    if(xmlStrcmp(BAD_CAST "exceptions", curNode->name)!=0){
        if((curNode=xmlAddChild(aprNode, xmlNewNode(NULL,BAD_CAST "exceptions"))) != NULL){
            return curNode;
        }else{
            return NULL;
        }
    }else{
        return curNode;
    }
#endif
}

void XmlStorage::adjExpNodeNum(int opt)
{
    xmlDocPtr doc=NULL;
    xmlNodePtr rootNode=NULL;
    xmlNodePtr aprNode=NULL;
    xmlNodePtr expNode=NULL;
    xmlNodePtr curNode=NULL;
    int ret=0;
    char value[PROPERTY_VALUE_MAX];

    if(opt==EXPNUMCLR){
        APR_LOGD("adjExpNodeNum EXPNUMCLR");
        property_set("persist.sys.apr.exceptionnode", "0");
    }
    if(opt==EXPNUMSYNC){
        APR_LOGD("adjExpNodeNum EXPNUMSYNC");
        doc = xmlParseFile(m_pathname);
        rootNode = xmlDocGetRootElement(doc);
#ifdef APR_CMCC_ONLY
        expNode = rootNode;
#else
        aprNode = xmlLastElementChild(rootNode);
        expNode = xmlLastElementChild(aprNode);
        if(expNode==NULL||xmlStrcmp(BAD_CAST "exceptions", expNode->name)!=0){
            sprintf(value, "%d", 0);
            goto SETPROP;
        }
#endif
        curNode=expNode->children;
        while (curNode != NULL&&curNode->name!=NULL) {
            if (curNode->type == XML_ELEMENT_NODE &&
                (xmlStrcmp(curNode->name, BAD_CAST "entry")==0||
                xmlStrcmp(curNode->name, BAD_CAST "exception")==0)){
                ret++;
            }
            curNode = curNode->next;
        }
        sprintf(value, "%d", ret);
SETPROP:
        property_set("persist.sys.apr.exceptionnode", value);
        if (doc!=NULL)
        {
            xmlFreeDoc(doc);
            doc = NULL;
        }
    }
}
xmlDocPtr XmlStorage::creatNewAprDoc()
{
    xmlDocPtr doc=NULL;
    xmlNodePtr rootNode=NULL;
    xmlNodePtr aprNode=NULL;
    
    adjExpNodeNum(EXPNUMCLR);
    // document pointer
    doc = xmlNewDoc(BAD_CAST"1.0");
    // root node pointer
    #ifdef APR_CMCC_ONLY
    rootNode = xmlNewNode(NULL, BAD_CAST"apr");  /*CMCC mode <apr></apr> is root node and without child context before <exception></exception>*/
    #else
    rootNode = xmlNewNode(NULL, BAD_CAST"aprs");
    createAprNode(&aprNode);
    #endif
    
    xmlDocSetRootElement(doc, rootNode);

    if(NULL!=aprNode)
        xmlAddChild(rootNode, aprNode);

    return doc;
}

void XmlStorage::init()
{
    APR_LOGD("XmlStorage::init()\n");
    int ret=0;
    AprData* aprData = static_cast<AprData *>(m_observable);
    char *default_value = (char*)"unknown";
    string v;
    char* pbadfile;

    xmlDocPtr doc;
    xmlNodePtr rootNode;
    xmlNodePtr aprNode;
    xmlNodePtr expNode;

    APR_LOGD("m_bakFile %s", m_bakFile);
    if (!_fileIsExist(m_bakFile))
    {
        int bakfd = -1;
        bakfd = open(m_bakFile, O_RDONLY | O_CREAT, 0660);
        if (bakfd < 0)
        {
            APR_LOGD("Open apr.xml.bak fail: %s", strerror(errno));
            exit(1);
        }
        APR_LOGD("open %s fd is %d error %s", m_bakFile,bakfd, strerror(errno));
        close(bakfd);
    }
    // file exist
    if (_fileIsExist(m_pathname))
    {
        xmlKeepBlanksDefault(0); // libxml2 global variable.
        xmlIndentTreeOutput = 1; // indent .with \n
        doc = xmlParseFile(m_pathname);//, MY_ENCODING, XML_PARSE_RECOVER);
        if (NULL == doc) {
            if (_fileIsExist(m_bakFile)){
                _copy_file(m_pathname, m_bakFile);
                doc = xmlParseFile(m_pathname);
                if ( NULL == doc ){
                    doc = creatNewAprDoc();/*hyli rewrite*/
                }
                else{
                    adjAprNode(doc ,E_MAXNUM);
                    adjExpNodeNum(EXPNUMSYNC);
                    goto finish;
                }
            }
            else{
                doc = creatNewAprDoc();
            }
        }
        else{
            #ifdef APR_CMCC_ONLY
            if  (0)
            #else
            if  (checkRadioMode(doc, aprData))
            #endif
            {
                xmlFreeDoc(doc);
                doc = creatNewAprDoc();
            }else{
                //<aprs>
                rootNode = xmlDocGetRootElement(doc);
                // sysdump ??

                #ifdef APR_CMCC_ONLY
                aprNode=rootNode;
                adjExpNodeNum(EXPNUMSYNC);
                #else
                adjExpNodeNum(EXPNUMCLR);
                createAprNode(&aprNode);
                xmlAddChild(rootNode, aprNode);
                #endif
                v = aprData->getBootMode();
                if (strcmp(v.c_str(), default_value))
                {
                    if (aprNode != NULL)
                    {
                        APR_LOGD("last reboot is %s", v.c_str());
                        char value[PROPERTY_VALUE_MAX];
                        xmlNodePtr entryNode;
                        char lifetime[PROPERTY_VALUE_MAX];
                        //struct e_info *p_ei = (struct e_info*)arg;
                        // get wall clock time as timestamp
                        aprData->getWallClockTime(value, sizeof(value));
                        property_get("persist.sys.apr.lifetime",lifetime,"0");
                        expNode=adjExpNode(aprNode);
                        if(expNode!=NULL)
                            createAppEntryNode(expNode, value, lifetime,"reboot", v.c_str(), NULL);
                        else
                            APR_LOGD("etNode is NULL");
                    }
                }

            }
        }

    } 
    else{
        doc = creatNewAprDoc();
    }

    ret = xmlSaveFormatFileEnc(m_pathname, doc, MY_ENCODING, 1);
finish:
    if (doc!=NULL)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }
    if (ret != -1) {
        APR_LOGD("%s file is created\n", m_pathname);
    } else {
        APR_LOGE("xmlSaveFormatFile failed\n");
        exit(1);
    }
}

int XmlStorage::checkRadioMode(xmlDocPtr mdoc, AprData * apr_data)
{
    xmlNodePtr mrootNode;
    xmlNodePtr maprNode;

    unsigned char a_radioMode_tmp[PROPERTY_VALUE_MAX];
    unsigned char a_radioMode[PROPERTY_VALUE_MAX];
    memset(a_radioMode_tmp, 0, PROPERTY_VALUE_MAX);
    memset(a_radioMode, 0, PROPERTY_VALUE_MAX);

    //memcpy(a_radioMode, apr_data->getRadioMode(), strlen(apr_data->getRadioMode()));
    apr_data->getRadioMode((char *)a_radioMode);
    if(strcmp((const char *)a_radioMode, "unknown"))
        return 0;

    mrootNode = xmlDocGetRootElement(mdoc);
    maprNode = xmlLastElementChild(mrootNode);
    _xmlGetContentByName(maprNode, (char*)"RadioMode", (char *)a_radioMode_tmp);

    int ret;
    ret = strcmp((const char *)a_radioMode, (const char *)a_radioMode_tmp);

    return ret;
}

int XmlStorage::_copy_file(const char *dest_file, const char * src_file)
{
    return copy_file(dest_file, src_file, 0);
}

void XmlStorage::removeNode(xmlNode *parentNode, xmlNode *nodeToDelete) {
    if (nodeToDelete == NULL) {
        APR_LOGD("error: nodeToDelete is null");
        return;
    }
    //APR_LOGD("parentNode = %s,nodeToDelete:%s",parentNode->name,nodeToDelete->name);

    xmlNodePtr childNode = nodeToDelete->xmlChildrenNode;
    xmlNodePtr curNode = NULL;
    while (childNode != NULL) {
        curNode = childNode;
        childNode = childNode->next;
        if (curNode != NULL) {
            xmlUnlinkNode(curNode);
            xmlFreeNode(curNode);
            curNode = childNode;
        }else{
            APR_LOGD("no child node");
            break;
        }
    }
    xmlUnlinkNode(nodeToDelete);
    xmlFreeNode(nodeToDelete);
}

int XmlStorage::listRmXmlNode(xmlNodePtr node)
{
    APR_LOGD("listRmXmlNode");
    if (node != NULL){
        APR_LOGD("aprNode not null,aprNode -%s",node->name);
        xmlNodePtr exNode = node->children;
        if (exNode != NULL)
            removeNode(node, exNode);
    }
    return 0;
}

int XmlStorage::createAprNode(xmlNodePtr *node)
{
    string v;
    char value[PROPERTY_VALUE_MAX];
    int len=0;
    char tmp[2048];
    AprData* aprData = static_cast<AprData *>(m_observable);
    unsigned char uuid[aprData->UUID_LEN];
    unsigned char a_radioMode[PROPERTY_VALUE_MAX];

#ifdef APR_CMCC_ONLY
    return 0;
#endif
    // <apr>
    xmlNodePtr aprNode = xmlNewNode(NULL,BAD_CAST"apr");
    //     <hardwareVersion> </hardwareVersion>
    v = aprData->getHardwareVersion();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "hardwareVersion", BAD_CAST v.c_str());
    //     <SN> </SN>
    v = aprData->getSN();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "SN", BAD_CAST v.c_str());
    //     <imei> </imei>
    v = aprData->getIMEIs();
    xmlNewTextChild(aprNode,NULL,BAD_CAST "imei",BAD_CAST v.c_str());

    //     <UUID> </UUID>
    memset(uuid,0,AprData::UUID_LEN);
    memset(a_radioMode, 0, PROPERTY_VALUE_MAX);
    if (!_fileIsExist(m_pathname)) {
        memcpy(uuid, aprData->getUUID(), aprData->UUID_LEN);
    }
    else{
        xmlDocPtr doc;
        xmlNodePtr rootNodeTmp;
        xmlNodePtr aprNodeTmp;
        doc = xmlParseFile(m_pathname);//, MY_ENCODING, XML_PARSE_RECOVER);
        if (NULL != doc)
        {
            rootNodeTmp = xmlDocGetRootElement(doc);
            aprNodeTmp = xmlLastElementChild(rootNodeTmp);
            _xmlGetContentByName(aprNodeTmp, (char*)"UUID", (char *)uuid);
            xmlFreeDoc(doc);
            APR_LOGD("xmlParseFile doc is NULL.");
        }
        else
        {
            memcpy(uuid, aprData->getUUID(), aprData->UUID_LEN);
        }
    }
    xmlNewTextChild(aprNode, NULL, BAD_CAST "UUID", BAD_CAST uuid);

    //     <buildNumber> </buildNumber>
    v = aprData->getBuildNumber();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "buildNumber", BAD_CAST v.c_str());
    //     <CPVersion> </CPVersion>
    v = aprData->getCPVersion();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "CPVersion", BAD_CAST v.c_str());
    //     <CP2Version> </CP2Version>
    v = aprData->getCP2Version();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "CP2Version", BAD_CAST v.c_str());
    //     <GPSVersion> <GPSVersion>
    v = aprData->getGEVersion();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "GPSVersion", BAD_CAST v.c_str());
    //     <extraInfo> </extraInfo>
    v = aprData->getExtraInfo();
    xmlNewTextChild(aprNode, NULL, BAD_CAST "extraInfo", BAD_CAST v.c_str());

    //     <modifyTime> </ modifyTime >
    char time_str[128];
    aprData->getPAC_Time(time_str, 128);
    xmlNewTextChild(aprNode, NULL, BAD_CAST "modifyTime", BAD_CAST time_str);

    //     <RadioMode> </RadioMode>
    aprData->getRadioMode((char *)a_radioMode);
    //memcpy(a_radioMode, aprData->getRadioMode(), strlen(aprData->getRadioMode()));
    xmlNewTextChild(aprNode, NULL, BAD_CAST "RadioMode", BAD_CAST a_radioMode);
    
    //     <startTime> </startTime>
    //     <endTime> </endTime>
    aprData->getUpTime(value);
    xmlNewTextChild(aprNode, NULL, BAD_CAST "startTime", BAD_CAST value);
    
    aprData->getUpTime(value);
    xmlNewTextChild(aprNode, NULL, BAD_CAST "endTime", BAD_CAST value);
    //     <lifetime> </lifetime>
    property_get("persist.sys.apr.lifetime",value,"0");
    xmlNewTextChild(aprNode, NULL, BAD_CAST "lifetime", BAD_CAST value);
    //     <exceptions> </exceptions>
    //m_exceptionsNode = NULL;
    // </apr>
    *node = aprNode;

    return 0;
}
int XmlStorage::syncNodes(xmlDocPtr doc, xmlChar*NodeName, xmlChar* content)
{
    //xmlDocPtr doc = NULL;
    xmlXPathObjectPtr result;  
    xmlXPathContextPtr context;
    xmlChar* xpath_exp;
    xmlNodeSetPtr nodeset = NULL;
    xmlChar* value;
    int i=0;

    //doc = xmlParseFile(m_pathname);
    if(doc==NULL)
    {
        APR_LOGD("hyli in syncNodes doc=NULL");
        return -1;
    }

    xpath_exp=NodeName;
    
    context = xmlXPathNewContext(doc);
    result = xmlXPathEvalExpression(xpath_exp, context);
    xmlXPathFreeContext(context);
    
    if(NULL != result) {
        nodeset = result->nodesetval;  
        for(i = 0; i < nodeset->nodeNr; i++)
        {
            value=xmlNodeGetContent(nodeset->nodeTab[i]);
            APR_LOGD("hyli in syncNodes curNode.name %s, content %s", nodeset->nodeTab[i]->name, value);
            if(xmlStrcmp(BAD_CAST "unknown", value)==0) //if(xmlStrcmp(content, value)!=0)
            {
                xmlNodeSetContent(nodeset->nodeTab[i], content);
            }
            xmlFree(value);  
        }
        xmlXPathFreeObject(result);
    }
    else{
        APR_LOGD("hyli in syncNodes result=NULL");
        return -1;
    }
   //xmlFreeDoc(doc);
   return 0;
}

void XmlStorage::XmlWriteVendor(xmlDocPtr doc, const char* prop, const char* NodeName, int writesync)
{
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    string tmp;
    int ret=-1, index;
    
    property_get(prop, value, default_value);
    tmp=value;
    if(tmp.compare(default_value)!=0)
    {
        if((index = tmp.find("@", 0)) != string::npos)
        {
            tmp.erase(index);
        }
        //tmp1+=tmp;
        APR_LOGD("hyli syncNodes %s %s", NodeName, tmp.c_str());
        ret=syncNodes(doc, BAD_CAST NodeName, BAD_CAST tmp.c_str());
        if(writesync==1&&ret==0)
        {
            tmp+="@sync";
            property_set(prop, tmp.c_str());
            APR_LOGD("hyli @sync MODEM_PROPGET, tmp=%s", tmp.c_str());
        }
    }
}

int XmlStorage::adjAprNode(xmlDocPtr doc, EExceptionType et)
{
    int isI_E=0, index;

    APR_LOGD("hyli EExceptionType %d", et);
    switch(et){
        case I_MODEMVER_GET:
            isI_E=1;
            goto MODEM_PROPGET;
        case I_IMEI_GET:
            isI_E=1;
            goto IMEI_PROPGET;
        case I_CP2VER_GET:
            isI_E=1;
            goto CP2_PROPGET;
        case I_GPSVER_GET:
            isI_E=1;
            goto GPS_PROPGET;
        case I_MODEMMODE_FLASH:
            isI_E=1;
            goto MODEMMODE_FLASH;
        case E_MAXNUM:
            isI_E=0;
            goto ALL_PROPGET;
        default:
            return 0;
    }

ALL_PROPGET:
    APR_LOGD("hyli step ALL_PROPGET, isI_E=%d", isI_E);
MODEM_PROPGET:
    APR_LOGD("hyli step MODEM_PROPGET, isI_E=%d", isI_E);
    //     <CPVersion> </CPVersion>
    XmlWriteVendor(doc, "persist.apr.cpversion", "//CPVersion", isI_E);
    if(isI_E==1)
        goto FINISH;

IMEI_PROPGET:
    APR_LOGD("hyli step IMEI_PROPGET, isI_E=%d", isI_E);
    //     <imei> </imei>
    XmlWriteVendor(doc, "persist.apr.imeis", "//imei", isI_E);
    if(isI_E==1)
        goto FINISH;

CP2_PROPGET:
    APR_LOGD("hyli step CP2_PROPGET, isI_E=%d", isI_E);
    //     <CP2Version> </CP2Version>
    XmlWriteVendor(doc, "persist.apr.cp2version", "//CP2Version", isI_E);
    if(isI_E==1)
        goto FINISH;

GPS_PROPGET:
    APR_LOGD("hyli step GPS_PROPGET, isI_E=%d", isI_E);
    //     <GPSVersion> </GPSVersion>
    XmlWriteVendor(doc, "persist.apr.gpsversion", "//GPSVersion", isI_E);
    if(isI_E==1)
        goto FINISH;

MODEMMODE_FLASH:
    APR_LOGD("hyli step MODEMMODE_FLASH, isI_E=%d", isI_E);
    //     <GPSVersion> </GPSVersion>
    XmlWriteVendor(doc, "persist.apr.radiomode", "//RadioMode", isI_E);
    if(isI_E==1)
        goto FINISH;

FINISH:
    return 0;
}

int XmlStorage::createAppEntryNode(xmlNodePtr node, const char* ts, char* lifetime, const char* type, const char* s_an1, const char* s_an2)
{
    //APR_LOGD("createAppEntryNode begin!");
    int exception_node_number = 0;
    string v;
    char value[PROPERTY_VALUE_MAX];
    char tmp[2048];
    int len=0;
    AprData* aprData = static_cast<AprData *>(m_observable);

    property_get("persist.sys.apr.exceptionnode", value, "0");
    exception_node_number = atoi(value);
    
 #ifdef APR_CMCC_ONLY
    #define EXNODE "exception"
    if(!aprData->isUserdebug()){
        APR_LOGD("exception_node_number = %d",exception_node_number);
        while( exception_node_number >= 20 ){
            listRmXmlNode(node);
            exception_node_number --;
        }
    }
#else
    #define EXNODE "entry"
#endif

    xmlNodePtr expNode = xmlNewNode(NULL,BAD_CAST EXNODE);

#ifdef APR_CMCC_ONLY

    //  <imei> </imei>
    v = aprData->getIMEIs();
    xmlNewTextChild(expNode,NULL,BAD_CAST "imei",BAD_CAST v.c_str());

    //  <softwareversion> </softwareversion>
    v = aprData->getBuildNumber();
    xmlNewTextChild(expNode, NULL, BAD_CAST "softwareversion", BAD_CAST v.c_str());
#endif
    //     <timestamp> </timestamp>
    xmlNewTextChild(expNode, NULL, BAD_CAST "timestamp", BAD_CAST ts);
    //     <type> </type>
    xmlNewTextChild(expNode, NULL, BAD_CAST "type", BAD_CAST type);
   //      <lifetime> </lifetime> in debug
#ifdef APR_CMCC_ONLY
    property_get("persist.sys.apr.reportlevel", value, "1");
    if(strcmp(value, "2") != 0 )
        xmlNewTextChild(expNode, NULL, BAD_CAST "lifetime", BAD_CAST lifetime);
#endif

    //     <mem_oversize> </mem_oversize>
    if(NULL!=s_an2)
        xmlNewTextChild(expNode, NULL, BAD_CAST "mem_oversize", BAD_CAST s_an2);

    if(NULL!=s_an1){
#ifdef APR_CMCC_ONLY
        //     <crashappname> </crashappname> CMCC mode
        xmlNewTextChild(expNode, NULL, BAD_CAST "crashappname", BAD_CAST s_an1);
#else
        //     <brief> </brief> copy of <crashappname> </crashappname>. sprd mode
        xmlNewTextChild(expNode, NULL, BAD_CAST "brief", BAD_CAST s_an1);
#endif
        //APR_LOGD("createAppEntryNode end!");
    }
    if(xmlAddChild(node, expNode) == NULL)
        APR_LOGD("***********rr is NULL");
    //*node = aprNode;
    exception_node_number ++;
    sprintf(value, "%d", exception_node_number);
    //APR_LOGD("createnode end exception node =%s", value);
    property_set("persist.sys.apr.exceptionnode", value);
    return 0;
}



void XmlStorage::handleEvent()
{

}

void XmlStorage::handleEvent(void* arg)
{
    APR_LOGD("XmlStorage::handleEvent()\n");
    int ret_et=0,ret_doc=0, isInfoEvent=0;
    char value[PROPERTY_VALUE_MAX];
    char lifetime[PROPERTY_VALUE_MAX];
    char* s_an1=NULL, *s_an2=NULL;
    AprData* aprData = static_cast<AprData *>(m_observable);
    char *default_value = (char*)"unknown";
    char* pbadfile;

    xmlDocPtr doc;
    xmlNodePtr rootNode;
    xmlNodePtr aprNode;
    xmlNodePtr etNode;
    xmlNodePtr tmpNode;
    // lock
    pthread_mutex_lock(&m_mutex);

    xmlKeepBlanksDefault(0); // libxml2 global variable.
    xmlIndentTreeOutput = 1; // indent .with \n
    if (!_fileIsExist(m_pathname)) {
        APR_LOGD("%s isn't exist!\n", m_pathname);
        doc = creatNewAprDoc();
    }
    else{
        doc = xmlParseFile(m_pathname);
        if (NULL == doc) {
            if (_fileIsExist(m_bakFile)){
                _copy_file(m_pathname, m_bakFile);
                
                doc = xmlParseFile(m_pathname);
                if ( NULL == doc ){
                    doc=creatNewAprDoc();
                }
                else{
                    adjAprNode(doc, E_MAXNUM);
                    adjExpNodeNum(EXPNUMSYNC);
                }
            }
            else{
                doc = creatNewAprDoc();
            }
        }
    }
    if (NULL == doc) {
        APR_LOGD("doc is NULL");
        goto RELEASE_RES;
        return;
    }
    property_get("persist.sys.apr.lifetime",lifetime,"0");
    rootNode = xmlDocGetRootElement(doc);
#ifdef APR_CMCC_ONLY
    aprNode = rootNode;
#else
    aprNode = xmlLastElementChild(rootNode);

    if (NULL == aprNode) {
        APR_LOGD("aprNode is NULL");
        goto RELEASE_RES;
        return;
    }
    APR_LOGD("aprNode content %s", aprNode->content);
    /*<endTime></endTime>*/
    tmpNode = _xmlGetContentByName(aprNode, (char*)"endTime", NULL);
    if (NULL != tmpNode) {
        aprData->getUpTime(value);
        APR_LOGD("set endTime: %s", value);
        xmlNodeSetContent(tmpNode, (const xmlChar*)value);
    }
    
    /*<lifetime></lifetime>*/
    tmpNode = _xmlGetContentByName(aprNode, (char*)"lifetime", NULL);
    if (NULL != tmpNode) {
        APR_LOGD("set lifetime: %s", lifetime);
        xmlNodeSetContent(tmpNode, (const xmlChar*)lifetime);
    }
#endif
    if (arg) {

        // add <exception>...<exception> to rootNode
        xmlNodePtr entryNode;
        struct e_info *p_ei = (struct e_info*)arg;
        // get wall clock time as timestamp
        aprData->getWallClockTime(value, sizeof(value));

        if((etNode=adjExpNode(aprNode))==NULL){
        APR_LOGD("etNode is NULL");
        goto RELEASE_RES;
            return;
        }
        isInfoEvent=0;
        switch(p_ei->et ){
            case I_MODEMVER_GET:
            case I_IMEI_GET:
            case I_CP2VER_GET:
            case I_GPSVER_GET:
            case I_MODEMMODE_FLASH:
                    isInfoEvent=1;
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("I_XXX_GET=%d Happen!", p_ei->et);
                    break;
            case E_ANR:
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("ANR Happen!"); 
                    break;
            case E_NATIVE_CRASH:
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("NATIVE_CRASH Happen!");
                    break;
            case E_JAVA_CRASH:
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("JAVA_CRASH Happen!");
                    break;
            case E_RAM_USED:
            char* temp;
            s_an1=strtok_r((char*)(p_ei->private_data),"~", &temp);
            s_an2=strtok_r(NULL,"~", &temp);
                    APR_LOGD("RAM oversize Happen!");
                    break;
            case E_MODEM_ASSERT:
            case E_WCN_ASSERT:
            case E_MODEM_BLOCKED:
            case E_WCN_GE2:
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("Modem exp Happen!");
                    break;
            case E_SYSTEM_REBOOT:
                    s_an1=(char*)(p_ei->private_data);
                    APR_LOGD("System_server exp Happen!");
                    break;
            case E_BOOTMODE:
            case E_MAXNUM:
                    break;
            default:
                    ret_et=-1;
                    APR_LOGD("wrong exception type!");
                    break;
        }
        if(ret_et!=-1)
        {
            if(isInfoEvent==1)
                adjAprNode(doc, p_ei->et);
            else
                createAppEntryNode(etNode, value, lifetime, get_et_name(p_ei->et).c_str(), s_an1, s_an2);
        }
        APR_LOGD("*******p_ei->et %d errno : %s", p_ei->et, strerror(errno));
    }

    ret_doc = xmlSaveFormatFileEnc(m_pathname, doc, MY_ENCODING, 1);

RELEASE_RES:
    if (doc != NULL)
    {
        APR_LOGD("XmlStorage: free m_doc\n");
        xmlFreeDoc(doc);
    }
    /*
     * If you have to long press the power button to restart the phone,
     * Occasionally it's happened to empty the contents of the apr.xml file
     * being written by 'collect_apr' process. sync() will reduce the
     * probability.
     */
    /*
     * It's write-back to disk in global, so remove this sync() syscall
     * to avoid influence on other modules.
     */
    /* sync(); */
    if (ret_doc > 0) {
        int r;
        APR_LOGD("%s file is created\n", m_pathname);
        r =_copy_file(m_bakFile, m_pathname);
        APR_LOGD("_copy_file return %d, errno :%s", r, strerror(errno));
    } else if (ret_doc == -1){
        APR_LOGE("xmlSaveFormatFile failed because: %s\n",strerror(errno));
        exit(1);
    }

    // unlock
    pthread_mutex_unlock(&m_mutex);
}

int XmlStorage::_fileIsExist(const char *file)
{
    int retval;
    /* clear the umask */
    umask(0);
    // If directory is not exist, make dir
    if (access(m_dir, F_OK) < 0) {
        retval = mkdir(m_dir, S_IRWXU | S_IRWXG | S_IRWXO);
        if (retval < 0) {
            APR_LOGE("mkdir %s fail, error:%s\n", m_dir, strerror(errno));
            exit(1);
        }
    }

    // file not exist
    if (access(file, F_OK) < 0) {
        return false;
    } else {
        return true;
    }
}

xmlNodePtr XmlStorage::_xmlGetContentByName(xmlNodePtr rNode, char* name, char* value)
{
    xmlChar *szKey;
    xmlNodePtr nextNode = rNode->xmlChildrenNode;
    xmlNodePtr curNode = NULL;
    while (nextNode != NULL) {
        curNode = nextNode;
        nextNode = nextNode->next;
        if (name != NULL) {
            if (!xmlStrcmp(curNode->name, (const xmlChar*)name)) {
                if (value != NULL) {
                    szKey = xmlNodeGetContent(curNode);
                    strcpy(value, (char*)szKey);
                    xmlFree(szKey);
                }
                break;
            }
            curNode = nextNode;
        }
    }

    return curNode;
}


