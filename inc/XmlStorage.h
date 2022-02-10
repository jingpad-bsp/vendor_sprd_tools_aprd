#ifndef XMLSTORAGE_H
#define XMLSTORAGE_H

#include <libxml/parser.h>
#include <libxml/tree.h>  
#include <libxml/xpath.h> 
#include <pthread.h>
#include <string>
#include "Observable.h"
#include "Observer.h"
#include "AprData.h"

class XmlStorage: public Observer
{
public:
    XmlStorage(Observable *o, char* pdir, char* pfname);
    ~XmlStorage();
    virtual void handleEvent();
    virtual void handleEvent(void* arg);
    void UpdateEndTime();
    void init();
    int checkRadioMode(xmlDocPtr, AprData *);

protected:
    xmlNodePtr _xmlGetContentByName(xmlNodePtr rNode, char* name, char* value);
    int syncNodes(xmlDocPtr doc, xmlChar*NodeName, xmlChar* content);
    void removeNode(xmlNode *parentNode, xmlNode *nodeToDelete);
    int listRmXmlNode(xmlNodePtr node);
    int createAprNode(xmlNodePtr *node);
    void XmlWriteVendor(xmlDocPtr doc, const char* prop, const char* NodeName, int writesync);
    int adjAprNode(xmlDocPtr doc, EExceptionType et); //et==E_MAXNUM adj all cp/cp2/gps nodes of whole apr.xml
    int createAppEntryNode(xmlNodePtr node, const char* ts, char* lifetime, const char* type, const char* s_an1, const char* s_an2);
    /*int createRamAppEntryNode(xmlNodePtr *node, const char* ts, const char* type,const char* lifetime,const char* s_an ,const char* ramoversize);*/
/*add hyli*/
    int _fileIsExist(const char *file);
    int _copy_file(const char *dest_file, const char * src_file);
    xmlDocPtr creatNewAprDoc();
    xmlNodePtr adjExpNode(xmlNodePtr aprNode);
    void adjExpNodeNum(int opt);
/*add*/
private:
    //xmlDocPtr m_doc;
    //xmlNodePtr m_rootNode;    /* root Node, <aprs></aprs> */
    //xmlNodePtr m_aprNode; /* <apr></apr> */
    //xmlNodePtr m_exceptionsNode; /* exceptions Node, <exceptions></exceptions> */

    //xmlNodePtr m_etNode;
/*add*/
    char m_dir[32];
    char m_pathname[48];
/*add hyli*/
    char m_bakFile[48];
/*add*/
    pthread_mutex_t m_mutex;

};

#endif

