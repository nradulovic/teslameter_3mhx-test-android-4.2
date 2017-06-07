#include <jni.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <android/log.h>
#include "teslameter_3mhx-cdi/io.h"

#include "i2c-dev.h"
#include "rtcomm/rtcomm.h"

/* ---------------------------------------------------------------------------------------------- *
 * Data types
 * ---------------------------------------------------------------------------------------------- */

#define VREF                            2.5
#define VQUANT_MV                       (((VREF * 2.0) / (8388608 - 1)) * 1000.0)
#define WINDOW_BUFF_SIZE                800

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
    /* Allocate buffer to store data coming from RTCOMM driver */
    struct io_buffer            io_buffer;
    /* Post process buffer */
    struct pp_buffer
    {
        struct pp_buffer_raw {
            int32_t                     aux_value[IO_AUX_CHANNELS];
            int32_t                     probe_value[IO_PROBE_CHANNELS];
            int32_t                     probe_window[WINDOW_BUFF_SIZE][IO_PROBE_CHANNELS];
            int32_t                     probe_array[IO_BUFF_SIZE][IO_PROBE_CHANNELS];
        }                           raw;
        uint32_t                    no_samples;
    }                           pp_buffer;
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

static struct rtcomm_ctx        g_ctx;


/* ---------------------------------------------------------------------------------------------- *
 * Private methods
 * ---------------------------------------------------------------------------------------------- */
static void sim_populate_buffer(struct rtcomm_ctx * ctx)
{
    static int                  counter;

    ctx->io_buffer.header.data_size = sizeof(ctx->io_buffer);
    ctx->io_buffer.header.crc = 0;
    ctx->io_buffer.header.frame = counter++;
    ctx->io_buffer.header.magic = RTCOMM_HEADER_MAGIC;
    ctx->io_buffer.stats.total_errors = 3;

    ctx->io_buffer.sample[0][IO_CHANNEL_X] = counter * 1.111;
    ctx->io_buffer.sample[0][IO_CHANNEL_Y] = counter * 1.222;
    ctx->io_buffer.sample[0][IO_CHANNEL_Z] = counter * 1.333;
}

static int io_buffer_get_no_samples(struct io_buffer * buffer)
{
    switch (buffer->param.vspeed) {
        case IO_VSPEED_10SPS: return 1;
        case IO_VSPEED_30SPS: return 3;
        case IO_VSPEED_50SPS: return 5;
        case IO_VSPEED_60SPS: return 6;
        case IO_VSPEED_100SPS: return 10;
        case IO_VSPEED_500SPS: return 50;
        case IO_VSPEED_1000SPS: return 100;
        case IO_VSPEED_2000SPS: return 200;
        case IO_VSPEED_3750SPS: return 375;
        case IO_VSPEED_7500SPS: return 750;
        case IO_VSPEED_15000SPS: return 1500;
        case IO_VSPEED_30000SPS: return 3000;
        default: return -1;
    }
}

static void post_process(struct rtcomm_ctx * ctx)
{
    uint32_t                    channel_id;
    int32_t                     sample_idx;
    int64_t                     sum[IO_PROBE_CHANNELS];
    int32_t                     no_samples;

    no_samples = io_buffer_get_no_samples(&ctx->io_buffer);

    if (no_samples < 1) {
        /*
         * TODO: Report an error about this
         */
        return;
    }

    /* Save AUX values */
    memcpy(&ctx->pp_buffer.raw.aux_value[0], &ctx->io_buffer.aux[0],
           sizeof(int32_t) * IO_AUX_CHANNELS);

    /* Calculate RAW probe value */

    for (channel_id = 0; channel_id < IO_PROBE_CHANNELS; channel_id++) {
        sum[channel_id] = 0;

        for (sample_idx = 0; sample_idx < no_samples; sample_idx++) {
            sum[channel_id] += ctx->io_buffer.sample[sample_idx][channel_id];
        }
        ctx->pp_buffer.raw.probe_value[channel_id] = (int32_t)(sum[channel_id] / no_samples);
    }

    /* Calculate WINDOW probe values */
    if (no_samples <= WINDOW_BUFF_SIZE) {
        memcpy(&ctx->pp_buffer.raw.probe_window[0][0], &ctx->io_buffer.sample[0][0],
               sizeof(int32_t) * IO_PROBE_CHANNELS * no_samples);
    } else {

    }
    /* Save all probe values */
    memcpy(&ctx->pp_buffer.raw.probe_array[0][0], &ctx->io_buffer.sample[0][0],
           sizeof(int32_t) * IO_PROBE_CHANNELS * no_samples);

    /* Fill in no_samples */
    ctx->pp_buffer.no_samples = (uint32_t)no_samples;
}

static int32_t get_aux_raw(struct rtcomm_ctx *ctx, uint32_t mchannel)
{
    /* Check boundaries  */
    if (mchannel >= (sizeof(ctx->io_buffer.aux) / sizeof(ctx->io_buffer.aux[0]))) {
        /*
         * TODO: Report an error about this
         */
        return (0);
    }

    return (ctx->pp_buffer.raw.aux_value[mchannel]);
}

static int32_t get_probe_x_raw(struct rtcomm_ctx * ctx, uint32_t channel)
{
    return (ctx->pp_buffer.raw.probe_value[channel]);
}


/* ---------------------------------------------------------------------------------------------- *
 * Public methods
 * ---------------------------------------------------------------------------------------------- */

/* -- Initialization ---------------------------------------------------------------------------- */

extern "C" jint
Java_com_teslameter_nr_teslameter_MainActivity_rtcommInit(
        JNIEnv *env,
        jobject /* this */,
        jint mode) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    switch (mode) {
        case 0:
            ctx->mode = RTCOMM_JNI_MODE_NORMAL;
            system("/start_rtcomm.sh");
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
jint
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeXRaw(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_X));
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeYRaw(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Y));
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeZRaw(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Z));
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_dataAuxRaw(
        JNIEnv *env,
        jobject /* this */,
        jint mchannel) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_aux_raw(ctx, mchannel));
}

extern "C"
jfloat
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeXVoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_X) * VQUANT_MV);
}

extern "C"
jfloat
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeYVoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Y) * VQUANT_MV);
}

extern "C"
jfloat
Java_com_teslameter_nr_teslameter_MainActivity_dataProbeZVoltage(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Z) * VQUANT_MV);
}

extern "C"
jfloat
Java_com_teslameter_nr_teslameter_MainActivity_dataAuxVoltage(
        JNIEnv *env,
        jobject /* this */,
        jint mchannel) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_aux_raw(ctx, mchannel) * VQUANT_MV);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetStats(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[300];

    snprintf(buffer, sizeof(buffer), "te%d,rcf%d,cce%d,cde%d,ae%d,nre%d,rse%d,rte%d,rce%d",
            ctx->io_buffer.stats.total_errors,
            ctx->io_buffer.stats.runtime_check_failed,
            ctx->io_buffer.stats.ctrl_comm_err,
            ctx->io_buffer.stats.ctrl_data_err,
            ctx->io_buffer.stats.ads_err,
            ctx->io_buffer.stats.no_resource_err,
            ctx->io_buffer.stats.rtcomm_skipped_err,
            ctx->io_buffer.stats.rtcomm_transfer_err,
            ctx->io_buffer.stats.rtcomm_complete_err);
    
    return env->NewStringUTF(buffer);
}

extern "C"
jstring
Java_com_teslameter_nr_teslameter_MainActivity_dataGetInfos(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[300];
    static uint32_t             header_invalid_count;
    static uint32_t             footer_invalid_count;
    uint32_t                    data_size = 0;
    uint32_t                    frame = 0;
    bool                        is_valid_footer = false;
    bool                        is_valid_header = false;

    if (rtcomm_header_unpack(&ctx->io_buffer.header, &data_size, &frame)) {
        is_valid_header = true;

        if (rtcomm_footer_unpack(&ctx->io_buffer.footer, &ctx->io_buffer.header)) {
            is_valid_footer = true;
        }
    }
    header_invalid_count += is_valid_header ? 0 : 1;
    footer_invalid_count += is_valid_footer ? 0 : 1;

    snprintf(buffer, sizeof(buffer), "header e:%d,footer e:%d,ds%d,f%d", header_invalid_count,
             footer_invalid_count, data_size, frame);

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

    if (ctx->mode == RTCOMM_JNI_MODE_SIMULATION) {
        return (0);
    }

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

    if (ctx->mode == RTCOMM_JNI_MODE_SIMULATION) {
        struct timespec tm;

        tm.tv_sec = 0;
        tm.tv_nsec = 100000000l;

        nanosleep(&tm, NULL);
        return (0);
    }

    ret = read(ctx->driver_fd, &ctx->io_buffer, sizeof(ctx->io_buffer));

    if (ret != sizeof(ctx->io_buffer)) {
        if (ret == -1) {
            LOGE("Failed to read, error: %d\n", errno);
        } else {
            LOGE("Failed to read %d bytes, error: %d\n", sizeof(ctx->io_buffer), ret);
        }

        return (1);
    }
    post_process(ctx);

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_samplingClose(
        JNIEnv *env,
        jobject /* this */) {
    struct rtcomm_ctx *         ctx = &g_ctx;

    if (ioctl(ctx->driver_fd, RTCOMM_STOP)) {
        LOGE("RTCOMM_STOP failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_STOP success.\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_TERM)) {
        LOGE("RTCOMM_TERM failed: %d\n", errno);

        return (1);
    } else {
        LOGI("RTCOMM_TERM success.\n");
    }
    close(ctx->driver_fd);

    return (0);
}


extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_i2cWrReg(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg,
                                                        jint data)
{
    char                        buff[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    char                        _data;

    if (chip_address > 255) {
        return (-1);
    }

    if (reg > 255) {
        return (-1);
    }
    _reg = (uint8_t)reg;

    if (data > 255) {
        return (-1);
    }
    _data = (uint8_t)data;

    snprintf(buff, sizeof(buff), "/dev/i2c-%d", bus_id);

    fd = open(buff, O_RDWR);

    if (fd < 0) {
        if (errno == ENOENT) {
            return (-1);
        } else {
            return (-2);
        }
    }

    if (ioctl(fd, I2C_SLAVE, chip_address) < 0) {
        close(fd);

        return (-2);
    }

    status = write(fd, &_reg, 1);

    if (status != 1) {
        return (-3);
    }

    status = write(fd, &_data, 1);

    if (status != 1) {
        return (-3);
    }
    close(fd);

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_MainActivity_i2cRdReg(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg)
{
    char                        buff[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    char                        _data;
    uint8_t *                   retval;

    if (chip_address > 255) {
        return (-1);
    }

    if (reg > 255) {
        return (-1);
    }
    _reg = (uint8_t)reg;

    snprintf(buff, sizeof(buff), "/dev/i2c-%d", bus_id);

    fd = open(buff, O_RDWR);

    if (fd < 0) {
        if (errno == ENOENT) {
            return (-1);
        } else {
            return (-2);
        }
    }

    if (ioctl(fd, I2C_SLAVE, chip_address) < 0) {
        close(fd);

        return (-2);
    }

    status = write(fd, &_reg, 1);

    if (status != 1) {
        return (-3);
    }

    status = read(fd, &_data, 1);

    if (status != 1) {
        return (-3);
    }
    close(fd);
    retval = (uint8_t *)&_data;

    return ((jint)*retval);
}

extern "C"
jintArray
Java_com_teslameter_nr_teslameter_MainActivity_i2cRdBuf(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg,
                                                        jint bufsize)
{
    char                        buff[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    uint8_t                     l_buffer[100];
    jint                        li_buffer[100];


    if (chip_address > 255) {
        return (NULL);
    }

    if (bufsize > sizeof(l_buffer)) {
        return (NULL);
    }

    if (reg > 255) {
        return (NULL);
    }
    _reg = (uint8_t)reg;

    snprintf(buff, sizeof(buff), "/dev/i2c-%d", bus_id);

    fd = open(buff, O_RDWR);

    if (fd < 0) {
        if (errno == ENOENT) {
            return (NULL);
        } else {
            return (NULL);
        }
    }

    if (ioctl(fd, I2C_SLAVE, chip_address) < 0) {
        close(fd);

        return (NULL);
    }

    status = write(fd, &_reg, 1);

    if (status != 1) {
        return (NULL);
    }

    status = read(fd, l_buffer, bufsize);

    if (status != bufsize) {
        return (NULL);
    }
    close(fd);

    for (int i = 0; i < bufsize; i++) {
        li_buffer[i] = l_buffer[i];
    }

    jintArray retval = env->NewIntArray(bufsize);

    if (retval == NULL) {
        return NULL; /* out of memory error thrown */
    }

    env->SetIntArrayRegion(retval, 0, bufsize, li_buffer);
    return (retval);
}
