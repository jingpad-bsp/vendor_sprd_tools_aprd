#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
//#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
// GMS push need fts to calculate file size ->
#include <fts.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
// GMS push <-
#include <cutils/properties.h>
#include "common.h"

char external_stor[MAX_NAME_LEN];

void usage(const char *name)
{
    printf("Usage: %s <operation> [arguments]\n", name);
    printf("Operation:\n"
               "\ttimechanged  time changed\n"
               "\trlchanged  ram limit changed\n"
               "\treload  reload apr\n");
    return;
}


int main(int argc, char *argv[])
{
    int sockfd, ret;
    struct apr_cmd cmd;
    struct sockaddr_un address;
    struct timeval tv_out;
    /* GMS push arguments -> */
    struct statfs gmsDiskInfo;
    long gmsAvailableSize;
    int ftsOptions;
    FTSENT *p_ftsent;
    DIR* gms_dir_ptr;
    FTS* fts;
    long blocksize;
    long gmsBlocks;
    int gmsEval;
    const char *gmsPath[2];

    if(argc < 2) {
        usage(argv[0]);
        return 0;
    }

    memset(&cmd, 0, sizeof(cmd));

    if(!strncmp(argv[1], "timechanged", 11)) {
        cmd.type = CTRL_CMD_TYPE_PROP_TIME_CHANGED;
    }  else if(!strncmp(argv[1], "rlchanged", 9)) {
        cmd.type = CTRL_CMD_TYPE_PROP_RAMLIMIT_CHANGED;
    }else if(!strncmp(argv[1], "reload", 6)){
        cmd.type = CTRL_CMD_TYPE_RELOAD;
    } else {
        usage(argv[0]);
        return 0;
    }

    /* init unix domain socket */
    memset(&address, 0, sizeof(address));
    address.sun_family=AF_UNIX;
    strcpy(address.sun_path, APR_SOCKET_FILE);

     sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) {
        perror("create socket failed");
        return -1;
    }
    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if (ret < 0) {
        perror("connect failed,use another way");
        //printf("internal storage,%s,\n", INTERNAL_LOG_PATH);
        //printf("external storage,%s,\n", init_external_stor());
        return -1;
    }
    ret = send_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
        perror("send cmd failed");
        return -1;
    }

    tv_out.tv_sec = 3600;
    tv_out.tv_usec = 0;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    if (ret < 0) {
        perror("setsockopt failed");
    }
    ret = recv_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
        perror("recv failed");
        return -1;
    }
    if(!strncmp(cmd.content,"FAIL", 4)){
        printf("apr cmd fail \n");
        return -1;
    }
    printf("%s\n", cmd.content);

    close(sockfd);
    return 0;
}

