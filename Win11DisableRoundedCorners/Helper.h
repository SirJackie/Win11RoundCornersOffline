#ifndef __HELPER_H__
#define __HELPER_H__

#include <Windows.h>
#include <conio.h>
#include <iostream>

class Helper {
public:
	static std::string AskCWD();
	static std::string AskSysDir();
	static std::string DownloadSymbol(std::string dllFilename);

	static BOOL TakeOwnership(std::string filename);
	static BOOL MoveTheFile(std::string src, std::string dst);
	static BOOL CopyTheFile(std::string src, std::string dst);
	static BOOL Patch_uDWM_dll(std::string dll, std::string pdb);

	static int       FileExists(std::string file);
	static void      KillDWM();
	static void      DeleteTheFile(std::string filename);
	static wchar_t*  StdString2WideCharArray(std::string str);
};

#endif
