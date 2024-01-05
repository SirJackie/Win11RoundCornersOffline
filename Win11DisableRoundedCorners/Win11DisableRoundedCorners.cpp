#include <iostream>
#include "Helper.h"

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
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_win11drc.bak1"))  // Rename System32\uDWM.dll as System32\uDWM_win11drc.bak1
        {
            printf("Unable to prepare for replacing system file.\n");
            _getch();
            return 9;
        }
        if (!Helper::CopyTheFile(Helper::AskCWD() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM.dll"))  // Patch CWD\uDWM.dll to System32\uDWM.dll
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
