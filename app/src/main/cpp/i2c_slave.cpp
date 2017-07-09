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
 * @brief       I2C Slave C implementation
 *****************************************************************************************//** @{ */
/*=============================================================================  INCLUDE FILES  ==*/

#include <jni.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <android/log.h>

#include "i2c-dev.h"
#include "i2c_slave.h"

/*=============================================================================  LOCAL MACRO's  ==*/
/*==========================================================================  LOCAL DATA TYPES  ==*/
/*=================================================================  LOCAL FUNCTION PROTOTYPES  ==*/
/*===========================================================================  LOCAL VARIABLES  ==*/
/*==========================================================================  GLOBAL VARIABLES  ==*/
/*================================================================  LOCAL FUNCTION DEFINITIONS  ==*/
/*===============================================================  GLOBAL FUNCTION DEFINITIONS  ==*/

JNI_I2C_SLAVE(jint, i2cWrReg) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg, jint data)
{
    char                        buff[100];
    int                         fd;
    ssize_t                     transferred;
    char                        _reg;
    char                        _data;
    int                         status;
    int                         err;

    (void)env;
    (void)this_obj;

    _reg = (uint8_t)reg;
    _data = (uint8_t)data;

    snprintf(buff, sizeof(buff), "/dev/i2c-%d", bus_id);

    fd = open(buff, O_RDWR);

    if (fd < 0) {
        return (-errno);
    }

    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = -errno;
        close(fd);

        return (err);
    }
    transferred = write(fd, &_reg, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);

        return (err);
    }

    transferred = write(fd, &_data, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);

        return (err);
    }
    close(fd);

    return (0);
}

JNI_I2C_SLAVE(jint, i2cRdReg) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     transferred;
    char                        _reg;
    char                        _data;
    int                         status;
    int                         err;

    (void)env;
    (void)this_obj;

    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);

    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        return (-errno);
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = -errno;
        close(fd);

        return (err);
    }
    _reg = (uint8_t)reg;
    transferred = write(fd, &_reg, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);

        return (err);
    }
    transferred = read(fd, &_data, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);

        return (err);
    }
    close(fd);

    return ((jint)*(uint8_t *)&_data);
}

JNI_I2C_SLAVE(jintArray, i2cRdBuf) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                                    jint reg, jint bufsize)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    uint8_t *                   byte_buffer = NULL;
    jint *                      jint_buffer = NULL;
    int                         err;
    jintArray                   retval;

    (void)env;
    (void)this_obj;

    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);

    byte_buffer = (uint8_t  *)malloc(bufsize);

    if (!byte_buffer) {
        err = -ENOBUFS;
        goto FAILURE_EXIT;
    }

    jint_buffer = (jint *)malloc(sizeof(jint) * bufsize);

    if (!jint_buffer) {
        err = -ENOBUFS;
        goto FAILURE_EXIT;
    }

    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        err = -errno;
        goto FAILURE_EXIT;
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = -errno;
        close(fd);
        goto FAILURE_EXIT;
    }
    _reg = (uint8_t)reg;
    status = write(fd, &_reg, 1);

    if (status != 1) {
        if (status < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }

    status = read(fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }
    close(fd);

    for (int idx = 0; idx < bufsize; idx++) {
        jint_buffer[idx] = byte_buffer[idx];
    }

    retval = env->NewIntArray(bufsize);

    if (retval != NULL) {
        env->SetIntArrayRegion(retval, 0, bufsize, jint_buffer);
    }
    free(byte_buffer);
    free(jint_buffer);

    return (retval);
FAILURE_EXIT:
    jint jint_value = err * 256;
    retval = env->NewIntArray(1);

    if (retval != NULL) {
        env->SetIntArrayRegion(retval, 0, 1, &jint_value);
    }

    if (byte_buffer) {
        free(byte_buffer);
    }

    if (jint_buffer) {
        free(jint_buffer);
    }
    return (retval);
}

JNI_I2C_SLAVE(jint, i2cWrBuf) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg, jintArray buf)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    uint8_t *                   byte_buffer = NULL;
    jint *                      jint_buffer = NULL;
    int                         err;
    jsize                       bufsize;

    (void)env;
    (void)this_obj;

    bufsize = env->GetArrayLength(buf);
    byte_buffer = (uint8_t *)malloc(bufsize);

    if (!byte_buffer) {
        err = -ENOBUFS;
        goto FAILURE_EXIT;
    }
    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);
    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        err = -errno;
        goto FAILURE_EXIT;
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = -errno;
        close(fd);
        goto FAILURE_EXIT;
    }
    _reg = (uint8_t)reg;
    status = write(fd, &_reg, 1);

    if (status != 1) {
        if (status < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }
    jint_buffer = env->GetIntArrayElements(buf, NULL);

    for (int idx = 0; idx < bufsize; idx++) {
        byte_buffer[idx] = (uint8_t)jint_buffer[idx];
    }
    status = write(fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = -errno;
        } else {
            err = -EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }
    close(fd);

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
 * END of i2c_client.cpp
 **************************************************************************************************/