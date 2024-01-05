#include <Windows.h>
#include <conio.h>
#include <iostream>

extern "C" {
#include <valinet/pdb/pdb.h>
#include <valinet/utility/memmem.h>
#include <valinet/utility/takeown.h>
}

class Helper {
public:
	static std::string AskSysDir() {
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

	static std::string AskCWD() {
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

	static int FileExists(std::string file) {
		return fileExists((char*)file.c_str());
	}

	static void KillDWM() {
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
			reinterpret_cast<LPSTARTUPINFOA> (&si),
			&pi
		);
		WaitForSingleObject(pi.hProcess, INFINITE);
		Sleep(10000);
	}

	static std::string DownloadSymbol(std::string dllFilename) {
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

	static void DeleteTheFile(std::string filename) {
		DeleteFileA((LPCSTR)filename.c_str());
	}

	static BOOL MoveTheFile(std::string src, std::string dst) {
		return MoveFileA((LPCSTR)src.c_str(), (LPCSTR)dst.c_str());
	}
};

int main(int argc, char** argv)
{
	/*
	** Determine Patch/Restore
	*/
    BOOL bRestore = Helper::FileExists(Helper::AskSysDir() + "\\uDWM_win11drc.bak");  // Patch: 0, Restore: 1

	std::string szOriginalDWM = Helper::AskSysDir() + "\\uDWM_win11drc.bak";  // System32\uDWM_win11drc.bak
	std::string szDWM = Helper::AskSysDir() + "\\uDWM.dll";  // System32\uDWM.dll
	std::string szModifiedDWM = bRestore ? Helper::AskSysDir() + "\\uDWMm.dll" : Helper::AskCWD() + "\\uDWM.dll";  // RESTORE MODE: System32\uDWMm.dll; PATCH MODE: CWD\uDWM.dll

	/*
	** RESTORE MODE
	*/
    if (bRestore)
    {
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWMm.dll");  // Delete System32\uDWMm.dll
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWMm.dll"))  // Rename: System32\uDWM.dll => System32\uDWMm.dll
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 1;
        }
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM_win11drc.bak", Helper::AskSysDir() + "\\uDWM.dll"))  // Rename: System32\uDWM_win11drc.bak => System32\uDWM.dll
        {
            printf("Unable to restore DWM.\n");
            _getch();
            return 2;
        }
		Helper::KillDWM();  // Taskkill dwm.exe
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWMm.dll");  // Delete: System32\uDWMm.dll
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
        
		std::string pdbFileName = Helper::DownloadSymbol(szModifiedDWM);

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

		// ----- LPTSTR Charset Problem Start -----

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

		// ----- LPTSTR Charset Problem End -----

		szOriginalDWM = Helper::AskSysDir() + "\\uDWM_win11drc.bak1";
        DeleteFileA((LPCSTR)szOriginalDWM.c_str());  // Delete if existed: System32\uDWM_win11drc.bak1
        if (!MoveFileA((LPCSTR)szDWM.c_str(), (LPCSTR)szOriginalDWM.c_str()))  // Rename System32\uDWM.dll as System32\uDWM_win11drc.bak1
        {
            printf("Unable to prepare for replacing system file.\n");
            _getch();
            return 9;
        }
        if (!CopyFileA((LPCSTR)szModifiedDWM.c_str(), (LPCSTR)szDWM.c_str(), FALSE))  // Patch CWD\uDWM.dll to System32\uDWM.dll
        {
            printf("Unable to replace system file.\n");
            _getch();
            return 10;
        }
        DeleteFileA((LPCSTR)szModifiedDWM.c_str());  // Delete: CWD\uDWM.dll

		Helper::KillDWM();
		DeleteFileA((LPCSTR)szOriginalDWM.c_str());  // Delete System32\uDWM_win11drc.bak1
    }

    std::cout << "Operation successful." << std::endl;
}