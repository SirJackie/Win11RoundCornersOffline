#include "Helper.h"

extern "C" {
	#include <valinet/pdb/pdb.h>
	#include <valinet/utility/memmem.h>
	#include <valinet/utility/takeown.h>
}

std::string Helper::AskSysDir() {
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

std::string Helper::AskCWD() {
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

int Helper::FileExists(std::string file) {
	return fileExists((char*)file.c_str());
}

void Helper::KillDWM() {
	std::string szTaskkill = "\"" + Helper::AskSysDir() + "\\taskkill.exe\" /f /im dwm.exe";  // "C:\Windows\system32\taskkill.exe\" /f /im dwm.exe

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

std::string Helper::DownloadSymbol(std::string dllFilename) {
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

void Helper::DeleteTheFile(std::string filename) {
	DeleteFileA((LPCSTR)filename.c_str());
}

BOOL Helper::MoveTheFile(std::string src, std::string dst) {
	return MoveFileA((LPCSTR)src.c_str(), (LPCSTR)dst.c_str());
}

BOOL Helper::CopyTheFile(std::string src, std::string dst) {
	return CopyFileA((LPCSTR)src.c_str(), (LPCSTR)dst.c_str(), FALSE);
}

wchar_t* Helper::StdString2WideCharArray(std::string str) {
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

BOOL Helper::TakeOwnership(std::string filename) {
	wchar_t* sysDWMPath_Wide = Helper::StdString2WideCharArray(filename);  // Convert to UNICODE

	BOOL result = VnTakeOwnership(sysDWMPath_Wide);

	delete[] sysDWMPath_Wide;
	sysDWMPath_Wide = nullptr;

	return result;
}

BOOL Helper::Patch_uDWM_dll(std::string dll, std::string pdb) {

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
