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
 * @brief       Protocol C interface
 *****************************************************************************************//** @{ */

#ifndef TESLAMETER_PROTOCOL_H
#define TESLAMETER_PROTOCOL_H

/*=============================================================================  INCLUDE FILES  ==*/

#include <jni.h>

/*===================================================================================  MACRO's  ==*/

/* ---------------------------------------------------------------------------------------------- *
 * JNI MACRO HELPER                                                                               *
 * ---------------------------------------------------------------------------------------------- *
 *                                                                                                *
 * Use the macro JNI_PROTOCOL_PACKAGE to easily define which package this JNI .cpp file belongs   *
 * to.                                                                                            *
 *                                                                                                *
 * ---------------------------------------------------------------------------------------------- */

#define JNI_PROTOCOL_PACKAGE            Java_com_teslameter_nr_teslameter_Protocol_

#define JNI_PROTOCOL_P(rettype, package, name)                                                      \
    extern "C" rettype package ## name

#define JNI_PROTOCOL_PP(rettype, package, name)                                                     \
    JNI_PROTOCOL_P(rettype, package, name)

#define JNI_PROTOCOL(rettype, name)                                                                 \
    JNI_PROTOCOL_PP(rettype, JNI_PROTOCOL_PACKAGE, name)

/* Compilation/runtime constants
 * -----------------------------
 * These constants should be part of initialization process and setup during the runtime, eg. when a
 * class is instanced.
 * */
#define PROTOCOL_UART_DEVICE            "/dev/ttyO4"

/*================================================================================  DATA TYPES  ==*/
/*==========================================================================  GLOBAL VARIABLES  ==*/
/*=======================================================================  FUNCTION PROTOTYPES  ==*/

JNI_PROTOCOL(jint, protocolOpen) (JNIEnv *env, jobject this_obj);

JNI_PROTOCOL(jint, protocolClose) (JNIEnv *env, jobject this_obj);

JNI_PROTOCOL(jint, protocolRdByte) (JNIEnv *env, jobject this_obj);

JNI_PROTOCOL(jint, protocolWrBuf) (JNIEnv *env, jobject this_obj, jintArray buf);

/*====================================================*//** @cond *//*==  CONFIGURATION ERRORS  ==*/
/** @endcond *//** @} *//**************************************************************************
 * END of cdi_manager.h
 **************************************************************************************************/
#endif /* TESLAMETER_UART_PROTOCOL_H */
