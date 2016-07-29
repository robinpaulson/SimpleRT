#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <android/log.h>

#define LOG_TAG "SIMPLE_RT_JNI"

#define DPRINTF(level, fmt, args...) \
    do { \
        __android_log_print(level, LOG_TAG, fmt, ##args); \
    } while (0);

#define LOGV(fmt, args...) DPRINTF(ANDROID_LOG_VERBOSE, fmt, ##args)
#define LOGD(fmt, args...) DPRINTF(ANDROID_LOG_DEBUG, fmt, ##args)
#define LOGI(fmt, args...) DPRINTF(ANDROID_LOG_INFO, fmt, ##args)
#define LOGW(fmt, args...) DPRINTF(ANDROID_LOG_WARN, fmt, ##args)
#define LOGE(fmt, args...) DPRINTF(ANDROID_LOG_ERROR, fmt, ##args)

static struct {
    pthread_t tun_thread;
    pthread_t acc_thread;
    int tun_fd;
    int acc_fd;
	volatile bool is_started;
} module;

enum ThreadType {
    TUN_THREAD = 0,
    ACC_THREAD = 1,
};

#define ACC_BUF_SIZE 4096

jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    LOGV(__func__);

	module.is_started = false;
    return JNI_VERSION_1_6;
}

void *thread_proc(void *arg)
{
    char buf[ACC_BUF_SIZE] = { 0 };
    ssize_t rd;
    int in_fd, out_fd;

    enum ThreadType thread_type = (enum ThreadType) arg;

    if (thread_type == TUN_THREAD) {
        in_fd = module.tun_fd;
        out_fd = module.acc_fd;
    } else {
        in_fd = module.acc_fd;
        out_fd = module.tun_fd;
    }

    while (module.is_started) {
        if ((rd = read(in_fd, buf, sizeof(buf))) > 0) {
            write(out_fd, buf, rd);
        } else {
            /* FIXME */
            break;
        }
    }

    module.is_started = false;
    close(in_fd);

    return NULL;
}

JNIEXPORT void JNICALL
Java_com_viper_simplert_Native_start(JNIEnv *env, jclass type, jint tun_fd, jint acc_fd)
{
    LOGV("%s: tun_fd = %d, acc_fd = %d", __func__, tun_fd, acc_fd);

    if (module.is_started) {
        LOGE("Native threads already started!");
        return;
    }

	module.is_started = true;
    module.tun_fd = tun_fd;
    module.acc_fd = acc_fd;

    int flags = fcntl(tun_fd, F_GETFL, 0);
    fcntl(tun_fd, F_SETFL, flags & ~O_NONBLOCK);

    flags = fcntl(acc_fd, F_GETFL, 0);
    fcntl(acc_fd, F_SETFL, flags & ~O_NONBLOCK);

    pthread_create(&module.tun_thread, NULL, thread_proc, TUN_THREAD);
    pthread_create(&module.acc_thread, NULL, thread_proc, ACC_THREAD);
}

JNIEXPORT void JNICALL
Java_com_viper_simplert_Native_stop(JNIEnv *env, jclass type)
{
    LOGV(__func__);

	module.is_started = false;

	pthread_join(module.tun_thread, NULL);
    pthread_join(module.acc_thread, NULL);
}

JNIEXPORT jboolean JNICALL
Java_com_viper_simplert_Native_is_1running(JNIEnv *env, jclass type)
{
    LOGV(__func__);
    return module.is_started;
}