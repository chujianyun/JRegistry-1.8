/*
 * com_registry_RegistryKey.c    2012/09/15
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
#include "com_registry_RegistryKey.h"
#include "jlong.h"

static jmethodID regionMatches;
static jmethodID equalsIgnoreCase;
static jint      libversion = 0;

static wchar_t computer[MAX_COMPUTERNAME_LENGTH + 3];

typedef BOOL (WINAPI *ConvertSidToStringSidType) (PSID, LPWSTR*);

/*
 * Class:     com_registry_RegistryKey
 * Method:    initIDs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryKey_initIDs
  (JNIEnv *env, jclass cls) {
          if (libversion == 0) {
              jclass string = (*env)->FindClass(env, "java/lang/String");
              if (string != NULL) {
                  regionMatches = (*env)->GetMethodID(env, string, "regionMatches", "(ZILjava/lang/String;II)Z");
                  equalsIgnoreCase = (*env)->GetMethodID(env, string, "equalsIgnoreCase", "(Ljava/lang/String;)Z");
              }
              libversion = (jint) (MAKELONG((WORD) 1, (WORD) 0x108));
          }
          return libversion;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    getComputerName()
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_RegistryKey_getComputerName
  (JNIEnv *env, jclass cls) {
          computer[0] = L'\\';
          computer[1] = L'\\';
          
          DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
          BOOL val = GetComputerName(
              &computer[2],
              &nSize
          );
          
          if (val) {
              return (*env)->NewString(env, (jchar *) computer, nSize + 2);
          }
          else {
              if (GetLastError() == ERROR_BUFFER_OVERFLOW) {
                  wchar_t *comp = (wchar_t *) malloc((nSize + 2) * sizeof(wchar_t));
                  if (comp == NULL)
                      return NULL;
                  
                  comp[0] = L'\\';
                  comp[1] = L'\\';
                  
                  val = GetComputerName(
                      &comp[2],
                      &nSize
                  );
                  
                  if (val) {
                      jstring compname = (*env)->NewString(env, (jchar *) comp, nSize + 2);
                      free(comp);
                      
                      return compname;
                  }
                  else {
                      return NULL;
                  }
              }
              else {
                  return NULL;
              }
          }
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    formatErrorMessage
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_RegistryKey_formatErrorMessage
  (JNIEnv *env, jclass cls, jint lastError) {
          LPVOID buffer = NULL;
          DWORD len = FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER |
              FORMAT_MESSAGE_FROM_SYSTEM |
              FORMAT_MESSAGE_IGNORE_INSERTS,
              (LPCVOID) NULL,
              (DWORD) lastError,
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
              (LPWSTR) &buffer,
              0,
              (va_list *) NULL
          );
          
          if (GetLastError() == ERROR_SUCCESS) {
              jstring errmsg = (*env)->NewString(env, (jchar *) buffer, len);
              LocalFree(buffer);
              
              return errmsg;
          }
          else {
              return NULL;
          }
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    RegGetSetKeySecurity
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryKey_RegGetSetKeySecurity
  (JNIEnv *env, jobject obj, jlong src, jlong dest) {
          DWORD cbSecurityDescriptor = 0;
          
          LONG error = RegGetKeySecurity(
              (HKEY) jlong_to_ptr(src),
              DACL_SECURITY_INFORMATION,
              NULL,
              &cbSecurityDescriptor
          );
          
          if (error == ERROR_INSUFFICIENT_BUFFER) {
              PSECURITY_DESCRIPTOR pSecurityDescriptor = (PSECURITY_DESCRIPTOR) malloc(cbSecurityDescriptor);
              if (pSecurityDescriptor == NULL) {
                  return (jint) ERROR_OUTOFMEMORY;
              }
              
              error = RegGetKeySecurity(
                  (HKEY) jlong_to_ptr(src),
                  DACL_SECURITY_INFORMATION,
                  pSecurityDescriptor,
                  &cbSecurityDescriptor
              );
              
              if (error != ERROR_SUCCESS) {
                  free(pSecurityDescriptor);
                  return (jint) error;
              }
              
              error = RegSetKeySecurity(
                  (HKEY) jlong_to_ptr(dest),
                  DACL_SECURITY_INFORMATION,
                  pSecurityDescriptor
              );
              free(pSecurityDescriptor);
          }
          
          return (jint) error;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    getClassName
 * Signature: (JI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_RegistryKey_getClassName
  (JNIEnv *env, jobject obj, jlong hKey, jint lpcMaxClassLen) {
          jchar *lpClass = (jchar *) malloc(((DWORD) lpcMaxClassLen) * sizeof(jchar));
          if (lpClass == NULL) {
              return NULL;
          }
          
          LONG error = RegQueryInfoKey(
              (HKEY) jlong_to_ptr(hKey),
              lpClass,
              (LPDWORD) &lpcMaxClassLen,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL
          );
          
          if (error == ERROR_SUCCESS) {
              jstring classname = (*env)->NewString(env, lpClass, lpcMaxClassLen);
              free(lpClass);
              
              return classname;
          }
          
          return NULL;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    getCurrentUserSid
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_RegistryKey_getCurrentUserSid
  (JNIEnv *env, jclass cls) {
          jstring sid = NULL;
          
          HANDLE hToken;
          OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
          
          PTOKEN_USER tu = NULL;
          DWORD TokenInformationLength = 0;
          DWORD ReturnLength = 0;
          
          if (!GetTokenInformation(hToken, TokenUser, (LPVOID) tu, TokenInformationLength, &ReturnLength)) {
              if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                  tu = (PTOKEN_USER) malloc(ReturnLength);
                  TokenInformationLength = ReturnLength;
                  
                  GetTokenInformation(hToken, TokenUser, (LPVOID) tu, TokenInformationLength, &ReturnLength);
                  
                  HMODULE advapi32 = GetModuleHandle(L"advapi32.dll");
                  if (advapi32 != NULL) {
                      ConvertSidToStringSidType fn_ConvertSidToStringSid = (ConvertSidToStringSidType) GetProcAddress(advapi32, "ConvertSidToStringSidW");
                      if (fn_ConvertSidToStringSid != NULL) {
                          LPWSTR str;
                          fn_ConvertSidToStringSid((tu->User).Sid, &str);
                          
                          size_t length = wcslen(str) + 1;
                          jchar* result = (jchar*) malloc(length * sizeof(jchar));
                          if (result != NULL) {
                              result[0] = L'\0';
                              wcscpy(result, str);
                              
                              sid = (*env)->NewString(env, result, wcslen(result));
                              
                              LocalFree(str);
                              free(result);
                          }
                      }
                  }
                  
                  free(tu);
              }
          }
          
          CloseHandle(hToken);
          
          return sid;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    getSystemTime
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_com_registry_RegistryKey_getSystemTime
  (JNIEnv *env, jobject obj, jlong hKey) {
          FILETIME lpftLastWriteTime;
          SYSTEMTIME lpstLastWriteTime, lpstLocalTime;
          
          LONG error = RegQueryInfoKey(
              (HKEY) jlong_to_ptr(hKey),
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              &lpftLastWriteTime
          );
          
          if (error != ERROR_SUCCESS) {
              return NULL;
          }
          
          jint *temp = (jint *) malloc(8 * sizeof(jint));
          if (temp == NULL) {
              return NULL;
          }
          
          FileTimeToSystemTime(&lpftLastWriteTime, &lpstLastWriteTime);
          SystemTimeToTzSpecificLocalTime(
              NULL,
              &lpstLastWriteTime,
              &lpstLocalTime
          );
          
          temp[0] = (jint) lpstLocalTime.wYear;
          temp[1] = (jint) lpstLocalTime.wMonth;
          temp[2] = (jint) lpstLocalTime.wDayOfWeek;
          temp[3] = (jint) lpstLocalTime.wDay;
          temp[4] = (jint) lpstLocalTime.wHour;
          temp[5] = (jint) lpstLocalTime.wMinute;
          temp[6] = (jint) lpstLocalTime.wSecond;
          temp[7] = (jint) lpstLocalTime.wMilliseconds;
          
          jclass SystemTime = (*env)->FindClass(env, "com/registry/SystemTime");
          if (SystemTime == NULL) {
              free(temp);
              return NULL;
          }
          
          jmethodID cstruct_st = (*env)->GetMethodID(env, SystemTime, "<init>", "([I)V");
          if (cstruct_st == NULL) {
              free(temp);
              return NULL;
          }
          
          jintArray timeInfo = (*env)->NewIntArray(env, 8);
          if (timeInfo == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetIntArrayRegion(env, timeInfo, 0, 8, temp);
          free(temp);
          
          jobject SysTimeObj = (*env)->NewObject(env, SystemTime, cstruct_st, timeInfo);
          
          return SysTimeObj;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    lastModified
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_registry_RegistryKey_lastModified
  (JNIEnv *env, jobject obj, jlong hKey) {
          FILETIME lpftLastWriteTime;
          
          LONG error = RegQueryInfoKey(
              (HKEY) jlong_to_ptr(hKey),
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              NULL,
              &lpftLastWriteTime
          );
          
          if (error != ERROR_SUCCESS) {
              return (jlong) 0L;
          }
          
          jlong rv = 0;
          LARGE_INTEGER modTime;
          modTime.LowPart = (DWORD) lpftLastWriteTime.dwLowDateTime;
          modTime.HighPart = (LONG) lpftLastWriteTime.dwHighDateTime;
          rv = modTime.QuadPart / 10000;
          
          return rv;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    searchInteger
 * Signature: (JIJ[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryKey_searchInteger
  (JNIEnv *env, jobject obj, jlong hKey, jint index, jlong value, jobjectArray valueNames) {
          if (valueNames == NULL) {
              return (jint) -1;
          }
          
          jsize length = (*env)->GetArrayLength(env, valueNames);
          
          int i;
          for (i = index; i < (int) length; i++) {
              jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
              if (str == NULL) {
                  continue;
              }
              
              const jchar *lpValueName = (*env)->GetStringChars(env, str, NULL);
              DWORD lpType = 0, lpcbData = 0;
              LPBYTE lpData;
              
              LONG error = RegQueryValueEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpValueName,
                  NULL,
                  &lpType,
                  NULL,
                  &lpcbData
              );
              
              if (error != ERROR_SUCCESS) {
                  (*env)->ReleaseStringChars(env, str, lpValueName);
                  (*env)->DeleteLocalRef(env, str);
                  
                  continue;
              }
              
              switch (lpType) {
                     case REG_DWORD:
                     case REG_DWORD_BIG_ENDIAN:
                              if (lpcbData == 4) {
                                  lpData = (LPBYTE) malloc(lpcbData);
                                  if (lpData == NULL) {
                                      (*env)->ReleaseStringChars(env, str, lpValueName);
                                      break;
                                  }
                                  
                                  error = RegQueryValueEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      lpValueName,
                                      NULL,
                                      &lpType,
                                      lpData,
                                      &lpcbData
                                  );
                                  
                                  (*env)->ReleaseStringChars(env, str, lpValueName);
                                  
                                  if (error != ERROR_SUCCESS) {
                                      free(lpData);
                                      break;
                                  }
                                  
                                  DWORD a = 0, b = 0, c = 0, d = 0;
                                  
                                  // Integer: 0x12345678; a is lowest byte, d is highest byte
                                  if (lpType == REG_DWORD_LITTLE_ENDIAN) {
                                      // a = 0x12, b = 0x34, c = 0x56, d = 0x78
                                      a = lpData[0];
                                      b = lpData[1];
                                      c = lpData[2];
                                      d = lpData[3];
                                  }
                                  else {
                                      // a = 0x78, b = 0x56, c = 0x34, d = 0x12
                                      a = lpData[3];
                                      b = lpData[2];
                                      c = lpData[1];
                                      d = lpData[0];
                                  }
                                  
                                  // number: 0xdcba
                                  DWORD number = (a & 0x000000ff)
                                              | ((b << 8)  & 0x0000ff00)
                                              | ((c << 16) & 0x00ff0000)
                                              | ((d << 24) & 0xff000000);
                                  free(lpData);
                                  
                                  if (((DWORD) value) == number) {
                                      (*env)->DeleteLocalRef(env, str);
                                      (*env)->DeleteLocalRef(env, valueNames);
                                      
                                      return (jint) i;
                                  }
                              }
                              
                              break;
                     case REG_QWORD:
                              if (lpcbData == 8) {
                                  lpData = (LPBYTE) malloc(lpcbData);
                                  if (lpData == NULL) {
                                      (*env)->ReleaseStringChars(env, str, lpValueName);
                                      break;
                                  }
                                  
                                  error = RegQueryValueEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      lpValueName,
                                      NULL,
                                      &lpType,
                                      lpData,
                                      &lpcbData
                                  );
                                  
                                  (*env)->ReleaseStringChars(env, str, lpValueName);
                                  
                                  if (error != ERROR_SUCCESS) {
                                      free(lpData);
                                      break;
                                  }
                                  
                                  DWORD data1 = (lpData[0] & 0x000000ff)
                                             | ((lpData[1] << 8)  & 0x0000ff00)
                                             | ((lpData[2] << 16) & 0x00ff0000)
                                             | ((lpData[3] << 24) & 0xff000000);
                                  
                                  DWORD data2 = (lpData[4] & 0x000000ff)
                                             | ((lpData[5] << 8)  & 0x0000ff00)
                                             | ((lpData[6] << 16) & 0x00ff0000)
                                             | ((lpData[7] << 24) & 0xff000000);
                                  
                                  jlong number = (jlong) data1;
                                        number = (jlong) ((number << 32) | data2);
                                  free(lpData);
                                  
                                  if (value == number) {
                                      (*env)->DeleteLocalRef(env, str);
                                      (*env)->DeleteLocalRef(env, valueNames);
                                      
                                      return (jint) i;
                                  }
                              }
                              
                     default: break;
              }
              
              (*env)->DeleteLocalRef(env, str);
          }
          
          (*env)->DeleteLocalRef(env, valueNames);
          
          return (jint) -1;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    searchString
 * Signature: (JIZZZLjava/lang/String;[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryKey_searchString
  (JNIEnv *env, jobject obj, jlong hKey, jint index, jboolean values, jboolean data, jboolean matchWholeStr,
   jstring key, jobjectArray valueNames) {
          if (valueNames == NULL) {
              return (jint) -1;
          }
          
          jsize length = (*env)->GetArrayLength(env, valueNames);
          
          int i;
          for (i = index; i < (int) length; i++) {
              jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
              if (str == NULL) {
                  continue;
              }
              
              if (values == JNI_TRUE) {
                  jboolean result = JNI_FALSE;
                  
                  if (matchWholeStr == JNI_TRUE) {
                      result = (*env)->CallBooleanMethod(env, str, equalsIgnoreCase, key);
                  }
                  else {
                      jsize strlen = (*env)->GetStringLength(env, str);
                      jsize ktrlen = (*env)->GetStringLength(env, key);
                      
                      int j;
                      for (j = 0; j < (int) strlen; j++) {
                          result = (*env)->CallBooleanMethod(env, str, regionMatches, JNI_TRUE, j, key, 0, ktrlen);
                          if (result == JNI_TRUE) {
                              break;
                          }
                      }
                  }
                  
                  if (result == JNI_TRUE) {
                      (*env)->DeleteLocalRef(env, str);
                      (*env)->DeleteLocalRef(env, valueNames);
                      
                      return (jint) i;
                  }
              }
              
              if (data == JNI_TRUE) {
                  const jchar *lpValueName = (*env)->GetStringChars(env, str, NULL);
                  DWORD lpType = 0, lpcbData = 0, length = 0;
                  jboolean result = JNI_FALSE;
                  jchar *buf;
                  
                  LONG error = RegQueryValueEx(
                      (HKEY) jlong_to_ptr(hKey),
                      lpValueName,
                      NULL,
                      &lpType,
                      NULL,
                      &lpcbData
                  );
                  
                  if (error != ERROR_SUCCESS) {
                      (*env)->ReleaseStringChars(env, str, lpValueName);
                      (*env)->DeleteLocalRef(env, str);
                      
                      continue;
                  }
                  
                  switch (lpType) {
                         case REG_SZ:
                         case REG_EXPAND_SZ:
                                  buf = (jchar *) malloc(lpcbData);
                                  if (buf == NULL) {
                                      (*env)->ReleaseStringChars(env, str, lpValueName);
                                      break;
                                  }
                                  
                                  error = RegQueryValueEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      lpValueName,
                                      NULL,
                                      &lpType,
                                      (LPBYTE) buf,
                                      &lpcbData
                                  );
                                  
                                  (*env)->ReleaseStringChars(env, str, lpValueName);
                                  
                                  if (error != ERROR_SUCCESS) {
                                      free(buf);
                                      break;
                                  }
                                  
                                  length = (lpcbData / sizeof(jchar));
                                  if (length != 0 && buf[length - 1] == L'\0') {
                                      length--;
                                  }
                                  
                                  jstring tstr = (*env)->NewString(env, buf, length);
                                  free(buf);
                                  if (tstr == NULL) {
                                      break;
                                  }
                                  
                                  if (matchWholeStr == JNI_TRUE) {
                                      result = (*env)->CallBooleanMethod(env, tstr, equalsIgnoreCase, key);
                                  }
                                  else {
                                      jsize strlen = (*env)->GetStringLength(env, tstr);
                                      jsize ktrlen = (*env)->GetStringLength(env, key);
                                      
                                      int j;
                                      for (j = 0; j < (int) strlen; j++) {
                                          result = (*env)->CallBooleanMethod(env, tstr, regionMatches,
                                              JNI_TRUE, j, key, 0, ktrlen);
                                          if (result == JNI_TRUE) {
                                              break;
                                          }
                                      }
                                  }
                                  
                                  (*env)->DeleteLocalRef(env, tstr);
                                  
                                  if (result == JNI_TRUE) {
                                      (*env)->DeleteLocalRef(env, str);
                                      (*env)->DeleteLocalRef(env, valueNames);
                                      
                                      return (jint) i;
                                  }
                                  
                                  break;
                         case REG_MULTI_SZ:
                                  if (matchWholeStr == JNI_TRUE) {
                                      break;
                                  }
                                  
                                  buf = (jchar *) malloc(lpcbData);
                                  if (buf == NULL) {
                                      (*env)->ReleaseStringChars(env, str, lpValueName);
                                      break;
                                  }
                                  
                                  error = RegQueryValueEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      lpValueName,
                                      NULL,
                                      &lpType,
                                      (LPBYTE) buf,
                                      &lpcbData
                                  );
                                  
                                  (*env)->ReleaseStringChars(env, str, lpValueName);
                                  
                                  if (error != ERROR_SUCCESS) {
                                      free(buf);
                                      break;
                                  }
                                  
                                  unsigned int j, start = 0, lines = 0, count = 0;
                                  for (j = 0; j < lpcbData - 1; j++) {
                                      if (buf[j] == '\0') {
                                          lines++;
                                          if (buf[j + 1] == '\0') {
                                              break;
                                          }
                                      }
                                  }
                                  
                                  j = 0;
                                  while (j < lpcbData - 1 && count < lines) {
                                        if (buf[j++] == '\0') {
                                            int len = j - start - 1;
                                            jchar *ch = (jchar *) malloc(len * sizeof(jchar));
                                            if (ch == NULL) {
                                                break;
                                            }
                                            
                                            int k;
                                            for (k = 0; k < len; k++) {
                                                ch[k] = (jchar) buf[k + start];
                                            }
                                            
                                            jstring tstr = (*env)->NewString(env, ch, len);
                                            free(ch);
                                            if (tstr == NULL) {
                                                break;
                                            }
                                            
                                            jsize strlen = (*env)->GetStringLength(env, tstr);
                                            jsize ktrlen = (*env)->GetStringLength(env, key);
                                            
                                            int q;
                                            for (q = 0; q < (int) strlen; q++) {
                                                result = (*env)->CallBooleanMethod(env, tstr, regionMatches,
                                                    JNI_TRUE, q, key, 0, ktrlen);
                                                if (result == JNI_TRUE) {
                                                    break;
                                                }
                                            }
                                            
                                            (*env)->DeleteLocalRef(env, tstr);
                                            
                                            if (result == JNI_TRUE) {
                                                (*env)->DeleteLocalRef(env, str);
                                                (*env)->DeleteLocalRef(env, valueNames);
                                                free(buf);
                                                
                                                return (jint) i;
                                            }
                                            
                                            start = j;
                                            count++;
                                            if (buf[j] == '\0') {
                                                break;
                                            }
                                        }
                                  }
                                  
                                  free(buf);
                         default: break;
                  }
              }
              
              (*env)->DeleteLocalRef(env, str);
          }
          
          (*env)->DeleteLocalRef(env, valueNames);
          
          return (jint) -1;
  }

/*
 * Class:     com_registry_RegistryKey
 * Method:    searchBinary
 * Signature: (JI[BLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_RegistryKey_searchBinary
  (JNIEnv *env, jobject obj, jlong hKey, jint index, jbyteArray value, jobjectArray valueNames) {
          if (valueNames == NULL) {
              return (jint) -1;
          }
          
          jbyte *bArray = (*env)->GetByteArrayElements(env, value, NULL);
          if (bArray == NULL) {
              return (jint) -1;
          }
          jsize bLength = (*env)->GetArrayLength(env, value);
          
          jsize length = (*env)->GetArrayLength(env, valueNames);
          
          int i;
          for (i = index; i < (int) length; i++) {
              jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
              if (str == NULL) {
                  continue;
              }
              
              const jchar *lpValueName = (*env)->GetStringChars(env, str, NULL);
              DWORD lpType = 0, lpcbData = 0;
              LPBYTE lpData;
              
              LONG error = RegQueryValueEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpValueName,
                  NULL,
                  &lpType,
                  NULL,
                  &lpcbData
              );
              
              if (error != ERROR_SUCCESS) {
                  (*env)->ReleaseStringChars(env, str, lpValueName);
                  (*env)->DeleteLocalRef(env, str);
                  
                  continue;
              }
              
              if (lpcbData >= (DWORD) bLength) {
                  lpData = (LPBYTE) malloc(lpcbData);
                  if (lpData == NULL) {
                      (*env)->ReleaseStringChars(env, str, lpValueName);
                      continue;
                  }
                  
                  error = RegQueryValueEx(
                      (HKEY) jlong_to_ptr(hKey),
                      lpValueName,
                      NULL,
                      &lpType,
                      lpData,
                      &lpcbData
                  );
                  
                  (*env)->ReleaseStringChars(env, str, lpValueName);
                  
                  if (error != ERROR_SUCCESS) {
                      free(lpData);
                      continue;
                  }
                  
                  unsigned int j, k;
                  for (j = 0; j < lpcbData; j++) {
                      if (((DWORD) (lpcbData - j)) >= (DWORD) bLength) {
                          BOOL match = TRUE;
                          for (k = 0; k < bLength; k++) {
                              if (lpData[j + k] != bArray[k]) {
                                  match = FALSE;
                                  break;
                              }
                          }
                          
                          if (match == TRUE) {
                              (*env)->DeleteLocalRef(env, str);
                              (*env)->DeleteLocalRef(env, valueNames);
                              (*env)->ReleaseByteArrayElements(env, value, bArray, 0);
                              free(lpData);
                              
                              return (jint) i;
                          }
                      }
                  }
              }
              
              free(lpData);
              (*env)->DeleteLocalRef(env, str);
          }
          
          (*env)->DeleteLocalRef(env, valueNames);
          (*env)->ReleaseByteArrayElements(env, value, bArray, 0);
          
          return (jint) -1;
  }
