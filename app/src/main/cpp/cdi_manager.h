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
 * @brief       CDI Manager C interface
 *****************************************************************************************//** @{ */

#ifndef TESLAMETER_CDI_MANAGER_H
#define TESLAMETER_CDI_MANAGER_H

/*=============================================================================  INCLUDE FILES  ==*/

#include <jni.h>

/*===================================================================================  MACRO's  ==*/

/* ---------------------------------------------------------------------------------------------- *
 * JNI MACRO HELPER                                                                               *
 * ---------------------------------------------------------------------------------------------- *
 *                                                                                                *
 * Use the macro JNI_CDI_MANAGER_PACKAGE to easily define which package this JNI .cpp file        *
 * belongs to.                                                                                    *
 *                                                                                                *
 * ---------------------------------------------------------------------------------------------- */

#define JNI_CDI_MANAGER_PACKAGE         Java_com_teslameter_nr_teslameter_CdiManager_

#define JNI_CDI_MANAGER_P(rettype, package, name)                                                   \
    extern "C" rettype package ## name

#define JNI_CDI_MANAGER_PP(rettype, package, name)                                                  \
    JNI_CDI_MANAGER_P(rettype, package, name)

#define JNI_CDI_MANAGER(rettype, name)                                                              \
    JNI_CDI_MANAGER_PP(rettype, JNI_CDI_MANAGER_PACKAGE, name)


/* Compilation/runtime constants
 * -----------------------------
 * These constants should be part of initialization process and setup during the runtime, eg. when a
 * class is instanced. Since we must finish the project in less than 24 hours, then fuck it. Define
 * the constants here.
 * */
#define WINDOW_BUFF_SIZE                800
#define VREF                            2.5

/*================================================================================  DATA TYPES  ==*/
/*==========================================================================  GLOBAL VARIABLES  ==*/
/*=======================================================================  FUNCTION PROTOTYPES  ==*/

JNI_CDI_MANAGER(jint, rtcommInit) (JNIEnv *env, jobject this_obj, jintArray config_array);

JNI_CDI_MANAGER(jint, rtcommTerminate) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataAcquire) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataRelease) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataProbeXRaw) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataProbeYRaw) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataProbeZRaw) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, dataAuxRaw) (JNIEnv *env, jobject this_obj, jint mchannel);

JNI_CDI_MANAGER(jfloat, dataProbeXVoltage) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jfloat, dataProbeYVoltage) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jfloat, dataProbeZVoltage) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jfloat, dataAuxVoltage) (JNIEnv *env, jobject this_obj, jint mchannel);

JNI_CDI_MANAGER(jintArray , dataProbeXRawArray) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jintArray , dataProbeYRawArray) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jintArray , dataProbeZRawArray) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jstring, dataGetStats) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jstring, dataGetInfos) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, samplingOpen) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, samplingRefresh) (JNIEnv *env, jobject this_obj);

JNI_CDI_MANAGER(jint, samplingClose) (JNIEnv *env, jobject this_obj);

/*====================================================*//** @cond *//*==  CONFIGURATION ERRORS  ==*/
/** @endcond *//** @} *//**************************************************************************
 * END of cdi_manager.h
 **************************************************************************************************/
#endif /* TESLAMETER_CDI_MANAGER_H */