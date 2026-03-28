#pragma once
#include"WinPch.h"

#include<Windows.h>
#include<TlHelp32.h>
using PROCESSENTRY32A = tagPROCESSENTRY32;
using LPPROCESSENTRY32A = PROCESSENTRY32A*;
using PPROCESSENTRY32A = PROCESSENTRY32A*;
BOOL WINAPI Process32FirstA(
    HANDLE hSnapshot,
    LPPROCESSENTRY32A lppe
);

BOOL WINAPI Process32NextA(
    HANDLE hSnapshot,
    LPPROCESSENTRY32A lppe
);