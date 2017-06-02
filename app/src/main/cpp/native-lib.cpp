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
 * Data types
 * ---------------------------------------------------------------------------------------------- */



/* -- RTCOMM-JNI working mode -- */
enum rtcomm_jni_mode {
    RTCOMM_JNI_MODE_NORMAL,
    RTCOMM_JNI_MODE_SIMULATION
};

/* -- RTCOMM-JNI context structure -- */
struct rtcomm_ctx
{
    /* RTCOMM-JNI working mode */
    enum rtcomm_jni_mode        mode;
    /* File handle for RTCOMM driver */
    int                         driver_fd;
    /* Allocate buffer to store data comming from RTCOMM driver */
    struct io_buffer            io_buffer;
};

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

static int                      g_counter;

static struct rtcomm_ctx        g_ctx;


/* ---------------------------------------------------------------------------------------------- *
 * Private methods
 * ---------------------------------------------------------------------------------------------- */
static void sim_populate_buffer(struct rtcomm_ctx * ctx)
{

}

/* ---------------------------------------------------------------------------------------------- *
 * Public methods
 * ---------------------------------------------------------------------------------------------- */

/* -- Initialization ---------------------------------------------------------------------------- */

JNIEXPORT jint JNICALL
Java_com_teslameter_nr_teslameter_MainActivity_rtcommInit(
        JNIEnv *env,
        jobject /* this */,
        jint mode) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    switch (mode) {
        case 0:
            ctx->mode = RTCOMM_JNI_MODE_NORMAL;
            break;
        case 1:
            ctx->mode = RTCOMM_JNI_MODE_SIMULATION;
            break;
        default:
            ctx->mode = RTCOMM_JNI_MODE_NORMAL;
    }

    return (0);
}


/* -- DATA access methods ----------------------------------------------------------------------- */
extern "C"
void
Java_com_teslameter_nr_teslameter_MainActivity_dataAcquire(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    switch (ctx->mode) {
        case RTCOMM_JNI_MODE_NORMAL:
            break;
        case RTCOMM_JNI_MODE_SIMULATION: {
            sim_populate_buffer(ctx);
            break;
        }
    }
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
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", ctx->io_buffer.sample[0][IO_CHANNEL_X]);
   
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetYraw(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", ctx->io_buffer.sample[0][IO_CHANNEL_Y]);
    
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetZraw(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%d", ctx->io_buffer.sample[0][IO_CHANNEL_Z]);
    
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetXvoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.111);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetYvoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.222);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetZvoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.3f", g_counter * 1.333);

    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetStats(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[100];

    snprintf(buffer, sizeof(buffer), "c%d", ctx->io_buffer.stats.ctrl_err);
    
    return env->NewStringUTF(buffer);
}

/* -- Sampling control -------------------------------------------------------------------------- */

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingOpen(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    char version[20];
    int buffer_size = sizeof(ctx->io_buffer);

    ctx->driver_fd = open("/dev/" RTCOMM_NAME, O_RDONLY);

    if (ctx->driver_fd == -1) {
        LOGE("Failed to open driver: %d\n", errno);

        return (1);
    } else {
        LOGI("Opened /dev/" RTCOMM_NAME "\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_GET_VERSION, version) == -1) {
        LOGE("RTCOMM_GET_VERSION failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_GET_VERSION get: %s\n", version);
    }

    if (ioctl(ctx->driver_fd, RTCOMM_SET_SIZE, &buffer_size) == -1) {
        LOGE("RTCOMM_SET_SIZE failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_SET_SIZE set: %d\n", buffer_size);
    }

    if (ioctl(ctx->driver_fd, RTCOMM_INIT)) {
        LOGE("RTCOMM_INIT failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_INIT success.\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_START)) {
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
    struct rtcomm_ctx *         ctx = &g_ctx;
    int             ret;
    uint32_t        idx;
    uint32_t        to_idx;
    uint32_t        idxl;

    ret = read(ctx->driver_fd, &ctx->io_buffer, sizeof(ctx->io_buffer));

    if (ret != sizeof(ctx->io_buffer)) {
        if (ret == -1) {
            LOGE("Failed to read, error: %d\n", errno);
        } else {
            LOGE("Failed to read %d bytes, error: %d\n", sizeof(ctx->io_buffer), ret);
        }

        return (1);
    }
    LOGI("pass %05d: read %d bytes\n", g_counter++, sizeof(ctx->io_buffer));

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingClose(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (0);
}



