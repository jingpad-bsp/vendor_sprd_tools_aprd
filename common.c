#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include<sys/time.h>
#include<pwd.h>

//#include <linux/jiffies.h>
//#include <linux/sched.h>
#include "common.h"
#include "string.h"
//#include <linux/android_alarm.h>


static char s_ATBuffer[AT_BUFFER_SIZE];
//static char s_ATDictBuf[AT_BUFFER_SIZE];
static char *s_ATBufferCur = s_ATBuffer;
struct file_state fos[50];
struct file_state fns[50];
char file_path1[256]={0};
int result;
int name_num;
int size_num;

void self_inspection_apr_enabled()
{
    char value[PROPERTY_VALUE_MAX];
    char *default_value = (char*)"unknown";
    property_get("persist.sys.apr.enabled", value, default_value);
    if (!strcmp(value, "0")) {
        exit(0);
    } else if (!strcmp(value, "unknown")) {
        //property_set("persist.sys.apr.enabled", "1");
    }
}

int64_t getdate(char* strbuf, size_t max, const char* format)
{
    struct tm tm;
    time_t t;

    tzset();

    time(&t);
    localtime_r(&t, &tm);
    strftime(strbuf, max, format, &tm);

    return t;
}
/*
void  gettime(char*strbuf, const char*format)
{

    struct timeval tv;
    unsigned long long now;
    gettimeofday(&tv, NULL);
    now = sched_clock();
    //sprintf(strbuf, "%d ", tv.tv_sec);
    sprintf(strbuf, "%d ", (now/1000/1000/1000));
    return ;
}*/

int uptime(char* strbuf)
{
    char buffer[1024];
    char *p;
    int fd, result;
    int64_t sec;

    fd = open("/proc/uptime", O_RDONLY);
    if (fd < 0)
        return fd;

    result = read(fd, buffer, 1024);
    close(fd);

    if (result >= 0) {
        p = strchr(buffer, '.');
        if (p) {
            *p = '\0';
        } else {
            return -1;
        }
        if (strbuf) {
            sprintf(strbuf, "%s", buffer);
            return 0;
        }
    }
    return -1;
}

#define UTC_DIFF 116444736000000000
#define NS 10000000     //windows UTS start 1601.01.01  per 100ns inc 1 ount
void trans_time(unsigned long long tt, char* retbuf, int retlen)
{
   char    time_tmp[128];
   struct tm *tm;

   if(retlen<128)
   {
       APR_LOGD("time str buf too short");
       return;
   }
   memset(time_tmp, 0, 128);
   if (setlocale(LC_ALL, "") == NULL)
   {
       strcpy(retbuf, "unknown");
       return;
   }
   tt=tt-UTC_DIFF;
   tt=tt/NS;
   tm=localtime((const time_t *)&tt);
   strftime(time_tmp,sizeof(time_tmp),"%F %T",tm);
   APR_LOGD("apr: %s", time_tmp);
   strcpy(retbuf, time_tmp);
}

/*These codes after add by haoyu.li & lina.yang for CMCC mode requests*/

int getTimeChanged()
{

  return changed_flag_time;

}

void setTimeChanged(int flag)
{
   changed_flag_time = flag;
}

int getRamLimitChanged()
{

  return changed_flag_RAMLimit;

}

void setRamLimitChanged(int maxram)
{
      changed_flag_RAMLimit = maxram;
}

void  apr_system_procrank(char *cmd_string)
{
     int ret = system(cmd_string);
     //APR_LOGD("ret = %d",ret);
     if (ret = 0 )
     APR_LOGD("system failed errno = %s :",errno,strerror(errno));
}

int send_socket(int sockfd, void* buffer, int size)
{
    int result = -1;
    int ioffset = 0;
    while(sockfd > 0 && ioffset < size) {
        result = send(sockfd, (char *)buffer + ioffset, size - ioffset, MSG_NOSIGNAL);
        if (result > 0) {
            ioffset += result;
        } else {
            break;
        }
    }
    return result;

}

int recv_socket(int sockfd, void* buffer, int size)
{
    int received = 0, result;
    while(buffer && (received < size))
    {
        result = recv(sockfd, (char *)buffer + received, size - received, MSG_NOSIGNAL);
        if (result > 0) {
             received += result;
        } else {
             received = result;
             break;
        }
    }
    return received;
}

void  set_property_time_changed(bool ch)
{
    //APR_LOGD("set_property_time_changed before changed_flag_time=%d ",changed_flag_time);
    changed_flag_time = ch;
    APR_LOGD("set_property_time_changed after changed_flag_time=%d ",changed_flag_time);

}

void set_property_RAMLimit_changed(bool ch)
{
    changed_flag_RAMLimit = ch;

}
static int reload(int flush)
{
    APR_LOGD("APR reload.");

    //property_set("persist.sys.apr.reload", "1");
    kill(getpid(), SIGTERM);
    return 0;
}

int copy_file(const char *dest_file, const char * src_file, int is_creat)
{
#define BUFSIZE 512
    int dest_fd = -1;
    int src_fd = -1;
        char buf[BUFSIZE];
        int bytes_read = 0;
        int bytes_write = 0;

    APR_LOGD("dest_file %s, src_file %s", dest_file, src_file);

    if (dest_file == NULL || src_file == NULL)
    {
        return 1;
    }
    if(is_creat==0)
        dest_fd = open(dest_file, O_WRONLY|O_TRUNC ,0660);
    else
        dest_fd = open(dest_file, O_WRONLY|O_CREAT ,0660);
    if (dest_fd < 0)
    {
        return 1;
    }

    src_fd = open(src_file, O_RDONLY, 0660);
    if (src_fd < 0)
    {
        close(dest_fd);
        return 1;
    }

    bytes_read = read(src_fd, buf, BUFSIZE);
    while(bytes_read)
    {
        if(bytes_read<0)
        {
            if(errno!=EINTR)
                break;
            else
                goto READCONTINUE;
        }
        bytes_write = write(dest_fd,buf, bytes_read);
        if (bytes_write < 0)
        {
            break;
        }
READCONTINUE:
        bytes_read = read(src_fd, buf, BUFSIZE);
    }

    fsync(src_fd);
    fsync(dest_fd);
    close(src_fd);
    close(dest_fd);
    return 0;
}

char* read_Procfile_FreeBuffOut(const char * src_file, int *len) {
#define BUFSIZE 1024
    int src_fd = -1, i=1, bytes_read = 0, total_read=0;
    char *buf, *tmp, *p;
    unsigned long bufsize;

    //APR_LOGD("dest_file %s, src_file %s", dest_file, src_file);

    if (NULL == src_file || NULL == len){
        return NULL;
    }
    src_fd = open(src_file, O_RDONLY|O_NONBLOCK, 0660);
    if (src_fd < 0){
        return NULL;
    }

    tmp=(char*)malloc(bufsize=BUFSIZE*i);
    if (NULL == tmp) {
        APR_LOGD("read_Procfile malloc fail");
        close(src_fd);
        return NULL;
    }
    memset((void*)tmp, 0, bufsize);
    buf=tmp;
    bytes_read = read(src_fd, buf, bufsize);
    while(bytes_read > 0)
    {
        total_read+=bytes_read;
        if(total_read >= bufsize) //extend size, keep tmp[] never full.
        {
            p=(char*)malloc(bufsize = BUFSIZE*(++i));
            if (NULL == p) {
                APR_LOGD("read_Procfile malloc fail~");
                free(tmp);
                close(src_fd);
                return NULL;
            }
            memset((void*)p, 0, bufsize);
            memcpy(p, tmp, BUFSIZE*(i-1));
            free(tmp);
            tmp=p;
        }
        //APR_LOGD("bytes_read %d, total_read %d, bufsize %d, i %d", bytes_read, total_read, bufsize, i);
        buf=&tmp[total_read];
        bytes_read = read(src_fd, buf, bufsize-total_read);
    }

    close(src_fd);
    if(bytes_read<0)
    {
        free(tmp);
        APR_LOGD("bytes_read %d, total_read %d", bytes_read, total_read);
        return NULL;
    }

    *len=total_read;
    tmp[*len]='\0';
    return tmp;
}

int parse_config()
{
    int ret = 0, count = 0, dret=0;
    char buffer[MAX_LINE_LEN];
    struct stat st;
    struct passwd *pa;
    /* we use tmp log config file first */
    if((dret=stat(TMP_APR_CONFIG, &st))<0){
        ret = mkdir(APR_CONFG_PARENTPATH, S_IRWXU | S_IRWXG);
        if (-1 == ret && (errno != EEXIST)) {
            APR_LOGE("mkdir %s failed.%s", APR_CONFG_PARENTPATH, strerror(errno));
            exit(0);
        }
#ifndef APR_CMCC_ONLY
        if(stat(DEFAULT_APR_CONFIG, &st)==0)
            copy_file(TMP_APR_CONFIG, DEFAULT_APR_CONFIG, 1);
        else {
            APR_LOGE("cannot find config files!%s", strerror(errno));
            exit(0);
        }
#else
        property_get("ro.debuggable", buffer, "");
        if (strcmp(buffer, "1") != 0) {
            if(stat(DEFAULT_USER_APR_CONFIG, &st)==0)
                copy_file(TMP_APR_CONFIG, DEFAULT_USER_APR_CONFIG, 1);
            else {
                APR_LOGE("cannot find config files!%s", strerror(errno));
                exit(0);
            }
        } else {
            if(stat(DEFAULT_DEBUG_APR_CONFIG, &st)==0)
                copy_file(TMP_APR_CONFIG, DEFAULT_DEBUG_APR_CONFIG, 1);
            else {
                APR_LOGE("cannot find config files!%s", strerror(errno));
                exit(0);
            }
        }
#endif
    }
    pa=getpwnam("system");
    APR_LOGD("system pa %s %d", pa->pw_name,  pa->pw_gid);
    chown(TMP_APR_CONFIG, -1, pa->pw_gid);
    chmod(TMP_APR_CONFIG, 0666);
    return ret;
}


void *command_handler(void *arg)
{
    struct sockaddr_un serv_addr;
    int ret, server_sock, client_sock;
    pthread_t thread_pid;

restart_socket:
    /* init unix domain socket */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family=AF_UNIX;
    strcpy(serv_addr.sun_path, APR_SOCKET_FILE);
    unlink(serv_addr.sun_path);

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    chmod(APR_SOCKET_FILE, 0666);
        if (server_sock < 0) {
        APR_LOGD("create socket failed!,errno =%d:%s",errno,strerror(errno));
        sleep(2);
        goto restart_socket;
    }

    if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        APR_LOGD("bind socket failed!,errno =%d:%s",errno,strerror(errno));
        close(server_sock);
        sleep(2);
        goto restart_socket;
    }

    if (listen(server_sock, 5) < 0) {
        APR_LOGD("listen socket failed!,errno =%d:%s",errno,strerror(errno));
        close(server_sock);
        sleep(2);
        goto restart_socket;
    }

    while(1) {
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) {
            APR_LOGD("accept failed!,errno =%d:%s",errno,strerror(errno));
            sleep(1);
            continue;
        }
        if ( 0 != pthread_create(&thread_pid, NULL, handle_request, (void *) &client_sock) )
            APR_LOGD("sock thread create error,errno =%d:%s",errno,strerror(errno));
    }
}


void *handle_request(void *arg)
{
    int ret, client_sock;
    struct apr_cmd cmd;
    char filename[MAX_NAME_LEN];
    time_t t;
    struct tm tm;

    client_sock = * ((int *) arg);
    ret = recv_socket(client_sock, (void *)&cmd, sizeof(cmd));
    if(ret <  0) {
        APR_LOGD("recv data failed!,errno =%d:%s",errno,strerror(errno));
        close(client_sock);
        return 0;
    }
    /*
    if(cmd.type == CTRL_CMD_TYPE_RELOAD) {
        cmd.type = CTRL_CMD_TYPE_RSP;
        sprintf(cmd.content, "OK");
        send_socket(client_sock, (void *)&cmd, sizeof(cmd));
        close(client_sock);
        reload(1);
        return 0;
    }
    */
    switch(cmd.type) {
    case CTRL_CMD_TYPE_PROP_TIME_CHANGED:
        APR_LOGD("Time property changed  cmd: %d %s.", cmd.type, cmd.content);
        set_property_time_changed(1);
        reload(1);
        sprintf(cmd.content, "OK");

        break;
    case CTRL_CMD_TYPE_PROP_RAMLIMIT_CHANGED:
        APR_LOGD("RAM limit property changed  cmd: %d %s.", cmd.type, cmd.content);
        set_property_RAMLimit_changed(1);
        reload(1);
        sprintf(cmd.content, "OK");
        break;
    case CTRL_CMD_TYPE_RELOAD:
        reload(1);
        sprintf(cmd.content, "OK");
        break;
    default:
        APR_LOGD("wrong cmd cmd: %d %s.", cmd.type, cmd.content);
        break;
    }

    cmd.type = CTRL_CMD_TYPE_RSP;
    APR_LOGD("after cmd,before response ret=%d,cmd.content[0]=%s!",cmd.content[0]);
    if(ret && cmd.content[0] == 0)
        sprintf(cmd.content, "FAIL");
    else if(!ret && cmd.content[0] == 0)
        sprintf(cmd.content, "OK");
    send_socket(client_sock, (void *)&cmd, sizeof(cmd));
    close(client_sock);

    return 0;
}

int newsta_saveto_oldsta()
{
        int i;
        for(i=0;i<MAX_TOMBSTONES_FILE_NUM;i++)
		{
			strcpy(fos[i].name ,fns[i].name);
			fos[i].flsize=fns[i].flsize;
		}
		APR_LOGD("oldsta_to_newsta\n");
		return 0;
}


int find_file()
{
		int i,j,flag;
		for(i=0;i<MAX_TOMBSTONES_FILE_NUM;i++)
		{   flag=1;
			for(j=0;j<MAX_TOMBSTONES_FILE_NUM;j++)
			{
				if (fns[i].flsize==fos[j].flsize)
				   {
					   flag=0;
				   }
			}
			if(flag)
			break;
		}
		APR_LOGD("find_file id %d\n", i);
		return i;
}


int traversal_Filename_size (char* path, int depth,int flag_new_old)
 {
	DIR *d;
	struct dirent *file;
	struct stat sb;
    if(!(d = opendir(path)))
     {
		APR_LOGD("error opendir %s!!!\n",strerror(errno));
		return -1;
     }
     while((file = readdir(d)) != NULL)
     {
		if (strstr(file->d_name, "tombstone_"))
        {
			if(flag_new_old)
			{
				strncpy(fos[name_num++].name, file->d_name, 16);
				//strcpy(fos[name_num++].name, file->d_name);
				sprintf(file_path1,"%s/%s","/data/tombstones",file->d_name);
				result=stat(file_path1, &sb);
				if(result!= 0)
				{
					perror( "error" );
				}
				fos[size_num++].flsize=sb.st_size;
			}
			else
			{
				strncpy(fns[name_num++].name, file->d_name, 16);
				//strcpy(fns[name_num++].name, file->d_name);
				sprintf(file_path1,"%s/%s","/data/tombstones",file->d_name);
				result=stat(file_path1, &sb);
				if(result!= 0)
				perror( "error" );
				fns[size_num++].flsize=sb.st_size;
			}
        }
     }
	name_num=0;
	size_num=0;
	closedir(d);
	return 0;
 }

 int memset_fosfns(void)
  {
 	memset(fos,0,sizeof(fos));
	memset(fns,0,sizeof(fns));
	return 0;
   }
