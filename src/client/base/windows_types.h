// Copyright (C) 2013 and onwards Google, Inc.
// Author: Difan Zhang
//
// This file includes weird Windows types

#ifndef IME_GOOPY_COMMON_WINDOWS_TYPES_H_
#define IME_GOOPY_COMMON_WINDOWS_TYPES_H_
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

#endif  // IME_GOOPY_COMMON_WINDOWS_TYPES_H_
