/*
 * com_registry_WatchData.c    2011/08/07
 * Copyright (C) 2011  Yinon Michaeli
 *
 * This file is part of JRegistry.
 *
 * JRegistry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact by e-mail if you discover any bugs or if you have a suggestion
 * to myinon2005@hotmail.com
 */

#include <windows.h>
#include "com_registry_WatchData.h"
#include "jlong.h"

/*
 * Class:     com_registry_WatchData
 * Method:    CreateEvent
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_registry_WatchData_CreateEvent
  (JNIEnv *env, jclass cls) {
          return ptr_to_jlong(CreateEvent(NULL, TRUE, FALSE, NULL));
  }

/*
 * Class:     com_registry_WatchData
 * Method:    WaitForSingleObject
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WatchData_WaitForSingleObject
  (JNIEnv *env, jclass cls, jlong hHandle, jint dwMilliseconds) {
          return WaitForSingleObject(
              (HANDLE) jlong_to_ptr(hHandle), (DWORD) dwMilliseconds);
  }

/*
 * Class:     com_registry_WatchData
 * Method:    CloseHandle
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_registry_WatchData_CloseHandle
  (JNIEnv *env, jclass cls, jlong hHandle) {
          CloseHandle((HANDLE) jlong_to_ptr(hHandle));
  }
