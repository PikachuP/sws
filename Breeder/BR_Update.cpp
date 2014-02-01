/******************************************************************************
/ BR_Update.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://www.standingwaterstudios.com/reaper
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/
#include "stdafx.h"
#include "BR_Update.h"
#include "BR_Util.h"
#include "../version.h"
#include "../SnM/SnM_Dlg.h"
#include "../reaper/localize.h"
#include "../../WDL/jnetlib/jnetlib.h"
#include "../../WDL/jnetlib/httpget.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const OFFICIAL_VERSION_URL = "http://sws-extension.googlecode.com/svn/tags/release/version.h";
const char* const BETA_VERSION_URL     = "http://sws-extension.googlecode.com/svn/trunk/version.h";
const char* const OFFICIAL_DOWNLOAD    = "http://www.standingwaterstudios.com/new.php";
const char* const BETA_DOWNLOAD        = "http://code.google.com/p/sws-extension/downloads/list";
const char* const STARTUP_VERSION_KEY  = "BR - StartupVersionCheck";

const int SEARCH_TIMEOUT     =  5;

const int SEARCH_INITIATED   = -2;
const int NO_CONNECTION      = -1;
const int UP_TO_DATE         =  0;
const int OFFICIAL_AVAILABLE =  1;
const int BETA_AVAILABLE     =  2;
const int BOTH_AVAILABLE     =  3;

/******************************************************************************
* Dialog functionality                                                        *
******************************************************************************/
static void SetVersionMessage (HWND hwnd, BR_SearchObject* searchObject)
{
	BR_Version versionO, versionB;
	int status = searchObject->GetStatus(&versionO, &versionB);
	char tmp[256] = {0};

	if (status == SEARCH_INITIATED)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), false);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_SHOW);
		SendMessage(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), PBM_SETPOS, 0, 0);

	}
	else if (status == NO_CONNECTION)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Connection could not be established","sws_DLG_172"));
		SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), __LOCALIZE("Retry","sws_DLG_172"));
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), true);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}
	else if (status == UP_TO_DATE)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("SWS extension is up to date.","sws_DLG_172"));
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), false);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}
	else if (status == OFFICIAL_AVAILABLE)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Official update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), versionO.maj, versionO.min, versionO.rev, versionO.build);
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), true);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}
	else if (status == BETA_AVAILABLE)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Beta update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), versionB.maj, versionB.min, versionB.rev, versionB.build);
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), true);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}
	else if (status == BOTH_AVAILABLE)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Official update is available: Version %d.%d.%d Build #%d\nBeta update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), versionO.maj, versionO.min, versionO.rev, versionO.build, versionB.maj, versionB.min, versionB.rev, versionB.build);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}

	SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE), tmp);
}

static WDL_DLGRET DialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static BR_SearchObject* searchObject = NULL;
	#ifndef _WIN32
		static bool positionSet;
	#endif

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			searchObject = (BR_SearchObject*)lParam;
			SetVersionMessage(hwnd, searchObject);
			SetTimer(hwnd, 1, 100, NULL);

			#ifdef _WIN32
				CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
			#else
				positionSet = false;
			#endif
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!positionSet)
					CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
				positionSet = true;
			}
			break;
		#endif

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_BR_VER_DOWNLOAD:
				{
					if (searchObject->GetStatus(NULL, NULL) == NO_CONNECTION)
					{
						searchObject->RestartSearch();
						SetVersionMessage(hwnd, searchObject);
						SetTimer(hwnd, 1, 100, NULL);
					}
					if (searchObject->GetStatus(NULL, NULL) == OFFICIAL_AVAILABLE)
					{
						ShellExecute(NULL, "open", OFFICIAL_DOWNLOAD, NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}
					else if (searchObject->GetStatus(NULL, NULL) == BETA_AVAILABLE)
					{
						ShellExecute(NULL, "open", BETA_DOWNLOAD, NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}
				}
				break;

				case IDC_BR_VER_OFF:
				{
					ShellExecute(NULL, "open", OFFICIAL_DOWNLOAD , NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;

				case IDC_BR_VER_BETA:
				{
					ShellExecute(NULL, "open", BETA_DOWNLOAD, NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					EndDialog(hwnd, 0);
				}
				break;
			}
		}
		break;

		case WM_TIMER:
		{
			SendMessage(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), PBM_SETPOS, (int)(searchObject->GetProgress() * 100.0), 0);

			if (searchObject->GetStatus(NULL, NULL) != SEARCH_INITIATED)
			{
				SetVersionMessage(hwnd, searchObject);
				KillTimer(hwnd, 1);
			}
		}
		break;

		case WM_DESTROY:
		{
			KillTimer(hwnd, 1);
			searchObject = NULL;
		}
		break;
	}
	return 0;
}

/******************************************************************************
* Startup functionality                                                       *
******************************************************************************/
void StartupSearch ()
{
	static BR_SearchObject* searchObject = new (nothrow) BR_SearchObject(true);

	if (searchObject)
	{
		int status = searchObject->GetStatus(NULL, NULL);
		if (status != SEARCH_INITIATED)
		{
			if (status == OFFICIAL_AVAILABLE || status == BETA_AVAILABLE || status == BOTH_AVAILABLE)
				DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_VERSION), g_hwndParent, DialogProc, (LPARAM)searchObject);
			delete searchObject;
			searchObject = NULL;
		}
	}
	else
		plugin_register("-timer",(void*)StartupSearch);
}

void VersionCheckInit ()
{
	// Get options
	bool official, beta; unsigned int lastTime;
	GetStartupSearchOptions (&official, &beta, &lastTime);

	if (official || beta)
	{
		// Make sure at least 24 hours have passed since last search
		unsigned int currentTime = (unsigned int)time(NULL);
		if (currentTime - lastTime >= 86400 || currentTime - lastTime < 0)
		{
			// Write current time
			SetStartupSearchOptions (official, beta, currentTime);

			// Register timer that starts after reaper loads, from there we start search
			plugin_register("timer",(void*)StartupSearch);
		}
	}
}

/******************************************************************************
* Command and preferences                                                    *
******************************************************************************/
void VersionCheckAction (COMMAND_T* ct)
{
	VersionCheckDialog(g_hwndParent);
}

void VersionCheckDialog (HWND hwnd)
{
	BR_SearchObject* searchObject = new (nothrow) BR_SearchObject();
	if (searchObject)
	{
		DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_VERSION), hwnd, DialogProc, (LPARAM)searchObject);
		delete searchObject;
		searchObject = NULL;
	}
}

void GetStartupSearchOptions (bool* official, bool* beta, unsigned int* lastTime)
{
	char tmp[256]; int tempO, tempB; unsigned int tempT;
	GetPrivateProfileString("SWS", STARTUP_VERSION_KEY, "1 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d %u", &tempO, &tempB, &tempT);

	WritePtr(official, !!tempO);
	WritePtr(beta,     !!tempB);
	WritePtr(lastTime, tempT  );
}

void SetStartupSearchOptions (bool official, bool beta, unsigned int lastTime)
{
	char tmp[256];
	if (lastTime == 0) // lastTime = 0 will prevent overwriting current lastTime
	{
		GetPrivateProfileString("SWS", STARTUP_VERSION_KEY, "1 0 0", tmp, 256, get_ini_file());
		sscanf(tmp, "%*d %*d %d", &lastTime);
	}
	_snprintf(tmp, sizeof(tmp), "%d %d %u", !!official, !!beta, lastTime);
	WritePrivateProfileString("SWS", STARTUP_VERSION_KEY, tmp, get_ini_file());
}

/******************************************************************************
* BR_SearchObject                                                             *
******************************************************************************/
BR_SearchObject::BR_SearchObject (bool startup /*= false*/) :
m_startup  (startup),
m_status   (SEARCH_INITIATED),
m_progress (0)
{
	this->EndSearch(); // make sure any existing search thread finishes
	this->SetProcess(CreateThread(NULL, 0, this->StartSearch, (void*)this, 0, NULL));
}

BR_SearchObject::~BR_SearchObject ()
{
	this->EndSearch();
}

int BR_SearchObject::GetStatus (BR_Version* official, BR_Version* beta)
{
	SWS_SectionLock lock(&m_mutex);
	if (official)
		*official = m_official;
	if (beta)
		*beta = m_beta;
	return m_status;
}

double BR_SearchObject::GetProgress ()
{
	return m_progress;
}

bool BR_SearchObject::GetKillFlag ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_killFlag;
}

bool BR_SearchObject::IsStartup ()
{
	return m_startup;
}

void BR_SearchObject::RestartSearch ()
{
	BR_Version version;
	this->EndSearch();
	this->SetProgress(0);
	this->SetStatus(version, version, SEARCH_INITIATED);
	this->SetProcess(CreateThread(NULL, 0, this->StartSearch, (void*)this, 0, NULL));
}

DWORD WINAPI BR_SearchObject::StartSearch (void* searchObject)
{
	BR_SearchObject* _this = (BR_SearchObject*)searchObject;
	JNL::open_socketlib();

	// Get local version
	BR_Version versionL;
	sscanf(SWS_VERSION_STR, "%d,%d,%d,%d", &versionL.maj, &versionL.min, &versionL.rev, &versionL.build);

	// When searching by user request, lookup both official and beta
	bool searchO = true, searchB = true;
	if (_this->IsStartup())
		GetStartupSearchOptions(&searchO, &searchB, NULL);

	// Get official version and compare
	BR_Version versionO;
	int statusO = NO_CONNECTION;
	if (searchO && !_this->GetKillFlag())
	{
		JNL_HTTPGet web;
		web.addheader("User-Agent:SWS (Mozilla)");
		web.addheader("Accept:*/*");
		web.connect(OFFICIAL_VERSION_URL);
		time_t startTime = time(NULL);
		while (time(NULL) - startTime <= SEARCH_TIMEOUT && !_this->GetKillFlag())
		{
			// Set progress bar used in dialog
			_this->SetProgress((double)(time(NULL) - startTime) / (SEARCH_TIMEOUT*2));

			// Try to get version.h
			int run = web.run();
			if (run < 0 || web.get_status() < 0 || web.getreplycode() >= 400)
				break;
			else if (run == 1 && web.getreplycode() == 200) //  get data only after the connection has closed
			{
				// Parse version.h and save results
				int size = web.bytes_available();
				if (char* buf = new (nothrow) char[size])
				{
					// Get official version number
					web.get_bytes(buf, size);
					char* token = strtok(buf, "\n");
					while (token != NULL)
					{
						if (sscanf(token, "#define SWS_VERSION %d,%d,%d,%d", &versionO.maj, &versionO.min, &versionO.rev, &versionO.build) > 0)
							break;
						token = strtok(NULL, "\n");
					}

					// Compare with local version and save status
					if (_this->CompareVersion(versionO, versionL) == 1)
						statusO = OFFICIAL_AVAILABLE;
					else
						statusO = UP_TO_DATE;

					delete buf;
					break;
				}
			}
		}
	}

	// Get beta version and compare
	BR_Version versionB;
	int statusB = NO_CONNECTION;
	if (searchB && !_this->GetKillFlag())
	{
		JNL_HTTPGet web;
		web.addheader("User-Agent:SWS (Mozilla)");
		web.addheader("Accept:*/*");
		web.connect(BETA_VERSION_URL);
		time_t startTime = time(NULL);
		while (time(NULL) - startTime <= SEARCH_TIMEOUT && !_this->GetKillFlag())
		{
			// Set progress bar used in dialog
			_this->SetProgress((double)(time(NULL) - startTime) / (SEARCH_TIMEOUT*2) + 0.5);

			// Try to get version.h
			int run = web.run();
			if (run < 0 || web.get_status() < 0 || web.getreplycode() >= 400)
				break;
			else if (run == 1 && web.getreplycode() == 200) //  get data only after the connection has closed
			{
				// Parse version.h and save results
				int size = web.bytes_available();
				if (char* buf = new (nothrow) char[size])
				{
					// Get beta version number
					web.get_bytes(buf, size);
					char* token = strtok(buf, "\n");
					while (token != NULL)
					{
						if (sscanf(token, "#define SWS_VERSION %d,%d,%d,%d", &versionB.maj, &versionB.min, &versionB.rev, &versionB.build) > 0)
							break;
						token = strtok(NULL, "\n");
					}

					// Compare with local version and save status
					if (_this->CompareVersion(versionB, versionL) == 1)
						statusB = BETA_AVAILABLE;
					else
						statusB = UP_TO_DATE;

					delete buf;
					break;
				}
			}
		}
	}

	// Make progress bar 100% when done searching
	_this->SetProgress(1);

	// Give some breathing space to hwndDlg (i.e. SNM startup action may load screenset that repositions Reaper...)
	if (_this->IsStartup() && !_this->GetKillFlag())
		Sleep(1500);

	// When both versions are available and are identical, ignore beta
	if (statusO == OFFICIAL_AVAILABLE && statusB == BETA_AVAILABLE)
		if (!_this->CompareVersion(versionO, versionB))
			statusB = UP_TO_DATE;

	// Save status
	int status;
	if      (statusO == NO_CONNECTION && statusB == NO_CONNECTION)       status = NO_CONNECTION;
	else if (statusO == OFFICIAL_AVAILABLE && statusB == BETA_AVAILABLE) status = BOTH_AVAILABLE;
	else if (statusO == OFFICIAL_AVAILABLE)                              status = OFFICIAL_AVAILABLE;
	else if (statusB == BETA_AVAILABLE)                                  status = BETA_AVAILABLE;
	else                                                                 status = UP_TO_DATE;

	_this->SetStatus(versionO, versionB, status);
	JNL::close_socketlib();
	return 0;
}

void BR_SearchObject::SetStatus (BR_Version official, BR_Version beta, int status)
{
	SWS_SectionLock lock(&m_mutex);

	m_official = official;
	m_beta = beta;
	m_status = status;
}

void BR_SearchObject::SetProgress (double progress)
{
	m_progress = progress;
}

void BR_SearchObject::SetKillFlag (bool killFlag)
{
	SWS_SectionLock lock(&m_mutex);
	m_killFlag = killFlag;
}

void BR_SearchObject::EndSearch ()
{
	if (this->GetProcess() != NULL)
	{
		this->SetKillFlag(true);
		WaitForSingleObject(this->GetProcess(), INFINITE);
		this->SetKillFlag(false);

		CloseHandle(this->GetProcess());
		this->SetProcess(NULL);
	}
}

void BR_SearchObject::SetProcess (HANDLE process)
{
	SWS_SectionLock lock(&m_mutex);
	m_hProcess = process;
}

HANDLE BR_SearchObject::GetProcess ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_hProcess;
}

int BR_SearchObject::CompareVersion (BR_Version one, BR_Version two)
{
	// return 0 if equal
	//        1 if one is greater
	//        2 if two is greater

	if (one.maj > two.maj)
		return 1;
	else if (one.maj < two.maj)
		return 2;
	if (one.min > two.min)
		return 1;
	else if (one.min < two.min)
		return 2;
	if (one.rev > two.rev)
		return 1;
	else if (one.rev < two.rev)
		return 2;
	if (one.build > two.build)
		return 1;
	else if (one.build < two.build)
		return 2;
	return 0;
}

bool BR_SearchObject::m_killFlag = false;
HANDLE BR_SearchObject::m_hProcess = NULL;