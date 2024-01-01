#undef UNICODE
#include <Windows.h>
#include <conio.h>

extern "C" {
#include <valinet/pdb/pdb.h>
#include <valinet/utility/memmem.h>
#include <valinet/utility/takeown.h>
}

int main(int argc, char** argv)
{
    BOOL bRestore = FALSE;

	/*
	** Save szTaskkill（杀死进程的命令）
	*/
	// szTaskkill = C:\Windows\system32\taskkill.exe /f /im dwm.exe
    char szTaskkill[MAX_PATH];
    ZeroMemory(
        szTaskkill,
        (MAX_PATH) * sizeof(char)
    );
    szTaskkill[0] = '\"';
    GetSystemDirectoryA(
        szTaskkill + sizeof(char),
        MAX_PATH
    );  // szTaskkill => C:\Windows\System32
    strcat_s(
        szTaskkill,
        MAX_PATH,
        "\\taskkill.exe\" /f /im dwm.exe"
    );  // szTaskkill => C:\Windows\system32\taskkill.exe /f /im dwm.exe

	/*
	** Save szPath（当前EXE名字）
	*/
	// szPath = Win11DisableOrRestoreRoundedCorners.exe
    char szPath[_MAX_PATH];
    ZeroMemory(
        szPath,
        (_MAX_PATH) * sizeof(char)
    );
    GetModuleFileNameA(
        GetModuleHandle(NULL),
        szPath,
        _MAX_PATH
    );  // szPath => C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\Win11DisableOrRestoreRoundedCorners.exe
    PathStripPathA(szPath);  // szPath => Win11DisableOrRestoreRoundedCorners.exe

	/*
	** Save szOriginalPath（未修改的原始DLL备份）
	*/
	// szOriginalDWM = C:\Windows\system32\uDWM_win11drc.bak
    char szOriginalDWM[_MAX_PATH];
    ZeroMemory(
        szOriginalDWM,
        (_MAX_PATH) * sizeof(char)
    );
    GetSystemDirectoryA(
        szOriginalDWM,
        MAX_PATH
    );
    strcat_s(
        szOriginalDWM,
        MAX_PATH,
        "\\uDWM_win11drc.bak"
    );

	/*
	** Determine Patch/Restore
	*/
    bRestore = fileExists(szOriginalDWM);  // Patch: 0, Restore: 1

	/*
	** Save szModifiedPath（已修改的DLL，PATCH模式指向下载的DLL，RESTORE模式指向System32的DLL）
	*/
	// PATCH MODE: szModifiedDWM = C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
	// RESTORE MODE: szModifiedDWM = C:\Windows\system32\uDWMm.dll
    char szModifiedDWM[_MAX_PATH];
    ZeroMemory(
        szModifiedDWM,
        (_MAX_PATH) * sizeof(char)
    );
    if (bRestore)  // RESTORE MODE
    {
        /*
        char szSfcscannow[MAX_PATH];
        ZeroMemory(
            szSfcscannow,
            (MAX_PATH) * sizeof(char)
        );
        szSfcscannow[0] = '\"';
        GetSystemDirectoryA(
            szSfcscannow + sizeof(char),
            MAX_PATH
        );
        strcat_s(
            szSfcscannow,
            MAX_PATH,
            "\\sfc.exe\" /scannow"
        );
        STARTUPINFO si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        BOOL b = CreateProcessA(
            NULL,
            szSfcscannow,
            NULL,
            NULL,
            TRUE,
            CREATE_UNICODE_ENVIRONMENT,
            NULL,
            NULL,
            &si,
            &pi
        );
        WaitForSingleObject(pi.hProcess, INFINITE);
        DeleteFileA(szOriginalDWM);
        printf("Operation successful.\n");
        Sleep(10000);
        ExitWindowsEx(EWX_REBOOT | EWX_FORCE | EWX_FORCEIFHUNG, SHTDN_REASON_FLAG_PLANNED);
        exit(0);
        */
        GetSystemDirectoryA(
            szModifiedDWM,
            MAX_PATH
        );    
        strcat_s(
            szModifiedDWM,
            MAX_PATH,
            "\\uDWMm.dll"
        );  // RESTORE MODE: szModifiedDWM = C:\Windows\system32\uDWMm.dll
    }
    else  // PATCH MODE
    {
        GetModuleFileNameA(
            GetModuleHandle(NULL),
            szModifiedDWM,
            _MAX_PATH
        );
        PathRemoveFileSpecA(szModifiedDWM);
        strcat_s(
            szModifiedDWM,
            MAX_PATH,
            "\\uDWM.dll"
        );  // PATCH MODE: szModifiedDWM = C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
    }

	/*
	** Save szDWM（系统的DLL）
	*/
	// szDWM = C:\Windows\system32\uDWM.dll
    char szDWM[MAX_PATH];
    ZeroMemory(
        szDWM,
        (MAX_PATH) * sizeof(char)
    );
    GetSystemDirectoryA(
        szDWM,
        MAX_PATH
    );
    strcat_s(
        szDWM,
        MAX_PATH,
        "\\uDWM.dll"
    );  // szDWM = C:\Windows\system32\uDWM.dll

	/*
	** RESTORE模式
	*/
    if (bRestore)
    {
        DeleteFileA(szModifiedDWM);  // 删除: "C:\\Windows\\system32\\uDWMm.dll"
        if (!MoveFileA(szDWM, szModifiedDWM))  // 移动: "C:\\Windows\\system32\\uDWM.dll" => "C:\\Windows\\system32\\uDWMm.dll"
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 1;
        }
        if (!MoveFileA(szOriginalDWM, szDWM))  // 移动: "C:\\Windows\\system32\\uDWM_win11drc.bak" => "C:\\Windows\\system32\\uDWM.dll"
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 2;
        }
    }
	// Patch模式
    else
    {
        if (!CopyFileA(szDWM, szModifiedDWM, FALSE))  // 移动：C:\Windows\system32\uDWM.dll => C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
        {
            printf(
                "Temporary file copy failed. Make sure the application has write "
                "access to the folder it runs from.\n"
            );
            _getch();
            return 1;
        }
        if (VnDownloadSymbols(
            NULL,
            szModifiedDWM,
            szModifiedDWM,
            _MAX_PATH
        ))  // 下载：C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.pdb
			// Side Effect: Setting szModifiedDWM = C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.pdb
        {
            printf(
                "Unable to download symbols. Make sure you have a working Internet "
                "connection.\n"
            );
            _getch();
            return 2;
        }
        DWORD addr[1] = { 0 };
        char* name[1] = { "CTopLevelWindow::GetEffectiveCornerStyle" };
        if (VnGetSymbols(  // Get Symbol Address in uDWM.pdb
            szModifiedDWM,
            addr,
            name,
            1
        ))
        {
            printf("Unable to determine function address.\n");
            _getch();
            return 3;
        }
        printf("Function address is: 0x%x.\n", addr[0]);
        DeleteFileA(szModifiedDWM);  // Delete: szModifiedDWM = C:\User\\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.pdb
        PathRemoveFileSpecA(szModifiedDWM);  // 设置: szModifiedDWM = C:\User\\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug
        strcat_s(
            szModifiedDWM,
            MAX_PATH,
            "\\uDWM.dll"
        );  // 设置: szModifiedDWM = C:\User\\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
        HANDLE hFile = CreateFileA(
            szModifiedDWM,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0
        );  // 创建: C:\User\\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
        if (!hFile)
        {
            printf("Unable to open system file.\n");
            _getch();
            return 4;
        }
        HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (hFileMapping == 0)
        {
            printf("Unable to create file mapping.\n");
            _getch();
            return 5;
        }
        char* lpFileBase = (char*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (lpFileBase == 0)
        {
            printf("Unable to memory map system file.\n");
            _getch();
            return 6;
        }
        char szPayload[8] = { 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xc3}; // mov rax, 0; ret
        memcpy(lpFileBase + addr[0], szPayload, sizeof(szPayload));
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);  // Finished patching the file: C:\User\\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug\uDWM.dll
        if (!CopyFileA(szDWM, szOriginalDWM, FALSE))  // Backup "C:\\Windows\\system32\\uDWM.dll" as "C:\\Windows\\system32\\uDWM_win11drc.bak"
        {
            printf("Unable to backup system file.\n");
            _getch();
            return 9;
        }
        if (!VnTakeOwnership(szDWM))  // TAKE OWNERSHIP: "C:\\Windows\\system32\\uDWM.dll"
        {
            printf("Unable to take ownership of system file.\n");
            _getch();
            return 8;
        }
        strcat_s(
            szOriginalDWM,
            MAX_PATH,
            "1"
        );  // szOriginalDWM = "C:\\Windows\\system32\\uDWM_win11drc.bak1"
        DeleteFileA(szOriginalDWM);  // Delete if existed: "C:\\Windows\\system32\\uDWM_win11drc.bak1"
        if (!MoveFileA(szDWM, szOriginalDWM))  // Backup "C:\\Windows\\system32\\uDWM.dll" as "C:\\Windows\\system32\\uDWM_win11drc.bak1"
        {
            printf("Unable to prepare for replacing system file.\n");
            _getch();
            return 9;
        }
        if (!CopyFileA(szModifiedDWM, szDWM, FALSE))  // Patch "C:\\Users\\Windows To Go\\Desktop\\Win11DisableRoundedCorners\\x64\\Debug\\uDWM.dll" to "C:\\Windows\\system32\\uDWM.dll"
        {
            printf("Unable to replace system file.\n");
            _getch();
            return 10;
        }
        DeleteFileA(szModifiedDWM);  // Delete: "C:\\Users\\Windows To Go\\Desktop\\Win11DisableRoundedCorners\\x64\\Debug\\uDWM.dll"
    }

	/*
	** Kill dwm.exe
	*/
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL b = CreateProcessA(
        NULL,
        szTaskkill,
        NULL,
        NULL,
        TRUE,
        CREATE_UNICODE_ENVIRONMENT,
        NULL,
        NULL,
        &si,
        &pi
    );
    WaitForSingleObject(pi.hProcess, INFINITE);
    Sleep(10000);


    if (bRestore)
    {
        DeleteFileA(szModifiedDWM);  // delete "C:\\Windows\\system32\\uDWMm.dll"
    }
    else
    {
        DeleteFileA(szOriginalDWM);  // delete "C:\\Windows\\system32\\uDWM_win11drc.bak1"
    }
    printf("Operation successful.\n");
	return 0;
}