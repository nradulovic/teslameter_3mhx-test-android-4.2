/*
 * This file is part of teslameter_3mhx-android.
 *
 * Copyright (C) 2010 - 2017 nenad
 *
 * teslameter_3mhx-android is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * teslameter_3mhx-android is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with teslameter_3mhx-android.  If not, see <http://www.gnu.org/licenses/>.
 *
 * web site:    
 * e-mail  :    
 *//*******************************************************************************************//**
 * @file
 * @author      Nenad Radulovic
 * @brief       I2C Slave C interface
 *****************************************************************************************//** @{ */

#ifndef TESLAMETER_I2C_SLAVE_H
#define TESLAMETER_I2C_SLAVE_H

/*=============================================================================  INCLUDE FILES  ==*/

#include <jni.h>

/*===================================================================================  MACRO's  ==*/

/* ---------------------------------------------------------------------------------------------- *
 * JNI MACRO HELPER                                                                               *
 * ---------------------------------------------------------------------------------------------- *
 *                                                                                                *
 * Use the macro JNI_I2C_SLAVE_PACKAGE to easily define which package this JNI .cpp file belongs  *
 * to.                                                                                            *
 *                                                                                                *
 * ---------------------------------------------------------------------------------------------- */

#define JNI_I2C_SLAVE_PACKAGE              Java_com_teslameter_nr_teslameter_I2cSlave_

#define JNI_I2C_SLAVE_P(rettype, package, name)                                                     \
    extern "C" rettype package ## name

#define JNI_I2C_SLAVE_PP(rettype, package, name)                                                    \
    JNI_I2C_SLAVE_P(rettype, package, name)

#define JNI_I2C_SLAVE(rettype, name)                                                                \
    JNI_I2C_SLAVE_PP(rettype, JNI_I2C_SLAVE_PACKAGE, name)

/*================================================================================  DATA TYPES  ==*/
/*==========================================================================  GLOBAL VARIABLES  ==*/
/*=======================================================================  FUNCTION PROTOTYPES  ==*/

JNI_I2C_SLAVE(jint, i2cWrReg) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg, jint data);

JNI_I2C_SLAVE(jint, i2cRdReg) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg);

JNI_I2C_SLAVE(jintArray, i2cRdBuf) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                                    jint reg, jint bufsize);

JNI_I2C_SLAVE(jint, i2cWrBuf) (JNIEnv *env, jobject this_obj, jint bus_id, jint chip_address,
                               jint reg, jintArray buf);

/*====================================================*//** @cond *//*==  CONFIGURATION ERRORS  ==*/
/** @endcond *//** @} *//**************************************************************************
 * END of i2c_client.h
 **************************************************************************************************/
#endif /* TESLAMETER_I2C_SLAVE_H */
