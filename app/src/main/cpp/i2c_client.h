//
// Created by nenad on 6/7/17.
//

#ifndef TESLAMETER_I2C_CLIENT_H
#define TESLAMETER_I2C_CLIENT_H

#include <jni.h>

extern "C"
jint
Java_com_teslameter_nr_teslameter_I2cSlave_i2cWrReg(JNIEnv *env,
                                                     jobject /* this */,
                                                     jint bus_id, jint chip_address, jint reg,
                                                     jint data);

extern "C"
jint
Java_com_teslameter_nr_teslameter_I2cSlave_i2cRdReg(JNIEnv *env,
                                                     jobject /* this */,
                                                     jint bus_id, jint chip_address, jint reg);

extern "C"
jintArray
Java_com_teslameter_nr_teslameter_I2cSlave_i2cRdBuf(JNIEnv *env,
                                                     jobject /* this */,
                                                     jint bus_id, jint chip_address, jint reg,
                                                     jint bufsize);

#endif //TESLAMETER_I2C_CLIENT_H
