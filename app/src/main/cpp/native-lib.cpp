#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>
#include "teslameter_3mhx-cdi/io.h"
#include "rtcomm/rtcomm.h"


/* ---------------------------------------------------------------------------------------------- *
 * Android log function wrappers
 * ---------------------------------------------------------------------------------------------- */

static const char* kTAG = "rtcomm-jni";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

/* ---------------------------------------------------------------------------------------------- *
 * Global data
 * ---------------------------------------------------------------------------------------------- */

static int g_counter;
static int g_fd;

/*
 * Allocate 64kB buffer to store data
 */
static struct io_buffer g_buffer;

/* ---------------------------------------------------------------------------------------------- *
 * Initialization
 * ---------------------------------------------------------------------------------------------- */

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_init(
        JNIEnv *env,
        jobject /* this */) {

    return (0);
}

/* ---------------------------------------------------------------------------------------------- *
 * DATA access methods
 * ---------------------------------------------------------------------------------------------- */
extern "C"
void
Java_com_teslameter_nr_teslameter_MainActivity_dataAcquire(
        JNIEnv *env,
        jobject /* this */) {
}

extern "C"
void
Java_com_teslameter_nr_teslameter_MainActivity_dataRelease(
        JNIEnv *env,
        jobject /* this */) {
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetXraw(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", g_buffer.sample[0][IO_CHANNEL_X]);
   
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetYraw(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", g_buffer.sample[0][IO_CHANNEL_Y]);
    
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetZraw(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", g_buffer.sample[0][IO_CHANNEL_Z]);
    
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetXvoltage(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.111);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetYvoltage(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.222);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetZvoltage(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.333);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetStats(
        JNIEnv *env,
        jobject /* this */) {
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "c%d", g_buffer.stats.ctrl_err);
    
    return env->NewStringUTF(buffer);
}

/* ---------------------------------------------------------------------------------------------- *
 * Sampling control
 * ---------------------------------------------------------------------------------------------- */

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingOpen(
        JNIEnv *env,
        jobject /* this */) {
    char version[20];
    int buffer_size = sizeof(g_buffer);

    g_fd = open("/dev/" RTCOMM_NAME, O_RDONLY);

    if (g_fd == -1) {
        LOGE("Failed to open driver: %d\n", errno);

        return (1);
    } else {
        LOGI("Opened /dev/" RTCOMM_NAME "\n");
    }

    if (ioctl(g_fd, RTCOMM_GET_VERSION, version) == -1) {
        LOGE("RTCOMM_GET_VERSION failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_GET_VERSION get: %s\n", version);
    }

    if (ioctl(g_fd, RTCOMM_SET_SIZE, &buffer_size) == -1) {
        LOGE("RTCOMM_SET_SIZE failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_SET_SIZE set: %d\n", buffer_size);
    }

    if (ioctl(g_fd, RTCOMM_INIT)) {
        LOGE("RTCOMM_INIT failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_INIT success.\n");
    }

    if (ioctl(g_fd, RTCOMM_START)) {
        LOGE("RTCOMM_START failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_START success.\n");
    }

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingRefresh(
        JNIEnv *env,
        jobject /* this */) {
    int             ret;
    uint32_t        idx;
    uint32_t        to_idx;
    uint32_t        idxl;

    ret = read(g_fd, &g_buffer, sizeof(g_buffer));

    if (ret != sizeof(g_buffer)) {
        if (ret == -1) {
            LOGE("Failed to read, error: %d\n", errno);
        } else {
            LOGE("Failed to read %d bytes, error: %d\n", sizeof(g_buffer), ret);
        }

        return (1);
    }
    LOGI("pass %05d: read %d bytes\n", g_counter++, sizeof(g_buffer));

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingClose(
        JNIEnv *env,
        jobject /* this */) {

    return (0);
}



