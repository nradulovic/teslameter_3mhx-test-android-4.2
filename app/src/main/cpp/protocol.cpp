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
//#include <termios.h>
#include <termios.h>
#include <android/log.h>
#include <stdio.h>

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

static
int set_interface_attribs(int fd, int speed, int parity)
{
    int                         error;
    struct termios              tty;
    memset (&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        error = errno;
        LOGE("set_interface_attribs: error %d from tcgetattr", error);

        return -error;
    }
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    /* NOTE:
     * Android termios C library is broken. We __must__ use literal here for IOCTL call to succeed.
     */
    if (tcsetattr(fd, 0x5402, &tty) != 0) {
        error = errno;
        LOGE("set_interface_attribs: error %d from tcsetattr", error);

        return -error;
    }

    return 0;
}

static
int set_blocking(int fd, int should_block)
{
    int                         error;
    struct termios              tty;

    memset (&tty, 0, sizeof tty);

    if (tcgetattr (fd, &tty) != 0) {
        error = errno;
        LOGE("set_blocking: error %d from tggetattr", error);

        return -error;
    }
    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, 0x5402, &tty) != 0) {
        error = errno;
        LOGE("set_blocking: error %d setting term attributes", error);

        return -error;
    }

    return (0);
}

/*===============================================================  GLOBAL FUNCTION DEFINITIONS  ==*/

JNI_PROTOCOL(jint, protocolOpen) (JNIEnv *env, jobject this_obj)
{
    struct termios              options;
    struct protocol_ctx *       ctx = &g_ctx;
    int                         error;

    LOGI("Open: openning %s", PROTOCOL_UART_DEVICE);
    ctx->fd = open(PROTOCOL_UART_DEVICE, O_RDWR | O_NOCTTY);

    if (ctx->fd == -1) {
        error = errno;
        LOGE("Open: failed to open file: %d", error);

        return (-error);
    }
    error = set_interface_attribs(ctx->fd, B115200, 0);

    if (error) {
        LOGE("Open: failed to set config: %d", error);

        return (-error);
    }
    error = set_blocking(ctx->fd, true);

    if (error) {
        LOGE("Open: failed to set blocking: %d", error);

        return (-error);
    }
    ctx->is_initialized = true;

    LOGI("Open: open successful");

    return (0);
}

JNI_PROTOCOL(jint, protocolClose) (JNIEnv *env, jobject this_obj)
{
    struct protocol_ctx *       ctx = &g_ctx;

    if (ctx->is_initialized) {
        LOGI("Close: closing");
        close(ctx->fd);
    }

    return (0);
}

JNI_PROTOCOL(jint, protocolRdByte) (JNIEnv *env, jobject this_obj)
{
    ssize_t                     retval;
    unsigned char               character;
    struct protocol_ctx *       ctx = &g_ctx;
    int                         err;

    if (!ctx->is_initialized) {
        LOGE("RdByte: context is not initialized");

        return (-ENODEV);
    }

    retval = read(ctx->fd, &character, 1);

    if (retval != 1) {

        if (retval < 0) {
            err = errno;
            LOGE("RdByte: failed to read file: %d", err);
        } else {
            err = EAGAIN;
            LOGE("RdByte: read zero bytes");
        }

        return (-err);
    } else {
        LOGI("RdByte: \'%c\' (0x%x)", character, character);

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
        LOGE("WrBuf: context is not initialized");

        return (-ENODEV);
    }
    bufsize = env->GetArrayLength(buf);
    byte_buffer = (uint8_t *)malloc(bufsize + 1);

    if (!byte_buffer) {
        LOGE("WrBuf: unable to allocate Java array of size %d", bufsize);
        err = -ENOBUFS;
        goto FAILURE_EXIT;
    }
    memset(byte_buffer, 0, bufsize + 1);
    jint_buffer = env->GetIntArrayElements(buf, NULL);

    /* Copy data from Java array to plain byte buffer */
    for (int idx = 0; idx < bufsize; idx++) {
        byte_buffer[idx] = (uint8_t)jint_buffer[idx];
    }
    /* Inform the user through logger */
    {
        size_t str_len = strlen((char *)byte_buffer);

        if (str_len == bufsize) {
            LOGI("WrBuf: \'%s\'", byte_buffer);
        } else {
            char local_buf[100];

            memset(local_buf, 0, sizeof(local_buf));

            for (int idx = 0; idx < bufsize; idx++) {
                int             free_space;
                char            one_byte_buf[10];

                snprintf(&one_byte_buf[idx], sizeof(one_byte_buf), "%x", byte_buffer[idx]);
                free_space = sizeof(local_buf) - strlen(local_buf);

                if (free_space > strlen(one_byte_buf)) {
                    strncat(local_buf, one_byte_buf, free_space);
                } else {
                    break;
                }
            }
            LOGI("WrBuf: (%s)", local_buf);
        }
    }
    status = write(ctx->fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = -errno;
            LOGE("WrBuf: failed to write file: %d", err);
        } else {
            err = -EAGAIN;
            LOGE("WrBuf: wrote only %d of %d bytes", status, bufsize);
        }
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