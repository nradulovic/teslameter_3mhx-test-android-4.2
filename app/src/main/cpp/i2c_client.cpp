//
// Created by nenad on 6/7/17.
//
#include <jni.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "i2c-dev.h"
#include "i2c_client.h"


extern "C"
jint
Java_com_teslameter_nr_teslameter_I2cSlave_i2cWrReg(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg,
                                                        jint data)
{
    char                        buff[100];
    int                         fd;
    ssize_t                     transferred;
    char                        _reg;
    char                        _data;
    int                         status;
    int                         err;

    _reg = (uint8_t)reg;
    _data = (uint8_t)data;

    snprintf(buff, sizeof(buff), "/dev/i2c-%d", bus_id);

    fd = open(buff, O_RDWR);

    if (fd < 0) {
        return (errno);
    }

    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = errno;
        close(fd);

        return (err);
    }
    transferred = write(fd, &_reg, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);

        return (err);
    }

    transferred = write(fd, &_data, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);

        return (err);
    }
    close(fd);

    return (0);
}

extern "C"
jint
Java_com_teslameter_nr_teslameter_I2cSlave_i2cRdReg(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     transferred;
    char                        _reg;
    char                        _data;
    int                         status;
    int                         err;

    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);

    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        return (errno);
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = errno;
        close(fd);

        return (err);
    }
    _reg = (uint8_t)reg;
    transferred = write(fd, &_reg, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);

        return (err);
    }
    transferred = read(fd, &_data, 1);

    if (transferred != 1) {
        if (transferred < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);

        return (err);
    }
    close(fd);

    return ((jint)*(uint8_t *)&_data);
}

extern "C"
jintArray
Java_com_teslameter_nr_teslameter_I2cSlave_i2cRdBuf(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint bus_id, jint chip_address, jint reg,
                                                        jint bufsize)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    uint8_t *                   byte_buffer = NULL;
    jint *                      jint_buffer = NULL;
    int                         err;
    jintArray                   retval;

    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);

    byte_buffer = (uint8_t  *)malloc(bufsize);

    if (!byte_buffer) {
        err = ENOBUFS;
        goto FAILURE_EXIT;
    }

    jint_buffer = (jint *)malloc(sizeof(jint) * bufsize);

    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        err = errno;
        goto FAILURE_EXIT;
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = errno;
        close(fd);
        goto FAILURE_EXIT;
    }
    _reg = (uint8_t)reg;
    status = write(fd, &_reg, 1);

    if (status != 1) {
        if (status < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }

    status = read(fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = errno;
        } else {
            err = EAGAIN;
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
    jint_buffer[0] = err;
    retval = env->NewIntArray(1);

    if (retval != NULL) {
        env->SetIntArrayRegion(retval, 0, 1, jint_buffer);
    }

    if (byte_buffer) {
        free(byte_buffer);
    }

    if (jint_buffer) {
        free(jint_buffer);
    }
    return (retval);
}

extern "C"
jintArray
Java_com_teslameter_nr_teslameter_I2cSlave_i2cWrBuf(JNIEnv *env,
                                                    jobject /* this */,
                                                    jint bus_id, jint chip_address, jint reg,
                                                    jintArray buf)
{
    char                        device_name[100];
    int                         fd;
    ssize_t                     status;
    char                        _reg;
    uint8_t *                   byte_buffer = NULL;
    jint *                      jint_buffer = NULL;
    int                         err;
    jintArray                   retval;

    snprintf(device_name, sizeof(device_name), "/dev/i2c-%d", bus_id);

    byte_buffer = (uint8_t  *)malloc(bufsize);

    if (!byte_buffer) {
        err = ENOBUFS;
        goto FAILURE_EXIT;
    }

    jint_buffer = (jint *)malloc(sizeof(jint) * bufsize);

    fd = open(device_name, O_RDWR);

    if (fd < 0) {
        err = errno;
        goto FAILURE_EXIT;
    }
    status = ioctl(fd, I2C_SLAVE, chip_address);

    if (status < 0) {
        err = errno;
        close(fd);
        goto FAILURE_EXIT;
    }
    _reg = (uint8_t)reg;
    status = write(fd, &_reg, 1);

    if (status != 1) {
        if (status < 0) {
            err = errno;
        } else {
            err = EAGAIN;
        }
        close(fd);
        goto FAILURE_EXIT;
    }

    status = read(fd, byte_buffer, bufsize);

    if (status != bufsize) {
        if (status < 0) {
            err = errno;
        } else {
            err = EAGAIN;
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
    jint_buffer[0] = err;
    retval = env->NewIntArray(1);

    if (retval != NULL) {
        env->SetIntArrayRegion(retval, 0, 1, jint_buffer);
    }

    if (byte_buffer) {
        free(byte_buffer);
    }

    if (jint_buffer) {
        free(jint_buffer);
    }
    return (retval);
}
