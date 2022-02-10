
#include "common.h"
#include "JavaCrashThread.h"
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
	}
};

static void handle_quit(int signo)
{
	APR_LOGD("JavaCrash Thread handle sig %d\n", signo);
	pthread_exit(NULL);
}

JavaCrashThread::JavaCrashThread(AprData* aprData)
{
	m_aprData = aprData;

	m_devices = NULL;
	m_logger_list = NULL;
	m_devCount = 0;
}

JavaCrashThread::~JavaCrashThread()
{
	m_aprData = NULL;
}

void JavaCrashThread::Setup()
{
	APR_LOGD("LoggerThread::Setup()\n");
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

void JavaCrashThread::Execute(void* arg)
{
	APR_LOGD("LoggerThread::Execute()\n");
	struct log_msg log_msg;
	log_device_t* dev;
	int ret;

	while (1) {
		/*
		 * Code snippet for testing
		 * ========================
		 * JavaCrashThread javaCrashThread(&aprData);
		 * for(i=0; i<10; i++) {
		 *	javaCrashThread.Start(NULL);
		 *	sleep(5);
		 *	javaCrashThread.Stop();
		 *	sleep(1);
		 * }
		 *
		 * Checking memory leak
		 * ====================
		 * /data/local/Inst/bin/valgrind --leak-check=full --show-leak-kinds=all /path/to/collect_apr
		 *
		 * Branch: sprdroid5.1_trunk
		 * When invoking android_logger_list_read() located in '/system/core/liblog/log_read_kern.c'
		 * rathre than '/system/core/liblog/log_read.c'(It depends on a Boolean value
		 * of TARGET_USES_LOGD in Android.mk), this test will make a memory leak.
		 *
		 * Here's why
		 * log_read_kern.c -> andoird_logger_list_read().
		 * 1) The calloc() is called before invoking poll().
		 * 2) But the corresponding free() will be invoked after poll().
		 * 3) This thread is killed by pthread_kill() when it is blocked inside poll().
		 * Note: the calloc() gets memory space from heap space belonging to process.
		 * 	When a thread exit, the dynamic memory space by malloc/calloc in a thread
		 * 	won't be released.
		 * Because this memory leak isn't seriousness, so ignore.
		 *
		 * Result
		 * ======
		 * ==6681== 160 bytes in 10 blocks are definitely lost in loss record 23 of 27
		 * ==6681==    at 0x481B98C: calloc (vg_replace_malloc.c:623)
		 * ==6681==    by 0x48A3CA9: android_logger_list_read (in /system/lib/libcutils.so)
		 * ==6681==
		 */
		ret = android_logger_list_read(m_logger_list, &log_msg);

		if (ret == 0) {
			APR_LOGE("read: Unexpected EOF!\n");
			exit(EXIT_FAILURE);
		}

		if (ret < 0) {
			if (ret == -EAGAIN) {
				break;
			}

			if (ret == -EIO) {
				APR_LOGE("read: Unexpected EOF!\n");
				exit(EXIT_FAILURE);
			}
			if (ret == -EINVAL) {
				APR_LOGE("read: Unexpected EOF!\n");
				exit(EXIT_FAILURE);
			}
			APR_LOGE("read failure\n");
			exit(EXIT_FAILURE);
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

void JavaCrashThread::Dispose()
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

void JavaCrashThread::processBuffer(log_device_t* dev, struct log_msg *tbuf)
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
		/*
		 * format: <priority:1><tag:N>\0<message:N>\0
		 *
		 * tag str
		 *   starts at buf->msg+1
		 * msg
		 *   starts at buf->msg+1+len(tag)+1
		 *
		 * The message may have been truncated by the kernel log driver.
		 * When that happens, we must null-terminate the message ourselves.
		 */
		android_log_processLogBuffer(&tbuf->entry_v1, &entry);
		//APR_LOGD("peng %s %d", entry.message, entry.messageLen);


		if (strstr(entry.tag, "AndroidRuntime")) {
					if ((s1 = strstr(entry.message, "FATAL EXCEPTION:")) != NULL) {
						m_ei.et = E_JAVA_CRASH;
						if ((s1 = strstr(s1, "Process:")) != NULL) {
							if ((s1 = strstr(s1, ":")) != NULL) {
								if ((s1 = strstr(s1, " ")) != NULL) {
									s2 = strstr(s1, ",");
									if((len=(unsigned long)s2-(unsigned long)s1)>1&&len<2048)
									{
										memset(m_buffer, 0, sizeof(m_buffer));
										strncpy(m_buffer, s1+1, len-1);
										*( m_buffer + len-1)= '\0';
										m_ei.private_data = (void*)m_buffer;
										APR_LOGD("E_JAVA_CRASH m_buffer = %s", m_buffer);
									}
								}
							}
						}
						m_aprData->setChanged();
						m_aprData->notifyObservers((void*)&m_ei);
					}
				} 
			}
		}



