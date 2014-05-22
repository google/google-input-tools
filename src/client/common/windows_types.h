/*
  Copyright 2014 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// This file includes weird Windows types

#ifndef GOOPY_COMMON_WINDOWS_TYPES_H_
#define GOOPY_COMMON_WINDOWS_TYPES_H_
#define MAX_PATH 260
#define TRUE true
#define FALSE false
typedef int BOOL;
typedef int HANDLE;
typedef unsigned char BYTE;
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short *PUSHORT;
typedef long LONG;
typedef long HRESULT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef ULONGLONG *PULONGLONG;
typedef unsigned long ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef void VOID;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef wchar_t WCHAR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;
typedef DWORD *LPDWORD;
typedef unsigned long UINT_PTR;
typedef UINT_PTR SIZE_T;
typedef LONGLONG USN;
typedef BYTE BOOLEAN;
typedef void *PVOID;

typedef struct _RECT {
  long left;
  long top;
  long right;
  long bottom;
} RECT, *PRECT;

#endif  // GOOPY_COMMON_WINDOWS_TYPES_H_
