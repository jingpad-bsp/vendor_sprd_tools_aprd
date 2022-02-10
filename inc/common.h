#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#include <string>
#include <regex>
extern "C" {
#endif
#include <log/log.h>
#include <log/logprint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>




#ifndef LOG_TAG
#define LOG_TAG "collect_apr"
#endif

#include <time.h>
#include <locale.h>

#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <pthread.h>

#define APR_DEBUG
#define MAX_LINE_LEN            2048
#define MAX_NAME_LEN                     128
#define TMP_FILE_PATH            "/data/sprdinfo/"
#define TMP_FILE_PATH1            "/data/sprdinfo/"
#define DEFAULT_DEBUG_APR_CONFIG        "/system/etc/apr.cmcc.conf"
#define DEFAULT_USER_APR_CONFIG        "/system/etc/apr.cmcc.conf.user"
#define DEFAULT_APR_CONFIG        "/system/etc/apr.conf.etc"

#define MAX_TOMBSTONES_FILE_NUM            50
#define MAX_FIRSTFIND_SYSSERVER            20

#ifndef APR_CMCC_ONLY
#define TMP_APR_CONFIG        TMP_FILE_PATH1 "apr.conf"
#define APR_CONFG_PARENTPATH TMP_FILE_PATH1
#else
#define TMP_APR_CONFIG        TMP_FILE_PATH "apr.conf"
#define APR_CONFG_PARENTPATH TMP_FILE_PATH
#endif

#define APR_SOCKET_FILE        TMP_FILE_PATH "apr_sock"
#define AT_BUFFER_SIZE            2048

#ifdef APR_DEBUG
#define APR_LOGD(x...) ALOGD(x)
#define APR_LOGE(x...) ALOGE(x)
#else
#define APR_LOGD(x...) do{}while(0)
#define APR_LOGE(x...) do{}while(0)
#endif


enum {
    CTRL_CMD_TYPE_PROP_TIME_CHANGED,
    CTRL_CMD_TYPE_PROP_RAMLIMIT_CHANGED,
    CTRL_CMD_TYPE_RELOAD,
    CTRL_CMD_TYPE_RSP
};
struct apr_cmd {
    int type;
    char content[MAX_LINE_LEN * 2];
};
 static int changed_flag_time = 0;
 static int changed_flag_RAMLimit = 0;
//extern unsigned long long notrace sched_clock(void);
void self_inspection_apr_enabled();
int64_t getdate(char* strbuf, size_t max, const char* format);
//void  gettime(char*strbuf, const char* format);
int uptime(char* strbuf);
void trans_time(unsigned long long tt, char* retbuf, int retlen);
int getTimeChanged();
void setTimeChanged(int flag);


extern int send_socket(int sockfd, void* buffer, int size);
extern int recv_socket(int sockfd, void* buffer, int size);
extern int copy_file(const char *dest_file, const char * src_file, int is_creat);
extern int parse_config();

static int reload(int flush);
void * command_handler(void *args);
void * handle_request(void *arg);
void set_property_time_changed(bool ch);
void set_property_RAMLimit_changed(bool ch);

int getRamLimitChanged();

void setRamLimitChanged(int maxram);
void apr_system_procrank(char *cmd_string);

char* read_Procfile_FreeBuffOut(const char * src_file, int *len);


extern char file_path1[256];
extern int result;
extern int name_num;
extern int size_num;

struct file_state{
	char name[16];
	unsigned long int  flsize;
	};
extern struct file_state fos[50];
extern struct file_state fns[50];

extern int newsta_saveto_oldsta();
extern int find_file();
extern int traversal_Filename_size (char* path, int depth,int flag_new_old);
extern int memset_fosfns();


#ifdef __cplusplus
}
#endif

#endif

