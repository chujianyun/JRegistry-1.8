/*
 * com_registry_WinRegistryAPI.c    2012/09/15
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
#include "com_registry_WinRegistryAPI.h"
#include "jlong.h"

typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING {
    WORD Length;
    WORD MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemRegistryQuotaInformation = 37
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef NTSTATUS (NTAPI *NtDeleteKeyType) (HANDLE);
typedef NTSTATUS (NTAPI *NtQuerySystemInformationType) (SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI *NtRenameKeyType) (HANDLE, PUNICODE_STRING);
typedef ULONG (NTAPI *RtlNtStatusToDosErrorType) (NTSTATUS);

typedef BOOL (WINAPI *GetSystemRegistryQuotaType) (PDWORD, PDWORD);
typedef LONG (WINAPI *RegCopyTreeType) (HKEY, LPCWSTR, HKEY); // No links in tree or fail
typedef LONG (WINAPI *RegDeleteKeyExType) (HKEY, LPCWSTR, REGSAM, DWORD);
typedef LONG (WINAPI *RegDeleteKeyValueType) (HKEY, LPCWSTR, LPCWSTR);
typedef LONG (WINAPI *RegDeleteTreeType) (HKEY, LPCWSTR); // No links in tree or fail
typedef LONG (WINAPI *RegDisableReflectionKeyType) (HKEY);
typedef LONG (WINAPI *RegEnableReflectionKeyType) (HKEY);
typedef LONG (WINAPI *RegQueryReflectionKeyType) (HKEY, BOOL *);
typedef LONG (WINAPI *RegSaveKeyExType) (HKEY, LPCWSTR, LPSECURITY_ATTRIBUTES, DWORD);

BOOL RegIsHKeyRemote(HKEY hKey) {
	DWORD_PTR dw = (DWORD_PTR) hKey;
	
	if ((hKey >= HKEY_CLASSES_ROOT) && (hKey <= HKEY_DYN_DATA)) {
		return FALSE; // local
	}
	
	return (((~dw) & 1) == 0);
}

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    ExpandEnvironmentStrings
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_WinRegistryAPI_ExpandEnvironmentStrings
  (JNIEnv *env, jclass cls, jstring str) {
          if (str == NULL) {
              return NULL;
          }
          
          const jchar* lpSrc = (*env)->GetStringChars(env, str, NULL);
          
          jchar *lpDst = NULL;
          DWORD  nSize = 0;
          
          DWORD len = ExpandEnvironmentStrings(lpSrc, lpDst, nSize);
          if (len != 0 && GetLastError() == ERROR_SUCCESS) {
              lpDst = (jchar *) malloc((len + 1) * sizeof(jchar));
              nSize = len;
              
              if (lpDst != NULL) {
                  len = ExpandEnvironmentStrings(
                      lpSrc,
                      lpDst,
                      nSize + 1
                  );
                  
                  jstring result = (*env)->NewString(env, lpDst, nSize);
                  free(lpDst);
                  
                  (*env)->ReleaseStringChars(env, str, lpSrc);
                  
                  return result;
              }
          }
          
          return NULL;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    GetSystemRegistryQuota
 * Signature: ()Lcom/registry/RegistryQuota;
 */
JNIEXPORT jobject JNICALL Java_com_registry_WinRegistryAPI_GetSystemRegistryQuota
  (JNIEnv *env, jclass cls) {
          HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
          if (kernel32 != NULL) {
              GetSystemRegistryQuotaType fn_GetSystemRegistryQuota =
                  (GetSystemRegistryQuotaType) GetProcAddress(kernel32, "GetSystemRegistryQuota");
              if (fn_GetSystemRegistryQuota != NULL) {
                  DWORD pdwQuotaAllowed;
                  DWORD pdwQuotaUsed;
                  BOOL  fRetVal;
                  
                  fRetVal = fn_GetSystemRegistryQuota(&pdwQuotaAllowed, &pdwQuotaUsed);
                  if (fRetVal == FALSE)
                      return NULL;
                  
                  jclass RegistryQuota = (*env)->FindClass(env, "com/registry/RegistryQuota");
                  if (RegistryQuota == NULL)
                      return NULL;
                  
                  jmethodID cstruct_rq = (*env)->GetMethodID(env, RegistryQuota, "<init>", "(II)V");
                  if (cstruct_rq == NULL)
                      return NULL;
                  
                  jobject RegQuotaObj = (*env)->NewObject(env, RegistryQuota, cstruct_rq, pdwQuotaAllowed, pdwQuotaUsed);
                  
                  return RegQuotaObj;
              }
          }
          
          HMODULE nt = GetModuleHandle(L"ntdll.dll");
          if (nt != NULL) {
              NtQuerySystemInformationType fn_NtQuerySystemInformation =
                  (NtQuerySystemInformationType) GetProcAddress(nt, "NtQuerySystemInformation");
              if (fn_NtQuerySystemInformation != NULL) {
                  SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;
                  ULONG uReturnLength;
                  
                  NTSTATUS status = fn_NtQuerySystemInformation(
                      SystemRegistryQuotaInformation,
                      &srqi,
                      sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION),
                      &uReturnLength);
                  
                  RtlNtStatusToDosErrorType fn_RtlNtStatusToDosError =
                      (RtlNtStatusToDosErrorType) GetProcAddress(nt,"RtlNtStatusToDosError");
                  if (fn_RtlNtStatusToDosError != NULL) {
                      ULONG error = fn_RtlNtStatusToDosError(status);
                      if (error == ERROR_SUCCESS) {
                          jclass RegistryQuota = (*env)->FindClass(env, "com/registry/RegistryQuota");
                          if (RegistryQuota == NULL)
                              return NULL;
                          
                          jmethodID cstruct_rq = (*env)->GetMethodID(env, RegistryQuota, "<init>", "(II)V");
                          if (cstruct_rq == NULL)
                              return NULL;
                          
                          jobject RegQuotaObj = (*env)->NewObject(
                              env,
                              RegistryQuota,
                              cstruct_rq,
                              srqi.RegistryQuotaAllowed,
                              srqi.RegistryQuotaUsed);
                          
                          return RegQuotaObj;
                      }
                  }
              }
          }
          
          return NULL;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegCloseKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegCloseKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          return (jint) RegCloseKey((HKEY) jlong_to_ptr(hKey));
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegConnectRegistry
 * Signature: (Ljava/lang/String;J)[J
 */
JNIEXPORT jlongArray JNICALL Java_com_registry_WinRegistryAPI_RegConnectRegistry
  (JNIEnv *env, jclass cls, jstring str, jlong hKey) {
          jlongArray result;
          HKEY phkResult;
          
          const jchar *lpMachineName = (str != NULL ? (*env)->GetStringChars(env, str, NULL) : NULL);
          
          LONG error = RegConnectRegistry(
              lpMachineName,
              (HKEY) jlong_to_ptr(hKey),
              &phkResult
          );
          
          if (str != NULL) {
              (*env)->ReleaseStringChars(env, str, lpMachineName);
          }
          
          jlong *temp = (jlong *) malloc(2 * sizeof(jlong));
          if (temp == NULL) {
              return NULL;
          }
          
          temp[0] = (jlong) error;
          temp[1] = ptr_to_jlong(phkResult);
          
          result = (*env)->NewLongArray(env, 2);
          if (result == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetLongArrayRegion(env, result, 0, 2, temp);
          free(temp);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegCopyTree
 * Signature: (JLjava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegCopyTree
  (JNIEnv *env, jclass cls, jlong hKey, jstring str, jlong hKey2) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          const jchar *lpSubKey = (str != NULL ? (*env)->GetStringChars(env, str, NULL) : NULL);
          
          RegCopyTreeType fn_RegCopyTree =
              (RegCopyTreeType) GetProcAddress(hModule, "RegCopyTreeW");
          if (fn_RegCopyTree != NULL) {
              error = (jint) fn_RegCopyTree(
                  (HKEY) jlong_to_ptr(hKey),
                  lpSubKey,
                  (HKEY) jlong_to_ptr(hKey2)
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          if (str != NULL)
              (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegCreateKeyEx
 * Signature: (JLjava/lang/String;IILjava/lang/String;)[J
 */
JNIEXPORT jlongArray JNICALL Java_com_registry_WinRegistryAPI_RegCreateKeyEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring str, jint dwOptions, jint samDesired, jstring clazz) {
          if (str == NULL) {
              return NULL;
          }
          
          jlongArray result;
          HKEY phkResult;
          DWORD lpdwDisposition = 0;
          
          const jchar *lpSubKey = (*env)->GetStringChars(env, str, NULL);
          const jchar *lpClass = (clazz != NULL ? (*env)->GetStringChars(env, clazz, NULL) : NULL);
          
          LONG error = RegCreateKeyEx(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey,
              0,
              (LPWSTR) lpClass,
              dwOptions,
              samDesired,
              NULL,
              &phkResult,
              &lpdwDisposition
          );
          
          if (clazz != NULL) {
              (*env)->ReleaseStringChars(env, clazz, lpClass);
          }
          (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          jlong *temp = (jlong *) malloc(3 * sizeof(jlong));
          if (temp == NULL) {
              return NULL;
          }
          
          temp[0] = (jlong) error;
          temp[1] = ptr_to_jlong(phkResult);
          temp[2] = (jlong) lpdwDisposition;
          
          result = (*env)->NewLongArray(env, 3);
          if (result == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetLongArrayRegion(env, result, 0, 3, temp);
          free(temp);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteKey
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring str) {
          if (str == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpSubKey = (*env)->GetStringChars(env, str, NULL);
          
          LONG error = RegDeleteKey(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey
          );
          
          (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteKeyEx
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteKeyEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring str, jint sam) {
          if (str == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          const jchar *lpSubKey = (*env)->GetStringChars(env, str, NULL);
          
          RegDeleteKeyExType fn_RegDeleteKeyEx =
              (RegDeleteKeyExType) GetProcAddress(hModule, "RegDeleteKeyExW");
          if (fn_RegDeleteKeyEx != NULL) {
              error = (jint) fn_RegDeleteKeyEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpSubKey,
                  sam,
                  0
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteKeyValue
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteKeyValue
  (JNIEnv *env, jclass cls, jlong hKey, jstring str, jstring value) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          const jchar *lpSubKey = (str != NULL ? (*env)->GetStringChars(env, str, NULL) : NULL);
          const jchar *lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          
          RegDeleteKeyValueType fn_RegDeleteKeyValue =
              (RegDeleteKeyValueType) GetProcAddress(hModule, "RegDeleteKeyValueW");
          if (fn_RegDeleteKeyValue != NULL) {
              error = (jint) fn_RegDeleteKeyValue(
                  (HKEY) jlong_to_ptr(hKey),
                  lpSubKey,
                  lpValueName
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          (*env)->ReleaseStringChars(env, str, lpSubKey);
          (*env)->ReleaseStringChars(env, value, lpValueName);
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteLink
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteLink
  (JNIEnv *env, jclass cls, jlong hKey) {
          HMODULE hModule = GetModuleHandle(L"ntdll.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          error = (jint) RegDeleteValue(
              (HKEY) jlong_to_ptr(hKey),
              L"SymbolicLinkValue"
          );
          
          if (error != ERROR_SUCCESS && error != ERROR_FILE_NOT_FOUND) {
              return error;
          }
          
          NtDeleteKeyType fn_NtDeleteKey =
              (NtDeleteKeyType) GetProcAddress(hModule, "NtDeleteKey");
          if (fn_NtDeleteKey != NULL) {
              NTSTATUS status = fn_NtDeleteKey((HANDLE) jlong_to_ptr(hKey));
              
              RtlNtStatusToDosErrorType fn_RtlNtStatusToDosError =
                  (RtlNtStatusToDosErrorType) GetProcAddress(hModule,"RtlNtStatusToDosError");
              if (fn_RtlNtStatusToDosError != NULL) {
                  error = (jint) fn_RtlNtStatusToDosError(status);
              }
              else {
                  error = (jint) GetLastError();
              }
          }
          else {
              error = (jint) GetLastError();
          }
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteTree
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteTree
  (JNIEnv *env, jclass cls, jlong hKey, jstring str) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          const jchar *lpSubKey = (str != NULL ? (*env)->GetStringChars(env, str, NULL) : NULL);
          
          RegDeleteTreeType fn_RegDeleteTree =
              (RegDeleteTreeType) GetProcAddress(hModule, "RegDeleteTreeW");
          if (fn_RegDeleteTree != NULL) {
              error = (jint) fn_RegDeleteTree(
                  (HKEY) jlong_to_ptr(hKey),
                  lpSubKey
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          if (str != NULL)
              (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDeleteValue
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDeleteValue
  (JNIEnv *env, jclass cls, jlong hKey, jstring str) {
          if (str == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpValueName = (*env)->GetStringChars(env, str, NULL);
          
          LONG error = RegDeleteValue(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName
          );
          
          (*env)->ReleaseStringChars(env, str, lpValueName);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegDisableReflectionKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegDisableReflectionKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          RegDisableReflectionKeyType fn_RegDisableReflectionKey =
              (RegDisableReflectionKeyType) GetProcAddress(hModule, "RegDisableReflectionKey");
          if (fn_RegDisableReflectionKey != NULL) {
              error = (jint) fn_RegDisableReflectionKey(
                  (HKEY) jlong_to_ptr(hKey)
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegEnableReflectionKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegEnableReflectionKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          RegEnableReflectionKeyType fn_RegEnableReflectionKey =
              (RegEnableReflectionKeyType) GetProcAddress(hModule, "RegEnableReflectionKey");
          if (fn_RegEnableReflectionKey != NULL) {
              error = (jint) fn_RegEnableReflectionKey(
                  (HKEY) jlong_to_ptr(hKey)
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegEnumKeyEx
 * Signature: (JII)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_WinRegistryAPI_RegEnumKeyEx
  (JNIEnv *env, jclass cls, jlong hKey, jint dwIndex, jint len) {
          jstring result;
          DWORD lpcName = (DWORD) len;
          
          jchar *lpName = (jchar *) malloc(lpcName * sizeof(jchar));
          if (lpName == NULL) {
              return NULL;
          }
          
          LONG error = RegEnumKeyEx(
              (HKEY) jlong_to_ptr(hKey),
              dwIndex,
              lpName,
              &lpcName,
              NULL,
              NULL,
              NULL,
              NULL
          );
          
          if (error != ERROR_SUCCESS) {
              free(lpName);
              return NULL;
          }
          
          result = (*env)->NewString(env, lpName, lpcName);
          free(lpName);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegEnumValue
 * Signature: (JII)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_WinRegistryAPI_RegEnumValue
  (JNIEnv *env, jclass cls, jlong hKey, jint dwIndex, jint len) {
          jstring result;
          DWORD lpcchValueName = (DWORD) len;
          
          jchar *lpValueName = (jchar *) malloc(lpcchValueName * sizeof(jchar));
          if (lpValueName == NULL) {
              return NULL;
          }
          
          LONG error = RegEnumValue(
              (HKEY) jlong_to_ptr(hKey),
              dwIndex,
              lpValueName,
              &lpcchValueName,
              NULL,
              NULL,
              NULL,
              NULL
          );
          
          if (error != ERROR_SUCCESS) {
              free(lpValueName);
              return NULL;
          }
          
          result = (*env)->NewString(env, lpValueName, lpcchValueName);
          free(lpValueName);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegFlushKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegFlushKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          return (jint) RegFlushKey((HKEY) jlong_to_ptr(hKey));
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegGetLinkLocation
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_registry_WinRegistryAPI_RegGetLinkLocation
  (JNIEnv *env, jclass cls, jlong hKey) {
          DWORD lpType = 0, lpcbData = 0;
          jchar *lpData;
          
          LONG error = RegQueryValueEx(
              (HKEY) jlong_to_ptr(hKey),
              L"SymbolicLinkValue",
              NULL,
              &lpType,
              NULL,
              &lpcbData
          );
          
          if (error != ERROR_SUCCESS) {
              return NULL;
          }
          
          if (lpType == REG_LINK) {
              lpData = (jchar *) malloc(lpcbData);
              if (lpData == NULL) {
                  return NULL;
              }
              
              error = RegQueryValueEx(
                  (HKEY) jlong_to_ptr(hKey),
                  L"SymbolicLinkValue",
                  NULL,
                  &lpType,
                  (LPBYTE) lpData,
                  &lpcbData
              );
              
              if (error != ERROR_SUCCESS) {
                  free(lpData);
                  return NULL;
              }
              
              DWORD length = (lpcbData / sizeof(jchar));
              if (lpData[length - 1] == L'\0') {
                  length--;
              }
              
              jstring result = (*env)->NewString(env, lpData, length);
              free(lpData);
              
              return result;
          }
          
          return NULL;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegIsLinkKey
 * Signature: (J)[I
 */
JNIEXPORT jintArray JNICALL Java_com_registry_WinRegistryAPI_RegIsLinkKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          DWORD lpcSubKeys = 0, lpcValues = 0, isLink = 0;
          
          LONG error = RegQueryInfoKey(
              (HKEY) jlong_to_ptr(hKey),
              NULL,
              NULL,
              NULL,
              &lpcSubKeys,
              NULL,
              NULL,
              &lpcValues,
              NULL,
              NULL,
              NULL,
              NULL
          );
          
          if (error == ERROR_SUCCESS) {
              HKEY phkResult;
              
              if (lpcSubKeys == 0 && lpcValues == 1) {
                  DWORD lpType = 0, lpcbData = 0;
                  
                  error = RegQueryValueEx(
                      (HKEY) jlong_to_ptr(hKey),
                      L"SymbolicLinkValue",
                      NULL,
                      &lpType,
                      NULL,
                      &lpcbData
                  );
                  
                  if (error == ERROR_SUCCESS || lpType == REG_LINK) {
                      LPBYTE lpData = (LPBYTE) malloc(lpcbData);
                      if (lpData != NULL) {
                          error = RegQueryValueEx(
                              (HKEY) jlong_to_ptr(hKey),
                              L"SymbolicLinkValue",
                              NULL,
                              &lpType,
                              lpData,
                              &lpcbData
                          );
                          
                          if (error == ERROR_SUCCESS) {
                              error = RegDeleteValue(
                                  (HKEY) jlong_to_ptr(hKey),
                                  L"SymbolicLinkValue"
                              );
                              
                              if (error == ERROR_SUCCESS) {
                                  error = RegOpenKeyEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      L"",
                                      0,
                                      KEY_READ,
                                      &phkResult
                                  );
                                  
                                  isLink = (error != ERROR_SUCCESS ? 1 : 0);
                                  RegCloseKey(phkResult);
                                  
                                  error = RegSetValueEx(
                                      (HKEY) jlong_to_ptr(hKey),
                                      L"SymbolicLinkValue",
                                      0,
                                      lpType,
                                      lpData,
                                      lpcbData
                                  );
                              }
                          }
                          
                          free(lpData);
                      }
                  }
              }
              else if (lpcSubKeys == 0 && lpcValues == 0) {
                  error = RegOpenKeyEx(
                      (HKEY) jlong_to_ptr(hKey),
                      L"",
                      0,
                      KEY_READ,
                      &phkResult
                  );
                  
                  isLink = (error != ERROR_SUCCESS ? 1 : 0);
                  RegCloseKey(phkResult);
              }
              else {
                  isLink = 0;
              }
          }
          
          
          jint *temp = (jint *) malloc(2 * sizeof(jint));
          if (temp == NULL) {
              return NULL;
          }
          
          temp[0] = (jint) error;
          temp[1] = (jint) isLink;
          
          jintArray result = (*env)->NewIntArray(env, 2);
          if (result == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetIntArrayRegion(env, result, 0, 2, temp);
          free(temp);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegLoadKey
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegLoadKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring subKey, jstring file) {
          if (file == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpSubKey = (subKey != NULL ? (*env)->GetStringChars(env, subKey, NULL) : NULL);
          const jchar *lpFile = (*env)->GetStringChars(env, file, NULL);
          
          LONG error = RegLoadKey(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey,
              lpFile
          );
          
          if (subKey != NULL) {
              (*env)->ReleaseStringChars(env, subKey, lpSubKey);
          }
          (*env)->ReleaseStringChars(env, file, lpFile);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegNotifyChangeKeyValue
 * Signature: (JZIJZ)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegNotifyChangeKeyValue
  (JNIEnv *env, jclass cls, jlong hKey, jboolean bWatchSubtree, jint dwNotifyFilter, jlong hEvent, jboolean fAsynchronous) {
          return (jint) RegNotifyChangeKeyValue(
              (HKEY) jlong_to_ptr(hKey),
              bWatchSubtree,
              dwNotifyFilter,
              (HANDLE) jlong_to_ptr(hEvent),
              fAsynchronous
          );
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegOpenKeyEx
 * Signature: (JLjava/lang/String;I)[J
 */
JNIEXPORT jlongArray JNICALL Java_com_registry_WinRegistryAPI_RegOpenKeyEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring str, jint samDesired) {
          if (str == NULL) {
              return NULL;
          }
          
          jlongArray result;
          HKEY phkResult;
          
          const jchar *lpSubKey = (*env)->GetStringChars(env, str, NULL);
          
          LONG error = RegOpenKeyEx(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey,
              0,
              samDesired,
              &phkResult
          );
          
          (*env)->ReleaseStringChars(env, str, lpSubKey);
          
          jlong *temp = (jlong *) malloc(2 * sizeof(jlong));
          if (temp == NULL) {
              return NULL;
          }
          
          temp[0] = (jlong) error;
          temp[1] = ptr_to_jlong(phkResult);
          
          result = (*env)->NewLongArray(env, 2);
          if (result == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetLongArrayRegion(env, result, 0, 2, temp);
          free(temp);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegQueryInfoKey
 * Signature: (J)[I
 */
JNIEXPORT jintArray JNICALL Java_com_registry_WinRegistryAPI_RegQueryInfoKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          jintArray result;
          DWORD lpcSubKeys = 0, lpcMaxSubKeyLen = 0, lpcMaxClassLen = 0;
          DWORD lpcValues = 0, lpcMaxValueNameLen = 0, lpcMaxValueLen = 0;
          
          LONG error = RegQueryInfoKey(
              (HKEY) jlong_to_ptr(hKey),
              NULL,
              NULL,
              NULL,
              &lpcSubKeys,
              &lpcMaxSubKeyLen,
              &lpcMaxClassLen,
              &lpcValues,
              &lpcMaxValueNameLen,
              &lpcMaxValueLen,
              NULL,
              NULL
          );
          
          jint *temp = (jint *) malloc(7 * sizeof(jint));
          if (temp == NULL) {
              return NULL;
          }
          
          temp[0] = (jint) error;
          temp[1] = (jint) lpcSubKeys;
          temp[2] = (jint) lpcMaxSubKeyLen;
          temp[3] = (jint) lpcMaxClassLen;
          temp[4] = (jint) lpcValues;
          temp[5] = (jint) lpcMaxValueNameLen;
          temp[6] = (jint) lpcMaxValueLen;
          
          result = (*env)->NewIntArray(env, 7);
          if (result == NULL) {
              free(temp);
              return NULL;
          }
          
          (*env)->SetIntArrayRegion(env, result, 0, 7, temp);
          free(temp);
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegQueryMultipleValues
 * Signature: (J[Ljava/lang/String;)[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL Java_com_registry_WinRegistryAPI_RegQueryMultipleValues
  (JNIEnv *env, jclass cls, jlong hKey, jobjectArray valueNames) {
          if (valueNames == NULL) {
              return NULL;
          }
          
          jsize length = (*env)->GetArrayLength(env, valueNames);
          PVALENT val_list = (PVALENT) malloc(sizeof(VALENT) * length);
          if (val_list == NULL) {
              return NULL;
          }
          
          jclass Integer = (*env)->FindClass(env, "java/lang/Integer");
          jclass Long    = (*env)->FindClass(env, "java/lang/Long");
          if (Integer == NULL || Long == NULL) {
              free(val_list);
              return NULL;
          }
          
          jmethodID cstruct_i = (*env)->GetMethodID(env, Integer, "<init>", "(I)V");
          jmethodID cstruct_l = (*env)->GetMethodID(env, Long, "<init>", "(J)V");
          if (cstruct_i == NULL || cstruct_l == NULL) {
              free(val_list);
              return NULL;
          }
          
          jobjectArray result;
          jclass obj = (*env)->FindClass(env, "java/lang/Object");
          if (obj == NULL) {
              free(val_list);
              return NULL;
          }
          else {
              result = (*env)->NewObjectArray(env, (jsize) ((length * 2) + 1), obj, NULL);
              if (result == NULL) {
                  free(val_list);
                  return NULL;
              }
          }
          
          unsigned int i;
          DWORD num_vals = 0;
          for (i = 0; i < (unsigned int) length; i++) {
              jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
              if (str == NULL) {
                  val_list[num_vals++].ve_valuename = NULL;
                  continue;
              }
              
              val_list[num_vals++].ve_valuename = (LPWSTR) (*env)->GetStringChars(env, str, NULL);
          }
          
          DWORD dwTotsize = 0;
          LPWSTR lpValueBuf = NULL;
          
          LONG error = RegQueryMultipleValues(
              (HKEY) jlong_to_ptr(hKey),
              val_list,
              num_vals,
              lpValueBuf,
              &dwTotsize
          );
          
          BOOL ok = TRUE;
          
          if (error != ERROR_MORE_DATA) {
              ok = FALSE;
          } else {
              lpValueBuf = (LPWSTR) malloc(dwTotsize);
              if (lpValueBuf == NULL) {
                  ok = FALSE;
              } else {
                  error = RegQueryMultipleValues(
                      (HKEY) jlong_to_ptr(hKey),
                      val_list,
                      num_vals,
                      lpValueBuf,
                      &dwTotsize
                  );
                  
                  if (error != ERROR_SUCCESS) {
                      ok = FALSE;
                  } else {
                      unsigned int j;
                      for (i = 0, j = 1; i < num_vals; i++) {
                           VALENT val = val_list[i];
                           (*env)->SetObjectArrayElement(
                               env,
                               result,
                               j++,
                               (*env)->NewObject(env, Integer, cstruct_i, (jint) val_list[i].ve_type)
                           );
                           switch (val.ve_type) {
                                  case REG_SZ:
                                  case REG_EXPAND_SZ:
                                           if (val.ve_valuename != NULL) {
                                               jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                                               (*env)->ReleaseStringChars(env, str, (const jchar *) val.ve_valuename);
                                           }
                                           
                                           val.ve_valuelen /= sizeof(jchar);
                                           if (val.ve_valuelen != 0 && ((jchar *) val.ve_valueptr)[val.ve_valuelen - 1] == L'\0') {
                                               val.ve_valuelen--;
                                           }
                                           
                                           jstring str = (*env)->NewString(env, (const jchar *) val.ve_valueptr, val.ve_valuelen);
                                           
                                           (*env)->SetObjectArrayElement(env, result, j++, str);
                                           
                                           break;
                                  case REG_DWORD_LITTLE_ENDIAN:
                                  case REG_DWORD_BIG_ENDIAN:
                                           if (val.ve_valuelen == 4) {
                                               if (val.ve_valuename != NULL) {
                                                   jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                                                   (*env)->ReleaseStringChars(env, str, (const jchar *) val.ve_valuename);
                                               }
                                               
                                               DWORD a = 0, b = 0, c = 0, d = 0;
                                               
                                               // Integer: 0x12345678; a is lowest byte, d is highest byte
                                               if (val.ve_type == REG_DWORD_LITTLE_ENDIAN) {
                                                   // a = 0x12, b = 0x34, c = 0x56, d = 0x78
                                                   a = ((LPBYTE) val.ve_valueptr)[0];
                                                   b = ((LPBYTE) val.ve_valueptr)[1];
                                                   c = ((LPBYTE) val.ve_valueptr)[2];
                                                   d = ((LPBYTE) val.ve_valueptr)[3];
                                               }
                                               else {
                                                   // a = 0x78, b = 0x56, c = 0x34, d = 0x12
                                                   a = ((LPBYTE) val.ve_valueptr)[3];
                                                   b = ((LPBYTE) val.ve_valueptr)[2];
                                                   c = ((LPBYTE) val.ve_valueptr)[1];
                                                   d = ((LPBYTE) val.ve_valueptr)[0];
                                               }
                                               
                                               // number: 0xdcba
                                               DWORD number = (a & 0x000000ff)
                                                           | ((b << 8)  & 0x0000ff00)
                                                           | ((c << 16) & 0x00ff0000)
                                                           | ((d << 24) & 0xff000000);
                                               
                                               (*env)->SetObjectArrayElement(
                                                   env,
                                                   result,
                                                   j++,
                                                   (*env)->NewObject(env, Integer, cstruct_i, (jint) number)
                                               );
                                           }
                                           
                                           break;
                                  case REG_MULTI_SZ:
                                           if (val.ve_valuename != NULL) {
                                               jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                                               (*env)->ReleaseStringChars(env, str, (const jchar *) val.ve_valuename);
                                           }
                                           
                                           unsigned int q, lines = 0, count = 0, start = 0;
                                           for (q = 0; q < val.ve_valuelen - 1; q++) {
                                                if (((jchar *) val.ve_valueptr)[q] == L'\0') {
                                                    lines++;
                                                    if (((jchar *) val.ve_valueptr)[q + 1] == L'\0') {
                                                        break;
                                                    }
                                                }
                                           }
                                           
                                           jclass string = (*env)->FindClass(env, "java/lang/String");
                                           if (string == NULL) {
                                               j++;
                                               break;
                                           }
                                           
                                           jobjectArray mstr = (*env)->NewObjectArray(env, lines, string, NULL);
                                           if (mstr == NULL) {
                                               j++;
                                               break;
                                           }
                                           
                                           q = 0;
                                           while (q < val.ve_valuelen - 1 && count < lines) {
                                                  if (((jchar *) val.ve_valueptr)[q++] == L'\0') {
                                                      int len = q - start - 1;
                                                      jchar *ch = (jchar *) malloc(len * sizeof(jchar));
                                                      if (ch == NULL) {
                                                          break;
                                                      }
                                                      
                                                      int k;
                                                      for (k = 0; k < len; k++) {
                                                           ch[k] = (jchar) ((jchar *) val.ve_valueptr)[k + start];
                                                      }
                                                      
                                                      jstring s = (*env)->NewString(env, ch, len);
                                                      free(ch);
                                                      
                                                      (*env)->SetObjectArrayElement(env, mstr, count++, s);
                                                      
                                                      start = q;
                                                      if (((jchar *) val.ve_valueptr)[q] == L'\0') {
                                                          break;
                                                      }
                                                  }
                                           }
                                           
                                           (*env)->SetObjectArrayElement(env, result, j++, mstr);
                                           
                                           break;
                                  case REG_QWORD_LITTLE_ENDIAN:
                                           if (val.ve_valuelen == 8) {
                                               if (val.ve_valuename != NULL) {
                                                   jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                                                   (*env)->ReleaseStringChars(env, str, (const jchar *) val.ve_valuename);
                                               }
                                               
                                               DWORD data1 = (((LPBYTE) val.ve_valueptr)[0] & 0x000000ff)
                                                          | ((((LPBYTE) val.ve_valueptr)[1] << 8)  & 0x0000ff00)
                                                          | ((((LPBYTE) val.ve_valueptr)[2] << 16) & 0x00ff0000)
                                                          | ((((LPBYTE) val.ve_valueptr)[3] << 24) & 0xff000000);
                                               
                                               DWORD data2 = (((LPBYTE) val.ve_valueptr)[4] & 0x000000ff)
                                                          | ((((LPBYTE) val.ve_valueptr)[5] << 8)  & 0x0000ff00)
                                                          | ((((LPBYTE) val.ve_valueptr)[6] << 16) & 0x00ff0000)
                                                          | ((((LPBYTE) val.ve_valueptr)[7] << 24) & 0xff000000);
                                               
                                               jlong number = (jlong) data2;
                                                     number = (jlong) ((number << 32) | data1);
                                               
                                               (*env)->SetObjectArrayElement(
                                                   env,
                                                   result,
                                                   j++,
                                                   (*env)->NewObject(env, Long, cstruct_l, number)
                                               );
                                           }
                                           
                                           break;
                                  case REG_NONE:
                                  case REG_BINARY:
                                  case REG_LINK:
                                  case REG_RESOURCE_LIST:
                                  case REG_FULL_RESOURCE_DESCRIPTOR:
                                  case REG_RESOURCE_REQUIREMENTS_LIST:
                                  default:
                                           if (val.ve_valuename != NULL) {
                                               jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                                               (*env)->ReleaseStringChars(env, str, (const jchar *) val.ve_valuename);
                                           }
                                           
                                           val.ve_valuelen /= sizeof(jbyte);
                                           jbyteArray byt = (*env)->NewByteArray(env, val.ve_valuelen);
                                           if (byt != NULL) {
                                               (*env)->SetByteArrayRegion(env, byt, 0, val.ve_valuelen, (jbyte *) val.ve_valueptr);
                                               (*env)->SetObjectArrayElement(env, result, j++, byt);
                                           }
                                           else {
                                               j++;
                                           }
                                           
                                           break;
                           }
                      }
                      
                      free(lpValueBuf);
                      free(val_list);
                  }
              }
          }
          
          if (!ok) {
              num_vals = 0;
              for (i = 0; i < (unsigned int) length; i++) {
                  jstring str = (jstring) (*env)->GetObjectArrayElement(env, valueNames, i);
                  if (str == NULL) {
                      num_vals++;
                      continue;
                  }
                  
                  (*env)->ReleaseStringChars(env, str, (const jchar *) val_list[num_vals++].ve_valuename);
              }
              free(val_list);
              
              if (lpValueBuf != NULL)
                  free(lpValueBuf);
          }
          
          (*env)->SetObjectArrayElement(
              env,
              result,
              0,
              (*env)->NewObject(env, Integer, cstruct_i, (jint) error)
          );
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegQueryReflectionKey
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_registry_WinRegistryAPI_RegQueryReflectionKey
  (JNIEnv *env, jclass cls, jlong hKey) {
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          BOOL bIsReflectionDisabled = FALSE;
          LONG error = 0;
          
          if (hModule == NULL) {
              return JNI_FALSE;
          }
          
          RegQueryReflectionKeyType fn_RegQueryReflectionKey =
              (RegQueryReflectionKeyType) GetProcAddress(hModule, "RegQueryReflectionKey");
          if (fn_RegQueryReflectionKey != NULL) {
              error = (jint) fn_RegQueryReflectionKey(
                  (HKEY) jlong_to_ptr(hKey),
                  &bIsReflectionDisabled
              );
          }
          else {
              error = GetLastError();
          }
          
          if (error != ERROR_SUCCESS)
              return JNI_FALSE;
          return (bIsReflectionDisabled ? JNI_TRUE : JNI_FALSE);
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegQueryValueEx
 * Signature: (JLjava/lang/String;)[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL Java_com_registry_WinRegistryAPI_RegQueryValueEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring value) {
          jclass Integer = (*env)->FindClass(env, "java/lang/Integer");
          jclass Long    = (*env)->FindClass(env, "java/lang/Long");
          if (Integer == NULL || Long == NULL) {
              return NULL;
          }
          
          jmethodID cstruct_i = (*env)->GetMethodID(env, Integer, "<init>", "(I)V");
          jmethodID cstruct_l = (*env)->GetMethodID(env, Long, "<init>", "(J)V");
          if (cstruct_i == NULL || cstruct_l == NULL) {
              return NULL;
          }
          
          jobjectArray result;
          jclass obj = (*env)->FindClass(env, "java/lang/Object");
          if (obj == NULL) {
              return NULL;
          }
          else {
              result = (*env)->NewObjectArray(env, (jsize) 3, obj, NULL);
              if (result == NULL) {
                  return NULL;
              }
          }
          
          const jchar *lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          
          DWORD lpType = 0, lpcbData = 0, length = 0;
          LPBYTE lpData;
          jchar *buf;
          jbyte *bData;
          
          LONG error = RegQueryValueEx(
              (HKEY) jlong_to_ptr(hKey),
              lpValueName,
              NULL,
              &lpType,
              NULL,
              &lpcbData
          );
          
          if (error != ERROR_SUCCESS) {
              if (value != NULL) {
                  (*env)->ReleaseStringChars(env, value, lpValueName);
              }
              
              (*env)->SetObjectArrayElement(
                  env,
                  result,
                  0,
                  (*env)->NewObject(env, Integer, cstruct_i, (jint) error)
              );
              return result;
          }
          
          (*env)->SetObjectArrayElement(
              env,
              result,
              1,
              (*env)->NewObject(env, Integer, cstruct_i, (jint) lpType)
          );
          
          switch (lpType) {
                 case REG_SZ:
                 case REG_EXPAND_SZ:
                          buf = (jchar *) malloc(lpcbData);
                          if (buf == NULL) {
                              if (value != NULL) {
                                  (*env)->ReleaseStringChars(env, value, lpValueName);
                              }
                              
                              error = ERROR_OUTOFMEMORY;
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
                          
                          if (value != NULL) {
                              (*env)->ReleaseStringChars(env, value, lpValueName);
                          }
                          
                          if (error != ERROR_SUCCESS) {
                              free(buf);
                              break;
                          }
                          
                          length = (lpcbData / sizeof(jchar));
                          if (length != 0 && buf[length - 1] == L'\0') {
                              length--;
                          }
                          
                          jstring str = (*env)->NewString(env, buf, length);
                          free(buf);
                          
                          (*env)->SetObjectArrayElement(env, result, 2, str);
                          
                          break;
                 case REG_DWORD_LITTLE_ENDIAN:
                 case REG_DWORD_BIG_ENDIAN:
                          if (lpcbData == 4) {
                              lpData = (LPBYTE) malloc(lpcbData);
                              if (lpData == NULL) {
                                  if (value != NULL) {
                                      (*env)->ReleaseStringChars(env, value, lpValueName);
                                  }
                                  
                                  error = ERROR_OUTOFMEMORY;
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
                              
                              if (value != NULL) {
                                  (*env)->ReleaseStringChars(env, value, lpValueName);
                              }
                              
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
                              
                              (*env)->SetObjectArrayElement(
                                  env,
                                  result,
                                  2,
                                  (*env)->NewObject(env, Integer, cstruct_i, (jint) number)
                              );
                          }
                          
                          break;
                 case REG_MULTI_SZ:
                          buf = (jchar *) malloc(lpcbData);
                          if (buf == NULL) {
                              if (value != NULL) {
                                  (*env)->ReleaseStringChars(env, value, lpValueName);
                              }
                              
                              error = ERROR_OUTOFMEMORY;
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
                          
                          if (value != NULL) {
                              (*env)->ReleaseStringChars(env, value, lpValueName);
                          }
                          
                          if (error != ERROR_SUCCESS) {
                              free(buf);
                              break;
                          }
                          
                          unsigned int i, lines = 0, count = 0, start = 0;
                          for (i = 0; i < lpcbData - 1; i++) {
                               if (buf[i] == L'\0') {
                                   lines++;
                                   if (buf[i + 1] == L'\0') {
                                       break;
                                   }
                               }
                          }
                          
                          jclass string = (*env)->FindClass(env, "java/lang/String");
                          if (string == NULL) {
                              free(buf);
                              break;
                          }
                          
                          jobjectArray mstr = (*env)->NewObjectArray(env, lines, string, NULL);
                          if (mstr == NULL) {
                              free(buf);
                              
                              error = ERROR_OUTOFMEMORY;
                              break;
                          }
                          
                          i = 0;
                          while (i < lpcbData - 1 && count < lines) {
                                 if (buf[i++] == L'\0') {
                                     int len = i - start - 1;
                                     jchar *ch = (jchar *) malloc(len * sizeof(jchar));
                                     if (ch == NULL) {
                                         break;
                                     }
                                     
                                     int k;
                                     for (k = 0; k < len; k++) {
                                          ch[k] = (jchar) buf[k + start];
                                     }
                                     
                                     jstring s = (*env)->NewString(env, ch, len);
                                     free(ch);
                                     
                                     (*env)->SetObjectArrayElement(env, mstr, count++, s);
                                     
                                     start = i;
                                     if (buf[i] == L'\0') {
                                         break;
                                     }
                                 }
                          }
                          
                          (*env)->SetObjectArrayElement(env, result, 2, mstr);
                          free(buf);
                          
                          break;
                 case REG_QWORD_LITTLE_ENDIAN:
                          if (lpcbData == 8) {
                              lpData = (LPBYTE) malloc(lpcbData);
                              if (lpData == NULL) {
                                  if (value != NULL) {
                                      (*env)->ReleaseStringChars(env, value, lpValueName);
                                  }
                                  
                                  error = ERROR_OUTOFMEMORY;
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
                              
                              if (value != NULL) {
                                  (*env)->ReleaseStringChars(env, value, lpValueName);
                              }
                              
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
                              
                              jlong number = (jlong) data2;
                                    number = (jlong) ((number << 32) | data1);
                              free(lpData);
                              
                              (*env)->SetObjectArrayElement(
                                  env,
                                  result,
                                  2,
                                  (*env)->NewObject(env, Long, cstruct_l, number)
                              );
                          }
                          
                          break;
                 case REG_NONE:
                 case REG_BINARY:
                 case REG_LINK:
                 case REG_RESOURCE_LIST:
                 case REG_FULL_RESOURCE_DESCRIPTOR:
                 case REG_RESOURCE_REQUIREMENTS_LIST:
                 default:
                          bData = (jbyte *) malloc(lpcbData);
                          if (bData == NULL) {
                              if (value != NULL) {
                                  (*env)->ReleaseStringChars(env, value, lpValueName);
                              }
                              
                              error = ERROR_OUTOFMEMORY;
                              break;
                          }
                          
                          error = RegQueryValueEx(
                              (HKEY) jlong_to_ptr(hKey),
                              lpValueName,
                              NULL,
                              &lpType,
                              (LPBYTE) bData,
                              &lpcbData
                          );
                          
                          if (value != NULL) {
                              (*env)->ReleaseStringChars(env, value, lpValueName);
                          }
                          
                          if (error != ERROR_SUCCESS) {
                              free(bData);
                              break;
                          }
                          
                          length = (lpcbData / sizeof(jbyte));
                          jbyteArray byt = (*env)->NewByteArray(env, length);
                          if (byt != NULL) {
                              (*env)->SetByteArrayRegion(env, byt, 0, length, bData);
                              free(bData);
                              
                              (*env)->SetObjectArrayElement(env, result, 2, byt);
                          }
                          else {
                              free(bData);
                          }
                          
                          break;
          }
          
          (*env)->SetObjectArrayElement(
              env,
              result,
              0,
              (*env)->NewObject(env, Integer, cstruct_i, (jint) error)
          );
          
          return result;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegRenameKey
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegRenameKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring keyname) {
          HMODULE hModule = GetModuleHandle(L"ntdll.dll");
          UNICODE_STRING nt_newname;
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          NtRenameKeyType fn_NtRenameKey =
              (NtRenameKeyType) GetProcAddress(hModule, "NtRenameKey");
          if (fn_NtRenameKey != NULL) {
              const jchar *par_src = (keyname != NULL ? (*env)->GetStringChars(env, keyname, NULL) : NULL);
              
              if (keyname != NULL) {
                  WORD len = (WORD) ((*env)->GetStringLength(env, keyname) * sizeof(jchar));
                  nt_newname.Buffer = (jchar *) par_src;
                  nt_newname.Length = len;
                  nt_newname.MaximumLength = len + sizeof(jchar);
              }
              else {
                  nt_newname.Buffer = NULL;
                  nt_newname.Length = 0;
                  nt_newname.MaximumLength = 0;
              }
              
              NTSTATUS status = fn_NtRenameKey(
                  (HANDLE) jlong_to_ptr(hKey),
                  &nt_newname
              );
              
              if (keyname != NULL) {
                  (*env)->ReleaseStringChars(env, keyname, par_src);
              }
              
              RtlNtStatusToDosErrorType fn_RtlNtStatusToDosError =
                  (RtlNtStatusToDosErrorType) GetProcAddress(hModule, "RtlNtStatusToDosError");
              if (fn_RtlNtStatusToDosError != NULL) {
                  error = (jint) fn_RtlNtStatusToDosError(status);
              }
              else {
                  error = (jint) GetLastError();
              }
          }
          else {
              error = (jint) GetLastError();
          }
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegReplaceKey
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegReplaceKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring subKey, jstring newFile, jstring oldFile) {
          if (newFile == NULL || oldFile == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpNewFile = (*env)->GetStringChars(env, newFile, NULL);
          const jchar *lpOldFile = (*env)->GetStringChars(env, oldFile, NULL);
          const jchar *lpSubKey = (subKey != NULL ? (*env)->GetStringChars(env, subKey, NULL) : NULL);
          
          LONG error = RegReplaceKey(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey,
              lpNewFile,
              lpOldFile
          );
          
          if (subKey != NULL) {
              (*env)->ReleaseStringChars(env, subKey, lpSubKey);
          }
          (*env)->ReleaseStringChars(env, newFile, lpNewFile);
          (*env)->ReleaseStringChars(env, oldFile, lpOldFile);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegRestoreKey
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegRestoreKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring file, jint dwFlags) {
          if (file == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpFile = (*env)->GetStringChars(env, file, NULL);
          
          LONG error = RegRestoreKey(
              (HKEY) jlong_to_ptr(hKey),
              lpFile,
              dwFlags
          );
          
          (*env)->ReleaseStringChars(env, file, lpFile);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegSaveKey
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegSaveKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring file) {
          if (file == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          const jchar *lpFile = (*env)->GetStringChars(env, file, NULL);
          
          LONG error = RegSaveKey(
              (HKEY) jlong_to_ptr(hKey),
              lpFile,
              NULL
          );
          
          (*env)->ReleaseStringChars(env, file, lpFile);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegSaveKeyEx
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegSaveKeyEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring file, jint flags) {
          if (file == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          HANDLE hModule = GetModuleHandle(L"advapi32.dll");
          jint error = 0;
          
          if (hModule == NULL) {
              return (jint) GetLastError();
          }
          
          const jchar *lpFile = (*env)->GetStringChars(env, file, NULL);
          
          RegSaveKeyExType fn_RegSaveKeyEx =
              (RegSaveKeyExType) GetProcAddress(hModule, "RegSaveKeyExW");
          if (fn_RegSaveKeyEx != NULL) {
              error = (jint) fn_RegSaveKeyEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpFile,
                  NULL,
                  (DWORD) flags
              );
          }
          else {
              error = (jint) GetLastError();
          }
          
          (*env)->ReleaseStringChars(env, file, lpFile);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegSetLinkValue
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegSetLinkValue
  (JNIEnv *env, jclass cls, jlong hKey, jstring value, jstring data) {
          if (value == NULL || data == NULL) {
              return (jint) ERROR_INVALID_PARAMETER;
          }
          
          jint error = 0;
          
          const jchar *lpValueName = (*env)->GetStringChars(env, value, NULL);
          const jchar *lpData = (*env)->GetStringChars(env, data, NULL);
          
          jsize length   = (*env)->GetStringLength(env, data);
          DWORD lpcbData = (length * sizeof(jchar));
          
          jchar *buf = (jchar *) malloc(lpcbData);
          if (buf != NULL) {
              int i;
              for (i = 0; i < (int) length; i++) {
                   buf[i] = lpData[i];
              }
              
              error = (jint) RegSetValueEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpValueName,
                  0,
                  REG_LINK,
                  (LPBYTE) buf,
                  lpcbData
              );
              
              free(buf);
          }
          else {
              error = (jint) ERROR_OUTOFMEMORY;
          }
          
          (*env)->ReleaseStringChars(env, value, lpValueName);
          (*env)->ReleaseStringChars(env, data, lpData);
          
          return error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegSetValueEx
 * Signature: (JLjava/lang/String;ILjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegSetValueEx
  (JNIEnv *env, jclass cls, jlong hKey, jstring value, jint lpType, jobject data) {
          const jchar *lpValueName = (value != NULL ? (*env)->GetStringChars(env, value, NULL) : NULL);
          DWORD lpcbData = 0;
          LONG error = 0;
          
          LPBYTE lpData;
          if (data == NULL) {
              lpData   = NULL;
              lpcbData = 0;
              
              error = RegSetValueEx(
                  (HKEY) jlong_to_ptr(hKey),
                  lpValueName,
                  0,
                  lpType,
                  lpData,
                  lpcbData
              );
          }
          else {
              jarray array;
              jclass Integer, Long;
              jmethodID intValue, longValue;
              jstring string;
              
              switch ((DWORD) lpType) {
                  case REG_SZ:
                  case REG_EXPAND_SZ:
                       string = (jstring) data;
                       
                       jsize length = (*env)->GetStringLength(env, string);
                       lpcbData = (DWORD) ((length + 1) * sizeof(jchar));
                       
                       const jchar *str = (*env)->GetStringChars(env, string, NULL);
                       
                       jchar *buf = (jchar *) malloc(lpcbData);
                       if (buf != NULL) {
                           int i;
                           for (i = 0; i < (int) length; i++) {
                                buf[i] = str[i];
                           }
                           buf[(int) length] = L'\0';
                       }
                       else {
                           lpcbData = 0;
                       }
                       
                       error = RegSetValueEx(
                           (HKEY) jlong_to_ptr(hKey),
                           lpValueName,
                           0,
                           lpType,
                           (LPBYTE) buf,
                           lpcbData
                       );
                       
                       (*env)->ReleaseStringChars(env, string, str);
                       free(buf);
                       
                       break;
                  case REG_DWORD_LITTLE_ENDIAN:
                  case REG_DWORD_BIG_ENDIAN:
                       Integer = (*env)->GetObjectClass(env, data);
                       if (Integer == NULL) {
                           break;
                       }
                       
                       intValue = (*env)->GetMethodID(env, Integer, "intValue", "()I");
                       if (intValue == NULL) {
                           break;
                       }
                       
                       jint dword = (*env)->CallIntMethod(env, data, intValue);
                       if (((DWORD) lpType) == REG_DWORD_BIG_ENDIAN) {
                           DWORD a = (dword >> 24) & 0xff;
                           DWORD b = (dword >> 16) & 0xff;
                           DWORD c = (dword >>  8) & 0xff;
                           DWORD d =  dword        & 0xff;
                           
                           dword = (a & 0xff)
                                | ((b << 8)  & 0x0000ff00)
                                | ((c << 16) & 0x00ff0000)
                                | ((d << 24) & 0xff000000);
                       }
                       
                       lpcbData = 4;
                       
                       error = RegSetValueEx(
                           (HKEY) jlong_to_ptr(hKey),
                           lpValueName,
                           0,
                           lpType,
                           (LPBYTE) &dword,
                           lpcbData
                       );
                       
                       break;
                  case REG_MULTI_SZ:
                       array = (jarray) data;
                       jsize len = (*env)->GetArrayLength(env, array);
                       
                       int i;
                       lpcbData = 0;
                       for (i = 0; i < (int) len; i++) {
                           jstring str = (jstring) (*env)->GetObjectArrayElement(env, array, i);
                           lpcbData += (((*env)->GetStringLength(env, str) + 1) * sizeof(jchar));
                       }
                       
                       jchar *multi = (jchar *) malloc(lpcbData + sizeof(jchar));
                       if (multi != NULL) {
                           int k, c = 0;
                           for (i = 0; i < (int) len; i++) {
                                jstring str = (jstring) (*env)->GetObjectArrayElement(env, array, i);
                                const jchar *buf = (*env)->GetStringChars(env, str, NULL);
                                jsize strlen = (*env)->GetStringLength(env, str);
                                
                                for (k = 0; k < (int) strlen; k++)
                                     multi[c++] = buf[k];
                                multi[c++] = L'\0';
                                
                                (*env)->ReleaseStringChars(env, str, buf);
                           }
                           
                           multi[c] = L'\0';
                           lpcbData += sizeof(jchar);
                       }
                       else {
                           lpcbData = 0;
                       }
                       
                       error = RegSetValueEx(
                           (HKEY) jlong_to_ptr(hKey),
                           lpValueName,
                           0,
                           lpType,
                           (LPBYTE) multi,
                           lpcbData
                       );
                       
                       free(multi);
                       
                       break;
                  case REG_QWORD_LITTLE_ENDIAN:
                       Long = (*env)->GetObjectClass(env, data);
                       if (Long == NULL) {
                           break;
                       }
                       
                       longValue = (*env)->GetMethodID(env, Long, "longValue", "()J");
                       if (longValue == NULL) {
                           break;
                       }
                       
                       jlong qword = (*env)->CallLongMethod(env, data, longValue);
                       
                       lpcbData = 8;
                       
                       error = RegSetValueEx(
                           (HKEY) jlong_to_ptr(hKey),
                           lpValueName,
                           0,
                           lpType,
                           (LPBYTE) &qword,
                           lpcbData
                       );
                       
                       break;
                  case REG_NONE:
                  case REG_BINARY:
                  case REG_LINK:
                  case REG_RESOURCE_LIST:
                  case REG_FULL_RESOURCE_DESCRIPTOR:
                  case REG_RESOURCE_REQUIREMENTS_LIST:
                  default:
                       array  = (jarray) data;
                       lpData = (LPBYTE) (*env)->GetByteArrayElements(env, array, NULL);
                       
                       lpcbData = (*env)->GetArrayLength(env, array) * sizeof(jbyte);
                       
                       error = RegSetValueEx(
                           (HKEY) jlong_to_ptr(hKey),
                           lpValueName,
                           0,
                           lpType,
                           (LPBYTE) lpData,
                           lpcbData
                       );
                       
                       (*env)->ReleaseByteArrayElements(env, array, (jbyte *) lpData, 0);
                       
                       break;
              }
          }
          
          if (value) {
              (*env)->ReleaseStringChars(env, value, lpValueName);
          }
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    RegUnLoadKey
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_registry_WinRegistryAPI_RegUnLoadKey
  (JNIEnv *env, jclass cls, jlong hKey, jstring subKey) {
          const jchar *lpSubKey = (subKey != NULL ? (*env)->GetStringChars(env, subKey, NULL) : NULL);
          
          LONG error = RegUnLoadKey(
              (HKEY) jlong_to_ptr(hKey),
              lpSubKey
          );
          
          (*env)->ReleaseStringChars(env, subKey, lpSubKey);
          
          return (jint) error;
  }

/*
 * Class:     com_registry_WinRegistryAPI
 * Method:    SetPrivilege
 * Signature: (ZLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_registry_WinRegistryAPI_SetPrivilege
  (JNIEnv *env, jclass cls, jboolean bEnablePrivilege, jstring name) {
          TOKEN_PRIVILEGES tp;
          LUID luid;
          HANDLE hToken;
          
          const jchar *lpszPrivilege = (*env)->GetStringChars(env, name, NULL);
          if (!LookupPrivilegeValue((LPCWSTR) NULL, (LPCWSTR) lpszPrivilege, &luid)) {
             (*env)->ReleaseStringChars(env, name, lpszPrivilege);
             return JNI_FALSE;
          }
          (*env)->ReleaseStringChars(env, name, lpszPrivilege);
          
          if (!OpenProcessToken(
                   GetCurrentProcess(),
                   TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                   &hToken))
                       return JNI_FALSE;
          
          tp.PrivilegeCount = 1;
          tp.Privileges[0].Luid = luid;
          if (bEnablePrivilege == JNI_TRUE)
             tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
          else
              tp.Privileges[0].Attributes = 0;
          
          if (!AdjustTokenPrivileges(
                   hToken,
                   FALSE,
                   &tp,
                   sizeof(TOKEN_PRIVILEGES),
                   (PTOKEN_PRIVILEGES) NULL,
                   (PDWORD) NULL)) {
                       CloseHandle(hToken);
                       return JNI_FALSE;
          }
          
          jboolean result = (GetLastError() == ERROR_NOT_ALL_ASSIGNED ? JNI_FALSE : JNI_TRUE);
          
          CloseHandle(hToken);
          
          return result;
  }
