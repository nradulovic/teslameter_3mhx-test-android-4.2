/*
 *  teslameter_3mhx-android - 2017
 *
 *  native-lib.cpp
 *
 *  Created on: Jun 5, 2017
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
 * @brief       Protocol JNI implementation
 *****************************************************************************************//** @{ */
/*=============================================================================  INCLUDE FILES  ==*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <android/log.h>

#include "protocol.h"

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

/*==========================================================================  LOCAL DATA TYPES  ==*/

struct protocol_ctx {
    int                         fd;
    bool                        is_initialized;
};

/*=================================================================  LOCAL FUNCTION PROTOTYPES  ==*/
/*===========================================================================  LOCAL VARIABLES  ==*/

static const char*              kTAG = "protocol-jni";

static struct protocol_ctx      g_ctx;

/*==========================================================================  GLOBAL VARIABLES  ==*/
/*================================================================  LOCAL FUNCTION DEFINITIONS  ==*/
/*===============================================================  GLOBAL FUNCTION DEFINITIONS  ==*/

JNI_PROTOCOL(jint, protocolOpen) (JNIEnv *env, jobject this_obj)
{
    struct termios              options;
    struct protocol_ctx *       ctx = &g_ctx;

    ctx->fd = open(PROTOCOL_UART_DEVICE, O_RDWR | O_NOCTTY);

    if (ctx->fd == -1) {
        return (-errno);
    }
    tcgetattr(ctx->fd, &options);
    /* NOTE:
     * B115200 - 155200 baud
     * CS8 - 8bit
     * CLOCAL - ignore modem status lines
     * CREAD - enable receiver
     * IGNBRK, ICRNL - disable break processing, disable translate carriage return to newline
     */
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag &= (unsigned)~(IGNBRK | INLCR | ICRNL);
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(ctx->fd, TCIFLUSH);
    tcsetattr(ctx->fd, TCSANOW, &options);
    ctx->is_initialized = true;

    return (0);
}

JNI_PROTOCOL(jint, protocolClose) (JNIEnv *env, jobject this_obj)
{
    struct protocol_ctx *       ctx = &g_ctx;

    if (ctx->is_initialized) {
        close(ctx->fd);
    }

    return (0);
}

JNI_PROTOCOL(jint, protocolRdByte) (JNIEnv *env, jobject this_obj)
{
    ssize_t                     retval;
    unsigned char               character;
    struct protocol_ctx *       ctx = &g_ctx;

    retval = read(ctx->fd, &character, 1);

    if (!ctx->is_initialized) {
        return (-ENODEV);
    }

    if (retval != 1) {
        LOGE("Failed to read protocol file: %d", errno);

        return (-errno);
    } else {
        return (character);
    }
}

JNI_PROTOCOL(jint, protocolWrBuf) (JNIEnv *env, jobject this_obj, jintArray buf)
{
    ssize_t                     status;
    jsize                       bufsize;
    uint8_t *                   byte_buffer = NULL;
    jint *                      jint_buffer = NULL;
    int                         err;
    struct protocol_ctx *       ctx = &g_ctx;

    if (!ctx->is_initialized) {
        return (-ENODEV);
    }
    bufsize = env->GetArrayLength(buf);
    byte_buffer = (uint8_t *)malloc(bufsize);

    if (!byte_buffer) {
        err = -ENOBUFS;
        goto FAILURE_EXIT;
    }

    jint_buffer = env->GetIntArrayElements(buf, NULL);

    for (int idx = 0; idx < bufsize; idx++) {
        byte_buffer[idx] = (uint8_t)jint_buffer[idx];
    }

    status = write(ctx->fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(ctx->fd);
        goto FAILURE_EXIT;
    }

    env->ReleaseIntArrayElements(buf, jint_buffer, 0);
    free(byte_buffer);

    return (0);
FAILURE_EXIT:

    if (jint_buffer) {
        env->ReleaseIntArrayElements(buf, jint_buffer, 0);
    }

    if (byte_buffer) {
        free(byte_buffer);
    }

    return (err);
}

/*====================================================*//** @cond *//*==  CONFIGURATION ERRORS  ==*/
/** @endcond *//** @} *//**************************************************************************
 * END of protocol.cpp
 **************************************************************************************************/