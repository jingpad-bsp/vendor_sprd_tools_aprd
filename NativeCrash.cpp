#include "common.h"
#include "NativeCrash.h"

NativeCrash::NativeCrash(AprData* aprData)
{
    m_aprData = aprData;
}

void NativeCrash::init_wd_context(vector<struct wd_context>* pvec)
{
    //APR_LOGD(" NativeCrash::init_wd_context");
    pvec->push_back(wd_context("/data", IN_CREATE|IN_DELETE|IN_MOVED_FROM));
    pvec->push_back(wd_context("/data/tombstones", IN_CLOSE_WRITE));
}

void NativeCrash::handle_event(int ifd, vector<struct wd_context>* pvec, struct inotify_event* event)
{
    //APR_LOGD(" NativeCrash::handle_event");

    if (event->wd == (*pvec)[1].wd) {
        event_wd1(event);
        return;
    }

    /* /data */
    if (event->wd == (*pvec)[0].wd) {
        event_wd0(ifd, pvec, event);
        return;
    }

}

int NativeCrash::event_wd0(int ifd, vector<wd_context>* pvec, struct inotify_event* event)
{
    int wd;
    if ((event->mask & IN_ISDIR)
        && !strcmp(event->name, "tombstones")) {

        if (event->mask & IN_CREATE) {
            APR_LOGD("inotify_add_watch(fd, %s)\n", (*pvec)[1].pathname.c_str());
            wd = inotify_add_watch(ifd, (*pvec)[1].pathname.c_str(), (*pvec)[1].mask);
            if (wd < 0) {
                APR_LOGE("Can't add watch for %s.\n", (*pvec)[1].pathname.c_str());
                exit(-1);
            }
            (*pvec)[1].wd = wd;
        } else if (event->mask & IN_MOVED_FROM) {
            APR_LOGD("inotify_rm_watch, %s\n", (*pvec)[1].pathname.c_str());
            wd = inotify_rm_watch(ifd, (*pvec)[1].wd);
            if (wd < 0) {
                APR_LOGE("Can't del watch for %s.\n", (*pvec)[1].pathname.c_str());
                exit(-1);
            }
            (*pvec)[1].wd = -1;
        } else if (event->mask & IN_DELETE) {
            APR_LOGD("inotify_rm_watch, %s\n", (*pvec)[1].pathname.c_str());
            wd = inotify_rm_watch(ifd, (*pvec)[1].wd);
            if (wd < 0) {
                APR_LOGE("Can't rm watch for %s.\n", (*pvec)[1].pathname.c_str());
                if (errno != EINVAL)
                    exit(-1);
            }
            (*pvec)[1].wd = -1;
        }
    }

    return 0;
}

int NativeCrash::event_wd1(struct inotify_event* event)
{
         int wd;
         int file_id;
         traversal_Filename_size ("/data/tombstones", 1,0);
    if (!(event->mask & IN_ISDIR))
    {
        if (event->mask & IN_CLOSE_WRITE) {
            // native crash
            m_ei.et = E_NATIVE_CRASH;
            file_id=find_file();
            newsta_saveto_oldsta();
            APR_LOGD("NativeCrash::event_wd1 file_id=%d\n",file_id);
            if(file_id < MAX_TOMBSTONES_FILE_NUM)
			{
                        APR_LOGD("fns[%d].name=%s\n",file_id, fns[file_id].name);
                     if(NULL != getTomAppName(fns[file_id].name, m_inotifynamecopy))
                   {
                       // getTomAppName(event->name, m_inotifynamecopy);
                       
                       if(strlen(m_inotifynamecopy)<PATH_MAX)
                           m_ei.private_data = (void*)m_inotifynamecopy;
                       m_aprData->setChanged();
                       m_aprData->notifyObservers((void*)&m_ei);
                   }
            }
        }
    }
    return 0;
}


char*  NativeCrash::getTomAppName(char *pathname, char *retbuf)
{
    char buffer[MAX_LINE_LEN];
    int ret,fd = -1;
    char *s1 = NULL;
    char *s2 = NULL;
    char *temp = NULL;
    FILE *fp_src = NULL;
    struct stat st;
    char pathname_1 [256]={0} ;

    umask(0);
    sprintf(pathname_1,"%s/%s","/data/tombstones",pathname);
    //APR_LOGD("pathname_1=%s",pathname_1);
    if(!stat(pathname_1, &st))
    {
        fp_src = fopen(pathname_1, "r");
        if (fp_src == NULL) {
            APR_LOGD("open file = %s  error=%s!",pathname,strerror(errno));
            return NULL;
        }

        ret = fread(buffer,1 , sizeof(buffer),fp_src);
        if (ret < 0) {
            APR_LOGD("read   error!");
            fclose(fp_src);
            return NULL;
        }

        // parse app name.

        if ((s1 = strstr(buffer, ">>>")) != NULL) {
           if((s1 = strstr(s1, " ")) != NULL)
               s2 = strtok_r(s1,"<<<",&temp);
        }else {
            APR_LOGD("can not get app name from tombstone ,%d\n",strerror(errno));
            fclose(fp_src);
             return NULL;
        }

    }else{
        APR_LOGD("cannot find tombstone files!%s",strerror(errno));
    }
	if(fp_src != NULL)
    fclose(fp_src);
	if(s2 != NULL)
    strcpy(retbuf,s2);
    return s2;
}

