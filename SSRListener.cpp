
#include "common.h"
#include "SSRListener.h"
#include "AprData.h"
#include <dirent.h>

static void handle_quit(int signo)
{
    APR_LOGD("JavaCrash Thread handle sig %d\n", signo);
    pthread_exit(NULL);
}

SSRListener::SSRListener(AprData* aprData)
{
    m_aprData = aprData;
    memset(m_system_pid_cur, 0, PID_LEN);
    memset(m_system_pid_old, 0, PID_LEN);
    property_get("sys.debug.fwc", m_persist_old, "unknown");
    if (strcmp(m_persist_old , "unknown")==0)  //sys.debug.fwc may be not exist.
    {
        GetSystemServerPid(1);
        strcpy(m_system_pid_old, m_system_pid_cur);
    }
}

SSRListener::~SSRListener()
{
    m_aprData = NULL;
}

void SSRListener::Setup()
{
    APR_LOGD("SSRListener::Setup()\n");
    signal(SIGQUIT, handle_quit);
}

void  SSRListener::GetSystemServerPid(int isFirst)
{
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[50];
    char filetext[50];
    memset(filepath, '\0', 50);
    memset(filetext, '\0', 50);
    int i=0;
    int firstfind=0;
	
    dir = opendir("/proc");
	
    if (NULL != dir)
    {
        if(isFirst==0)
        {
            sprintf(filepath, "/proc/%s/comm", m_system_pid_old);
            fp = fopen(filepath, "r");
            if (NULL != fp)
            {
                fread(filetext, 1, 50, fp);
                filetext[49] = '\0';

                //APR_LOGD("SSRListener %s",filetext);
                if (filetext == strstr(filetext, "system_server"))
                {
                    strcpy(m_system_pid_cur, m_system_pid_old);
                   //APR_LOGD("SSRListener file1 %s",filetext);
                    //The process file: /proc/m_persist_old is exit, and comm is system_server. This mean that framework haven't rebooted.
                    //keep m_system_pid_cur==m_persist_old, not triger UpdateXML.
                    fclose(fp);
                    closedir(dir);
                    return;
                }
                else
                {
                    fclose(fp);
                }
            }
        }
        for(i=0;i<MAX_FIRSTFIND_SYSSERVER;i++)
         {
                 while ((ptr = readdir(dir)) != NULL)
                 {
                     if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) continue;
                     if (NULL != strstr(ptr->d_name, "asound")) continue;
                     if (DT_DIR != ptr->d_type) continue;

                     //APR_LOGD("SSRListener %s",ptr->d_name);
                     sprintf(filepath, "/proc/%s/comm", ptr->d_name);
                     fp = fopen(filepath, "r");
                     if (NULL != fp)
                     {
                         fread(filetext, 1, 50, fp);
                         filetext[49] = '\0';

                         //APR_LOGD("SSRListener %s",filetext);
                         if (filetext == strstr(filetext, "system_server"))
                         {
                             APR_LOGD("SSRListener find system_server pid is %s",ptr->d_name);
                             strcpy(m_system_pid_cur, ptr->d_name);
                             firstfind=1;
                             fclose(fp);
                             break;
                         }
                         fclose(fp);
                     }
                     
                 }
                   closedir(dir);
				   APR_LOGD("SSRListener GetSystemServerPid = %d times",i+1);
                   if(firstfind==1)
                   {
                     break;
                   }
                   else{

                        sleep(i*2);
                        dir = opendir("/proc");
                        if (NULL == dir)
                               { APR_LOGD("SSRListener can not opendir /proc.");
							   break;
							   }
                        
                        }
                   
         }
         if(firstfind==0)
         {
               APR_LOGD("SSRListener can not find system_server pid in proc when first on.");
         }
        
    }
}


inline void SSRListener::UpdateXML()
{
    char buf[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    memset(buf,'\0', PROPERTY_VALUE_MAX);
    property_get("ro.boot.mode", buf, default_value);

    m_ei.et = E_SYSTEM_REBOOT;
    strcpy(m_buffer, buf);
    m_ei.private_data = (void *)m_buffer;
    m_aprData->setChanged();
    m_aprData->notifyObservers((void*)&m_ei);

    return;
}
void SSRListener::Execute(void* arg)
{
    APR_LOGD("SSRListener::Execute()\n");
    char *default_value = (char*)"unknown";
    char pid_new[PID_LEN];
    char persist_new[PROPERTY_VALUE_MAX];

    while (1) {
        //APR_LOGD("SSRListener beating ...");
        property_get("sys.debug.fwc", persist_new, default_value);
		
        if (strcmp(persist_new , default_value)!=0)  //sys.debug.fwc may be not exist.
        {
		
            if (strcmp(persist_new , m_persist_old))
            {
                APR_LOGD("SSRListener persist_old %s,persist_new %s",m_persist_old, persist_new);
                strcpy(m_persist_old, persist_new);
            //  strcpy(m_system_pid_old, m_system_pid_cur);
			    GetSystemServerPid(0);
				if (strcmp(m_system_pid_old, m_system_pid_cur))
				{
				//strcpy(m_persist_old, persist_new);
				APR_LOGD("SSRListener m_system_pid_old %s,m_system_pid_cur %s",m_system_pid_old, m_system_pid_cur);
				strcpy(m_system_pid_old, m_system_pid_cur);
				UpdateXML();
				}
                
                sleep(1);
                continue;
            }
        }
        /**/
        sleep(2);
    }
}

void SSRListener::Dispose()
{

}
