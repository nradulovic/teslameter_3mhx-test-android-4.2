/*
 *  teslameter_3mhx-android - 2017
 *
 *  native-lib.cpp
 *
 *  Created on: Jun 8, 2017
 * ------------------------------------------------------------------------------------------------
 *  This file is part of teslameter_3mhx-android.
 *
 *  teslameter_3mhx-android is free software: you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  teslameter_3mhx-android is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with teslameter_3mhx-android.  If not, see <http://www.gnu.org/licenses/>.
 * ------------------------------------------------------------------------------------------ *//**
 * @file
 * @author      Nenad Radulovic
 * @brief       Java Native Interface - RTCOMM communication
 *****************************************************************************************//** @{ */
/*=============================================================================  INCLUDE FILES  ==*/

#include <jni.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>
#include <sys/ioctl.h>
#include <android/log.h>

#include "cdi_manager.h"
#include "rtcomm/rtcomm.h"
#include "teslameter_3mhx-cdi/io.h"

/*=============================================================================  LOCAL MACRO's  ==*/

/* Andoird logger for C
 * --------------------
 */
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

/* Other macros
 * ------------
 */
#define VQUANT_MV                       (((VREF * 2.0) / (8388608 - 1)) * 1000.0)

/*==========================================================================  LOCAL DATA TYPES  ==*/

/* RTCOMM-JNI context structure
 * ----------------------------
 */
struct rtcomm_ctx
{
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
    struct sysinfo
    {
        char                        drv_version[20];
    }                           sysinfo;
    volatile bool               should_exit;
};

/*=================================================================  LOCAL FUNCTION PROTOTYPES  ==*/
/*===========================================================================  LOCAL VARIABLES  ==*/

static const char*              kTAG = "rtcomm-jni";

static struct rtcomm_ctx        g_ctx;

/*==========================================================================  GLOBAL VARIABLES  ==*/
/*================================================================  LOCAL FUNCTION DEFINITIONS  ==*/

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
        /*
         * TODO: Resample the buffer to 800 samples
         */
        memcpy(&ctx->pp_buffer.raw.probe_window[0][0], &ctx->io_buffer.sample[0][0],
               sizeof(int32_t) * IO_PROBE_CHANNELS * 800);
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

/*===============================================================  GLOBAL FUNCTION DEFINITIONS  ==*/

/* -- Initialization ---------------------------------------------------------------------------- */

JNI_CDI_MANAGER(jint, rtcommInit) (JNIEnv *env, jobject this_obj, jintArray config_array)
{
    (void)env;
    (void)this_obj;

    return (0);
}

JNI_CDI_MANAGER(jint, rtcommTerminate) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    return (0);
}

/* -- DATA access methods ----------------------------------------------------------------------- */

JNI_CDI_MANAGER(jint, dataAcquire) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    return (0);
}

JNI_CDI_MANAGER(jint, dataRelease) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    return (0);
}

JNI_CDI_MANAGER(jint, dataProbeXRaw) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_X));
}

JNI_CDI_MANAGER(jint, dataProbeYRaw) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Y));
}

JNI_CDI_MANAGER(jint, dataProbeZRaw) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_probe_x_raw(ctx, IO_CHANNEL_Z));
}

JNI_CDI_MANAGER(jint, dataAuxRaw) (JNIEnv *env, jobject this_obj, jint mchannel)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return (get_aux_raw(ctx, mchannel));
}

JNI_CDI_MANAGER(jfloat, dataProbeXVoltage) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return ((float)(get_probe_x_raw(ctx, IO_CHANNEL_X) * VQUANT_MV));
}

JNI_CDI_MANAGER(jfloat, dataProbeYVoltage) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return ((float)(get_probe_x_raw(ctx, IO_CHANNEL_Y) * VQUANT_MV));
}

JNI_CDI_MANAGER(jfloat, dataProbeZVoltage) (JNIEnv *env, jobject this_obj)
{
    (void)env;
    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    return ((float)(get_probe_x_raw(ctx, IO_CHANNEL_Z) * VQUANT_MV));
}

JNI_CDI_MANAGER(jfloat, dataAuxVoltage) (JNIEnv *env, jobject this_obj, jint mchannel)
{
    struct rtcomm_ctx *         ctx = &g_ctx;

    (void)env;
    (void)this_obj;

    return ((float)(get_aux_raw(ctx, mchannel) * VQUANT_MV));
}

/* Array methods */

JNI_CDI_MANAGER(jintArray , dataProbeXRawArray) (JNIEnv *env, jobject this_obj)
{
    jint                        jint_buffer[IO_BUFF_SIZE];
    jintArray                   retval;
    int                         idx;

    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    retval = env->NewIntArray(ctx->pp_buffer.no_samples);

    if (retval == NULL) {
        return NULL;
    }

    for (idx = 0; idx < ctx->pp_buffer.no_samples; idx++) {
        jint_buffer[idx] = ctx->pp_buffer.raw.probe_array[idx][IO_CHANNEL_X];
    }
    env->SetIntArrayRegion(retval, 0, ctx->pp_buffer.no_samples, jint_buffer);

    return (retval);
}

JNI_CDI_MANAGER(jintArray, dataProbeYRawArray) (JNIEnv *env, jobject this_obj)
{
    jint                        jint_buffer[IO_BUFF_SIZE];
    jintArray                   retval;
    int                         idx;

    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    retval = env->NewIntArray(ctx->pp_buffer.no_samples);

    if (retval == NULL) {
        return NULL;
    }

    for (idx = 0; idx < ctx->pp_buffer.no_samples; idx++) {
        jint_buffer[idx] = ctx->pp_buffer.raw.probe_array[idx][IO_CHANNEL_Y];
    }
    env->SetIntArrayRegion(retval, 0, ctx->pp_buffer.no_samples, jint_buffer);

    return (retval);
}

JNI_CDI_MANAGER(jintArray, dataProbeZRawArray) (JNIEnv *env, jobject this_obj)
{
    jint                        jint_buffer[IO_BUFF_SIZE];
    jintArray                   retval;
    int                         idx;

    (void)this_obj;

    struct rtcomm_ctx *         ctx = &g_ctx;

    retval = env->NewIntArray(ctx->pp_buffer.no_samples);

    if (retval == NULL) {
        return NULL;
    }

    for (idx = 0; idx < ctx->pp_buffer.no_samples; idx++) {
        jint_buffer[idx] = ctx->pp_buffer.raw.probe_array[idx][IO_CHANNEL_Z];
    }
    env->SetIntArrayRegion(retval, 0, ctx->pp_buffer.no_samples, jint_buffer);

    return (retval);
}

/* Misc methods */

JNI_CDI_MANAGER(jstring, dataGetStats) (JNIEnv *env, jobject this_obj)
{
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char                 buffer[300];
    (void)this_obj;

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

JNI_CDI_MANAGER(jstring, dataGetInfos) (JNIEnv *env, jobject this_obj)
{
    struct rtcomm_ctx *         ctx = &g_ctx;
    static char buffer[300];
    static uint32_t             header_invalid_count;
    static uint32_t             footer_invalid_count;
    uint32_t                    data_size = 0;
    uint32_t                    frame = 0;
    bool                        is_valid_footer = false;
    bool                        is_valid_header = false;

    (void)this_obj;

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

JNI_CDI_MANAGER(jint, samplingOpen) (JNIEnv *env, jobject this_obj)
{
    struct rtcomm_ctx *         ctx = &g_ctx;
    int                         buffer_size = sizeof(ctx->io_buffer);
    int                         err;
    (void)env;
    (void)this_obj;

    ctx->driver_fd = open("/dev/" RTCOMM_NAME, O_RDONLY);

    if (ctx->driver_fd == -1) {
        LOGE("Failed to open driver: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("Opened /dev/" RTCOMM_NAME "\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_GET_VERSION, &ctx->sysinfo.drv_version[0]) == -1) {
        LOGE("RTCOMM_GET_VERSION failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_GET_VERSION get: %s\n", ctx->sysinfo.drv_version);
    }

    if (ioctl(ctx->driver_fd, RTCOMM_SET_SIZE, &buffer_size) == -1) {
        LOGE("RTCOMM_SET_SIZE failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_SET_SIZE set: %d\n", buffer_size);
    }

    if (ioctl(ctx->driver_fd, RTCOMM_INIT) == -1) {
        LOGE("RTCOMM_INIT failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_INIT success.\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_START) == -1) {
        LOGE("RTCOMM_START failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_START success.\n");
    }

    return (0);
FAILURE_EXIT:
    err = errno;

    if (ctx->driver_fd != -1) {
        close(ctx->driver_fd);
    }

    return (err);
}

JNI_CDI_MANAGER(jint, samplingRefresh) (JNIEnv *env, jobject this_obj)
{
    struct rtcomm_ctx *         ctx = &g_ctx;
    int                         ret;
    (void)env;
    (void)this_obj;

    ret = read(ctx->driver_fd, &ctx->io_buffer, sizeof(ctx->io_buffer));

    if (ret != sizeof(ctx->io_buffer)) {
        if (ret == -1) {
            LOGE("Failed to read, error: %d\n", errno);

            return (errno);
        } else {
            LOGE("Failed to read %d bytes\n", sizeof(ctx->io_buffer));

            return (ECANCELED);
        }
    }
    post_process(ctx);

    return (0);
}

JNI_CDI_MANAGER(jint, samplingClose) (JNIEnv *env, jobject this_obj)
{
    struct rtcomm_ctx *         ctx = &g_ctx;
    int                         err;
    (void)env;
    (void)this_obj;

    if (ioctl(ctx->driver_fd, RTCOMM_STOP)) {
        LOGE("RTCOMM_STOP failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_STOP success.\n");
    }

    if (ioctl(ctx->driver_fd, RTCOMM_TERM)) {
        LOGE("RTCOMM_TERM failed: %d\n", errno);

        goto FAILURE_EXIT;
    } else {
        LOGI("RTCOMM_TERM success.\n");
    }

    return (0);
FAILURE_EXIT:
    err = errno;

    close(ctx->driver_fd);

    return (err);
}

/*====================================================*//** @cond *//*==  CONFIGURATION ERRORS  ==*/
/** @endcond *//** @} *//**************************************************************************
 * END of native-lib.cpp
 **************************************************************************************************/
