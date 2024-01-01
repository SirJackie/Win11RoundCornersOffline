#include <Windows.h>
#include <conio.h>
#include <iostream>
#define _CRT_SECURE_NO_WARNINGS

extern "C" {
#include <valinet/pdb/pdb.h>
#include <valinet/utility/memmem.h>
#include <valinet/utility/takeown.h>
}

std::string AskSysDir() {
	char sysDir[MAX_PATH];
	ZeroMemory(
		sysDir,
		(MAX_PATH) * sizeof(char)
	);
	GetSystemDirectoryA(
		sysDir,
		MAX_PATH
	);  // sysDir = C:\Windows\System32
	return std::string(sysDir);
}

std::string AskCWD() {
	char cwd[_MAX_PATH];
	ZeroMemory(
		cwd,
		(_MAX_PATH) * sizeof(char)
	);
	GetModuleFileNameA(
		GetModuleHandle(NULL),
		cwd,
		_MAX_PATH
	);
	PathRemoveFileSpecA(cwd);
	// modFn => C:\Users\Windows To Go\Desktop\Win11DisableRoundedCorners\x64\Debug
	return std::string(cwd);
}

int FileExists(std::string file) {
	return fileExists((char*)file.c_str());
}

void KillDWM() {
	std::string szTaskkill = "\"" + AskSysDir() + "\\taskkill.exe\" /f /im dwm.exe";  // "C:\Windows\system32\taskkill.exe\" /f /im dwm.exe

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	BOOL b = CreateProcessA(
		NULL,
		(LPSTR)szTaskkill.c_str(),
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
}

std::string DownloadSymbol(std::string dllFilename) {
	// Input: CWD\uDWM.dll
	// Effect: Download and Create CWD\uDWM.pdb
	// Output: CWD\uDWM.pdb

	char dllFn[_MAX_PATH];
	ZeroMemory(
		dllFn,
		(_MAX_PATH) * sizeof(char)
	);
	strcpy(dllFn, dllFilename.c_str());  // dllFn = CWD\uDWM.dll

	// Download: CWD\uDWM.pdb
	// Side Effect: Setting dllFn to CWD\uDWM.pdb
	if (VnDownloadSymbols(
		NULL,
		dllFn,
		dllFn,
		_MAX_PATH
	))
	{
		printf(
			"Unable to download symbols. Make sure you have a working Internet "
			"connection.\n"
		);
		_getch();
	}
	return std::string(dllFn);  // Output: CWD\uDWM.pdb
}

int main(int argc, char** argv)
{

    BOOL bRestore = FALSE;

	std::string szOriginalDWM = AskSysDir() + "\\uDWM_win11drc.bak";  // System32\uDWM_win11drc.bak
	std::string szDWM = AskSysDir() + "\\uDWM.dll";  // System32\uDWM.dll

	/*
	** Determine Patch/Restore
	*/
    bRestore = FileExists(szOriginalDWM);  // Patch: 0, Restore: 1

	// Get String: szModifiedDWM
	std::string szModifiedDWM;
	// RESTORE MODE: System32\uDWMm.dll
    if (bRestore) szModifiedDWM = AskSysDir() + "\\uDWMm.dll";  // 
	// PATCH MODE: CWD\uDWM.dll
	else szModifiedDWM = AskCWD() + "\\uDWM.dll";

	/*
	** RESTORE MODE
	*/
    if (bRestore)
    {
        DeleteFileA((LPCSTR)szModifiedDWM.c_str());  // Delete System32\uDWMm.dll
        if (!MoveFileA((LPCSTR)szDWM.c_str(), (LPCSTR)szModifiedDWM.c_str()))  // Rename: System32\uDWM.dll => System32\uDWMm.dll
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 1;
        }
        if (!MoveFileA((LPCSTR)szOriginalDWM.c_str(), (LPCSTR)szDWM.c_str()))  // Rename: System32\uDWM_win11drc.bak => System32\uDWM.dll
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 2;
        }
		KillDWM();  // Taskkill dwm.exe
		DeleteFileA((LPCSTR)szModifiedDWM.c_str());  // Delete: System32\uDWMm.dll
    }

	/*
	** PATCH MODE
	*/
    else
    {
        if (!CopyFileA((LPCSTR)szDWM.c_str(), (LPCSTR)szModifiedDWM.c_str(), FALSE))  // Copy£ºSystem32\uDWM.dll => CWD\uDWM.dll
        {
            printf(
                "Temporary file copy failed. Make sure the application has write "
                "access to the folder it runs from.\n"
            );
            _getch();
            return 1;
        }
        
		std::string pdbFileName = DownloadSymbol(szModifiedDWM);

		/*
		** ----- Start Patching DLL using PDB File -----
		*/

		// Get Function Address in PDB File
        DWORD addr[1] = { 0 };
        char* name[1] = { "CTopLevelWindow::GetEffectiveCornerStyle" };
        if (VnGetSymbols(
            (LPCSTR)pdbFileName.c_str(),
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

		// Delete: CWD\uDWM.pdb
        DeleteFileA((LPCSTR)pdbFileName.c_str());

        // Patch: CWD\uDWM.dll
        HANDLE hFile = CreateFileA(
			(LPCSTR)szModifiedDWM.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0
        );
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

		/*
		** ----- Finish Patching DLL using PDB File -----
		*/

        if (!CopyFileA((LPCSTR)szDWM.c_str(), (LPCSTR)szOriginalDWM.c_str(), FALSE))  // Rename: System32\\uDWM.dll => System32\uDWM_win11drc.bak
        {
            printf("Unable to backup system file.\n");
            _getch();
            return 9;
        }

		char xxx[MAX_PATH];
		ZeroMemory(
			xxx,
			(MAX_PATH) * sizeof(char)
		);
		strcpy(xxx, szDWM.c_str());

		int num = MultiByteToWideChar(0, 0, xxx, -1, NULL, 0);
		wchar_t* wide = new wchar_t[num];
		MultiByteToWideChar(0, 0, xxx, -1, wide, num);

        if (!VnTakeOwnership(wide))  // TAKE OWNERSHIP: System32\uDWM.dll
        {
            printf("Unable to take ownership of system file.\n");
            _getch();
            return 8;
        }
        strcat_s(
			(char*)szOriginalDWM.c_str(),
            MAX_PATH,
            "1"
        );  // szOriginalDWM = "C:\\Windows\\system32\\uDWM_win11drc.bak1"
        DeleteFileA((LPCSTR)szOriginalDWM.c_str());  // Delete if existed: "C:\\Windows\\system32\\uDWM_win11drc.bak1"
        if (!MoveFileA(szDWM, (LPCSTR)szOriginalDWM.c_str()))  // Backup "C:\\Windows\\system32\\uDWM.dll" as "C:\\Windows\\system32\\uDWM_win11drc.bak1"
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

		KillDWM();
		DeleteFileA((LPCSTR)szOriginalDWM.c_str());  // delete "C:\\Windows\\system32\\uDWM_win11drc.bak1"
    }

    std::cout << "Operation successful." << std::endl;
}