#include "common.h"
#include "AnrThread.h"
#include "AprData.h"

struct log_device_t {
    const char* device;
    bool binary;
    struct logger *logger;
    struct logger_list *logger_list;
    bool printed;
    char label;

    log_device_t* next;

    log_device_t(const char* d, bool b, char l) {
        device = d;
        binary = b;
        label = l;
        next = NULL;
        printed = false;
        logger = NULL;
        logger_list = NULL;
    }
};

static void handle_quit(int signo)
{
    APR_LOGD("Anr Thread handle sig %d\n", signo);
    pthread_exit(NULL);
}

AnrThread::AnrThread(AprData* aprData)
{
    m_aprData = aprData;

    m_devices = NULL;
    m_logger_list = NULL;
    m_devCount = 0;
}

AnrThread::~AnrThread()
{
    m_aprData = NULL;
}

void AnrThread::Setup()
{
    APR_LOGD("AnrThread::Setup()\n");
    log_device_t* dev;
    int mode = O_RDONLY;

    dev = m_devices = new log_device_t("main", false, 'm');
    m_devCount = 1;
    if (android_name_to_log_id("system") == LOG_ID_SYSTEM) {
        dev = dev->next = new log_device_t("system", false, 's');
        m_devCount++;
    }
    if (android_name_to_log_id("crash") == LOG_ID_CRASH) {
        dev = dev->next = new log_device_t("crash", false, 'c');
        m_devCount++;
    }
    dev = m_devices;
    /* read only the most recent <count> lines */
    m_logger_list = android_logger_list_alloc(mode, 1/*<count>*/, 0);

    while (dev) {
        dev->logger_list = m_logger_list;
        dev->logger = android_logger_open(m_logger_list,
                android_name_to_log_id(dev->device));
        if (!dev->logger) {
            APR_LOGE("Unable to open log device '%s'\n", dev->device);
            exit(EXIT_FAILURE);
        }

        dev = dev->next;
    }

    signal(SIGQUIT, handle_quit);
}

void AnrThread::Execute(void* arg)
{
    APR_LOGD("AnrThread::Execute()\n");
    struct log_msg log_msg;
    log_device_t* dev;
    int ret;

    while (1) {

        ret = android_logger_list_read(m_logger_list, &log_msg);

        if (ret == 0) {
            APR_LOGE("AnrThread: Unexpected EOF!ret= %d\n",ret);
            exit(EXIT_FAILURE);
        }

        if (ret < 0) {
            if (ret == -EAGAIN) {
                APR_LOGE("AnrThread: Unexpected EOF!ret= %d\n",ret);
                break;
            }

            if (ret == -EIO) {
                APR_LOGE("AnrThread: Unexpected EOF IO error!ret= %d\n",ret);
                break;
            }
            if (ret == -EINVAL) {
                APR_LOGE("AnrThread: unexpected length!ret= %d\n",ret);
                break;
            }
            APR_LOGE("AnrThread failure!ret = %d\n",ret);
            continue;
        }

        for(dev = m_devices; dev; dev = dev->next) {
            if (android_name_to_log_id(dev->device) == log_msg.id()) {
                break;
            }
        }
        if (!dev) {
            APR_LOGE("read: Unexpected log ID!\n");
            exit(EXIT_FAILURE);
        }
        processBuffer(dev, &log_msg);
    }

}

void AnrThread::Dispose()
{
    log_device_t* next_dev;
    log_device_t* current_dev;

    next_dev = m_devices;
    while (next_dev) {
        current_dev = next_dev;
        next_dev = current_dev->next;
        delete current_dev;
    }
    m_devices = NULL;
    /* Close all the logs */
    android_logger_list_free(m_logger_list);
    m_logger_list = NULL;
}

void AnrThread::processBuffer(log_device_t* dev, struct log_msg *tbuf)
{
	static int32_t pid = 0;
	const char* s1 = NULL;
	const char* s2 = NULL;
	int err;
	unsigned long len=0;
	//struct logger_entry* buf = &tbuf->entry_v1;
	AndroidLogEntry entry;
	if (dev->binary) {

	} else {
		android_log_processLogBuffer(&tbuf->entry_v1, &entry);
		if ((s1 = strstr(entry.message, "ANR in ")) != NULL) {
			m_ei.et = E_ANR;
			s1=s1+6;
			if (( s2 = strpbrk(s1+1, " \r\n\t")) !=NULL) {
				if((len=(unsigned long)s2-(unsigned long)s1)>1&&len<30)
				{
					memset(m_buffer, 0, sizeof(m_buffer));
					strncpy(m_buffer, s1+1, len-1);
					*( m_buffer + len-1)= '\0';
					APR_LOGD("E_ANR : m_buffer = %s", m_buffer);
					m_ei.private_data = (void*)m_buffer;
				}
			}else{
				if((len=strlen(s1))>3&&len<30)
				{
					memset(m_buffer, 0, sizeof(m_buffer));
					strncpy(m_buffer, s1+1, len);
					*( m_buffer + len-1)= '\0';
					APR_LOGD("E_ANR m_buffer = %s", m_buffer);
					m_ei.private_data = (void*)m_buffer;
				}
			}
			m_aprData->setChanged();
			m_aprData->notifyObservers((void*)&m_ei);
		}
	}
}


