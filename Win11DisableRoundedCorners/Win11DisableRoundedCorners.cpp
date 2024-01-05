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
	
	static BOOL CopyTheFile(std::string src, std::string dst) {
		return CopyFileA((LPCSTR)src.c_str(), (LPCSTR)dst.c_str(), FALSE);
	}

	static wchar_t* StdString2WideCharArray(std::string str) {
		// This Function Allocate Extra Memory, Remember to delete[] it!!!
		char chartmp[MAX_PATH];
		ZeroMemory(
			chartmp,
			(MAX_PATH) * sizeof(char)
		);
		strcpy(chartmp, str.c_str());

		int num = MultiByteToWideChar(0, 0, chartmp, -1, NULL, 0);
		wchar_t* widetmp = new wchar_t[num];
		MultiByteToWideChar(0, 0, chartmp, -1, widetmp, num);

		return widetmp;
	}

	static BOOL TakeOwnership(std::string filename) {
		wchar_t* sysDWMPath_Wide = Helper::StdString2WideCharArray(filename);  // Convert to UNICODE

		BOOL result = VnTakeOwnership(sysDWMPath_Wide);
		
		delete[] sysDWMPath_Wide;
		sysDWMPath_Wide = nullptr;

		return result;
	}

	static BOOL Patch_uDWM_dll(std::string dll, std::string pdb) {

		BOOL success = TRUE;

		// Get Function Address in PDB File
		DWORD addr[1] = { 0 };
		char* name[1] = { "CTopLevelWindow::GetEffectiveCornerStyle" };
		if (VnGetSymbols(
			(LPCSTR)pdb.c_str(),
			addr,
			name,
			1
		))
		{
			printf("Unable to determine function address.\n");
			success = FALSE;
		}
		printf("Function address is: 0x%x.\n", addr[0]);

		// Delete: CWD\uDWM.pdb
		Helper::DeleteTheFile(pdb);

		// Patch: CWD\uDWM.dll
		HANDLE hFile = CreateFileA(
			(LPCSTR)dll.c_str(),
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
			success = FALSE;
		}
		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (hFileMapping == 0)
		{
			printf("Unable to create file mapping.\n");
			success = FALSE;
		}
		char* lpFileBase = (char*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (lpFileBase == 0)
		{
			printf("Unable to memory map system file.\n");
			success = FALSE;
		}
		char szPayload[8] = { 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xc3 }; // mov rax, 0; ret
		memcpy(lpFileBase + addr[0], szPayload, sizeof(szPayload));
		UnmapViewOfFile(lpFileBase);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);

		return success;
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
        }
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM_win11drc.bak", Helper::AskSysDir() + "\\uDWM.dll"))  // Rename: System32\uDWM_win11drc.bak => System32\uDWM.dll
        {
            printf("Unable to restore DWM.\n");
            _getch();
        }
		Helper::KillDWM();  // Taskkill dwm.exe
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWMm.dll");  // Delete: System32\uDWMm.dll
    }

	/*
	** PATCH MODE
	*/
    else
    {
        if (!Helper::CopyTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskCWD() + "\\uDWM.dll"))  // Copy£ºSystem32\uDWM.dll => CWD\uDWM.dll
        {
            printf(
                "Temporary file copy failed. Make sure the application has write "
                "access to the folder it runs from.\n"
            );
            _getch();
        }
        
		std::string pdbFileName = Helper::DownloadSymbol(Helper::AskSysDir() + "\\uDWM.dll");

		// Patch DLL using PDB File
		if (!Helper::Patch_uDWM_dll(Helper::AskCWD() + "\\uDWM.dll", pdbFileName)) {
			printf("Failed to patch uDWM.dll.\n");
			_getch();
		}

        if (!Helper::CopyTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_win11drc.bak"))  // Rename: System32\\uDWM.dll => System32\uDWM_win11drc.bak
        {
            printf("Unable to backup system file.\n");
            _getch();
        }

		// Take Ownership: System32\uDWM.dll
		if (!Helper::TakeOwnership(Helper::AskSysDir() + "\\uDWM.dll"))
		{
			printf("Unable to take ownership of system file.\n");
			_getch();
		}

        Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_win11drc.bak1");  // Delete if existed: System32\uDWM_win11drc.bak1
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_win11drc.bak"))  // Rename System32\uDWM.dll as System32\uDWM_win11drc.bak1
        {
            printf("Unable to prepare for replacing system file.\n");
            _getch();
            return 9;
        }
        if (!CopyFileA((LPCSTR)szModifiedDWM.c_str(), (LPCSTR)szDWM.c_str(), FALSE))  // Patch CWD\uDWM.dll to System32\uDWM.dll
        {
            printf("Unable to replace system file.\n");
            _getch();
        }
        Helper::DeleteTheFile(Helper::AskCWD() + "\\uDWM.dll");  // Delete: CWD\uDWM.dll

		Helper::KillDWM();
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_win11drc.bak1");  // Delete System32\uDWM_win11drc.bak1
    }

    std::cout << "Operation successful." << std::endl;
}
