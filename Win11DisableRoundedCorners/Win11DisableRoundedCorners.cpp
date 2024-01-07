#include <iostream>
#include <cassert>
#include "Helper.h"

int main(int argc, char** argv)
{
	/*
	** Determine Patch/Restore
	*/

	// If patching-backup doesn't existed, do the patching.
    BOOL needPatch = !Helper::FileExists(Helper::AskSysDir() + "\\uDWM_win11drc.bak");

	/*
	** PATCH MODE
	*/

	if (needPatch)
    {
		/*
		** Generate Patched DLL: CWD\uDWM.dll
		*/

		// Copy£ºSystem32\uDWM.dll => CWD\uDWM.dll
        if (!Helper::CopyTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskCWD() + "\\uDWM.dll"))
        {
            printf(
                "Temporary file copy failed. Make sure the application has write "
                "access to the folder it runs from.\n"
            );
            _getch();
        }
        
		// Make Sure PDB File Exist: CWD\uDWM.pdb
		std::string pdbFileName = Helper::AskCWD() + "\\uDWM.pdb";

		if (Helper::FileExists(pdbFileName)) {
			printf("uDWM.pdb already existed, no need to download.\n");
		}
		else {
			printf("uDWM.pdb not found, downloading from the Internet.....\n");
			std::string pdbDownloadedFileName = Helper::DownloadSymbol(Helper::AskCWD() + "\\uDWM.dll");
			assert(pdbDownloadedFileName == pdbFileName);
		}

		// Patch CWD\uDWM.dll using CWD\uDWM.pdb
		if (!Helper::Patch_uDWM_dll(Helper::AskCWD() + "\\uDWM.dll", pdbFileName)) {
			printf("Failed to patch uDWM.dll.\n");
			_getch();
		}

		/*
		** Bypass System Restrictions and Delete: System32\uDWM.dll
		** 1. Take Ownership
		** 2. uDWM.dll cannot be directly deleted, so:
		**    a) rename: uDWM.dll => uDWM_tmp.dll
		**    b) delete: uDWM_tmp.dll
		** 3. Save a backup for later restore: uDWM_win11drc.bak
		*/

		// Copy: System32\\uDWM.dll => System32\uDWM_win11drc.bak
        if (!Helper::CopyTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_win11drc.bak"))
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

		// Delete if existed: System32\uDWM_tmp.dll
        Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_tmp.dll");

		// Rename: System32\uDWM.dll => System32\uDWM_tmp.dll
        if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_tmp.dll"))
        {
            printf("Unable to prepare for replacing system file.\n");
            _getch();
            return 9;
        }

		// Delete System32\uDWM_tmp.dll
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_tmp.dll");

		/*
		** Patch the Generated uDWM.dll
		*/

		// Copy: CWD\uDWM.dll => System32\uDWM.dll
        if (!Helper::CopyTheFile(Helper::AskCWD() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM.dll"))  // Patch CWD\uDWM.dll to System32\uDWM.dll
        {
            printf("Unable to replace system file.\n");
            _getch();
        }

		// Delete: CWD\uDWM.dll
        Helper::DeleteTheFile(Helper::AskCWD() + "\\uDWM.dll");

		/*
		** Refresh DWM by Killing dwm.exe. (it will start automatically)
		*/

		Helper::KillDWM();
    }

	/*
	** RESTORE MODE
	*/
	else
	{
		/*
		** Recover uDWM.dll from uDWM_win11drc.bak
		*/

		// Delete:  System32\uDWM_tmp.dll
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_tmp.dll");

		// Rename: System32\uDWM.dll => System32\uDWM_tmp.dll
		if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM.dll", Helper::AskSysDir() + "\\uDWM_tmp.dll"))
		{
			printf("Unable to restore DWM.\n");
			_getch();
		}

		// Rename: System32\uDWM_win11drc.bak => System32\uDWM.dll
		if (!Helper::MoveTheFile(Helper::AskSysDir() + "\\uDWM_win11drc.bak", Helper::AskSysDir() + "\\uDWM.dll"))
		{
			printf("Unable to restore DWM.\n");
			_getch();
		}

		// Delete: System32\uDWM_tmp.dll
		Helper::DeleteTheFile(Helper::AskSysDir() + "\\uDWM_tmp.dll");

		/*
		** Refresh DWM by Killing dwm.exe. (it will start automatically)
		*/

		Helper::KillDWM();
	}

    std::cout << "Operation successful." << std::endl;
	std::cout << "--------------------------------------------------" << std::endl;
	std::cout << "Win11RoundCornersOffline.exe is a tool to disable " << std::endl
		<< "round corners in Windows 11 without an Internet connection." << std::endl
		<< "View source code on: https://github.com/SirJackie/Win11RoundCornersOffline" << std::endl
		<< "Press any key to exit..." << std::endl;

	_getch();
}
