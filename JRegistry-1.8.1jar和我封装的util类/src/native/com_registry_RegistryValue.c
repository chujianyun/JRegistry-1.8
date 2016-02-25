/*
 * com_registry_RegistryValue.c    2011/08/07
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
#include "com_registry_RegistryValue.h"
#include "jlong.h"

/*
 * Class:     com_registry_RegistryValue
 * Method:    getByteData
 * Signature: (JLjava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_registry_RegistryValue_getByteData
  (JNIEnv *env, jobject obj, jlong hKey, jstring value) {
          const jchar *lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          
          DWORD lpcbData = 0;
          jbyte *lpData;
          
          LONG error = RegQueryValueEx(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName,
              NULL,
              NULL,
              NULL,
              &lpcbData
          );
          
          if (error != ERROR_SUCCESS) {
              if (value != NULL) {
                  (*env)->ReleaseStringChars(env, value, lpValueName);
              }
              return NULL;
          }
          
          lpData = (jbyte *) malloc(lpcbData);
          if (lpData == NULL) {
              if (value != NULL) {
                  (*env)->ReleaseStringChars(env, value, lpValueName);
              }
              return NULL;
          }
          
          error = RegQueryValueEx(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName,
              NULL,
              NULL,
              (LPBYTE) lpData,
              &lpcbData
          );
          
          if (value != NULL) {
              (*env)->ReleaseStringChars(env, value, lpValueName);
          }
          
          if (error != ERROR_SUCCESS) {
              free(lpData);
              return NULL;
          }
          
          DWORD length = (lpcbData / sizeof(jbyte));
          jbyteArray result = (*env)->NewByteArray(env, length);
          if (result != NULL) {
              (*env)->SetByteArrayRegion(env, result, 0, length, lpData);
              free(lpData);
              
              return result;
          }
          else {
              free(lpData);
          }
          
          return NULL;
  }

/*
 * Class:     com_registry_RegistryValue
 * Method:    setByteData
 * Signature: (JLjava/lang/String;I[B)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryValue_setByteData
  (JNIEnv *env, jobject obj, jlong hKey, jstring value, jint lpType, jbyteArray array) {
          if (array == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar* lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          
          jbyte *lpData = (*env)->GetByteArrayElements(env, array, JNI_FALSE);
          DWORD lpcbData = ((*env)->GetArrayLength(env, array) * sizeof(jbyte));
          
          LONG error = RegSetValueEx(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName,
              0,
              lpType,
              (LPBYTE) lpData,
              lpcbData
          );
          
          if (value) {
              (*env)->ReleaseStringChars(env, value, lpValueName);
          }
          (*env)->ReleaseByteArrayElements(env, array, lpData, 0);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_RegistryValue
 * Method:    getByteLength
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryValue_getByteLength
  (JNIEnv *env, jobject obj, jlong hKey, jstring value) {
          const jchar *lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          
          DWORD lpcbData = 0;
          jbyte *lpData;
          
          LONG error = RegQueryValueEx(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName,
              NULL,
              NULL,
              NULL,
              &lpcbData
          );
          
          if (value != NULL) {
              (*env)->ReleaseStringChars(env, value, lpValueName);
          }
          
          if (error != ERROR_SUCCESS) {
              return (jint) -1;
          }
          else {
              return (jint) lpcbData;
          }
  }
