/**
 * -----------------------------------------------------
 * File        calladmin-client.cpp
 * Authors     David O., Impact
 * License     GPLv3
 * Web         http://popoklopsi.de, http://gugyclan.eu
 * -----------------------------------------------------
 *
 * Copyright (C) 2013-2016 David O., Impact
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */
#include <wx/cmdline.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/xrc/xmlres.h>

#include "calladmin-client.h"
#include "curl_util.h"

#ifdef __WXMSW__
	// Memory leak detection for debugging 
	#include <wx/msw/msvcrt.h>
#endif


// Help for the CMDLine
static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "taskbar", "taskbar", "Move GUI to taskbar on start" },
	{ wxCMD_LINE_NONE }
};


// Implement the APP
wxIMPLEMENT_APP(CallAdmin);


CallAdmin::CallAdmin() {
	// Reset vars
	this->timer = NULL;
	this->config = NULL;
	this->mainFrame = NULL;
	this->taskBarIcon = NULL;
	this->steamThread = NULL;
	this->curlThread = NULL;

	this->startInTaskbar = false;
	this->isRunning = true;

	// Attempts to Zero
	this->attempts = 0;

	// Avatar Size
	this->avatarSize = 184;
}


// App Started
bool CallAdmin::OnInit() {
	if (!wxApp::OnInit()) {
		return false;
	}

	// Check duplicate
	static wxSingleInstanceChecker checkInstance("CallAdmin Client - " + wxGetUserId(), Config::GetConfigDir());

	if (checkInstance.IsAnotherRunning()) {
		wxMessageBox("CallAdmin Client is already running.", "CallAdmin Client", wxOK | wxCENTRE | wxICON_EXCLAMATION);

		return false;
	}

	// Calculate avatar size
	int y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);

	if (y < 900) {
		// Only use small avatars
		this->avatarSize = 128;
	} else if (y < 700) {
		this->avatarSize = 96;
	}

	// Init XML Resources
	wxImage::AddHandler(new wxICOHandler());
	wxImage::AddHandler(new wxBMPHandler());

	wxXmlResource::Get()->InitAllHandlers();
	InitXmlResource();

	// Create and parse config
	this->config = new Config();
	this->config->ParseConfig();

	// Create MainFrame
	this->mainFrame = new MainFrame();

	// Init MainFrame
	if (!this->mainFrame->InitFrame(this->startInTaskbar)) {
		ExitProgramm();

		return false;
	}

	// Create CURL Thread
	this->curlThread = new CurlThread();

	// Create Icon
	this->taskBarIcon = new TaskBarIcon();

	// Parse the config panel which starts everything else
	caGetConfigPanel()->ParseConfig();

	// Check for an update
	CheckUpdate();

	return true;
}


// Set Help text
void CallAdmin::OnInitCmdLine(wxCmdLineParser &parser) {
	// Add Help
	parser.SetDesc(cmdLineDesc);

	// Start with -
	parser.SetSwitchChars("-");
}


// Find -taskbar
bool CallAdmin::OnCmdLineParsed(wxCmdLineParser &parser) {
	this->startInTaskbar = parser.Found("taskbar") && TaskBarIcon::IsAvailable();

	return true;
}


void CallAdmin::StartTimer() {
	LogAction("Starting the timer", LogLevel::LEVEL_DEBUG);

	if (this->timer) {
		delete this->timer;
	}

	this->attempts = 0;

	this->timer = new Timer();
	this->timer->Run(this->config->GetStep() * 1000);
}


void CallAdmin::StartSteamThread() {
	if (!this->steamThread) {
		LogAction("Starting the steam thread", LogLevel::LEVEL_DEBUG);

		this->steamThread = new SteamThread();
	}
}


void CallAdmin::StartUpdate() {
	LogAction("Starting an update", LogLevel::LEVEL_DEBUG);

	ExitProgramm();

#if defined(__WXMSW__)
	wxExecute(GetRelativePath("calladmin-client-updater.exe -version " + wxString(CALLADMIN_CLIENT_VERSION) +
			  " -url " + wxString(CALLADMIN_UPDATE_URL) + " -executable " + wxString(CALLADMIN_UPDATE_EXE)));
#else
	wxExecute(GetRelativePath("calladmin-client-updater -version " + wxString(CALLADMIN_CLIENT_VERSION) +
			  " -url " + wxString(CALLADMIN_UPDATE_URL) + " -executable " + wxString(CALLADMIN_UPDATE_EXE)));
#endif
}


// Check for a new Version
void CallAdmin::CheckUpdate() {
	// Log Action
	LogAction("Checking for a new Update", LogLevel::LEVEL_DEBUG);

	GetPage(CallAdmin::OnUpdate, CALLADMIN_UPDATE_URL);
}


// Create the Window as a reconnecter
void CallAdmin::CreateReconnect(wxString error) {
	// Log Action
	LogAction("Too much errors. Manual reconnect needed", LogLevel::LEVEL_ERROR);

	// Stop timer
	this->timer->Stop();

	mainFrame->SetTitle("Error: Couldn't Connect");
	mainFrame->GetNotebook()->GetPanel<MainPanel *>(MAIN_PANEL_PAGE)->SetStatusText("Error: Please reconnect manually");
	mainFrame->GetNotebook()->GetPanel<MainPanel *>(MAIN_PANEL_PAGE)->SetReconnectButton(true);

	// Show it
	mainFrame->Show(true);
	mainFrame->Iconize(false);

	// And raise it
	if (!isOtherInFullscreen()) {
		mainFrame->Raise();
	}

	// Go to main page
	mainFrame->GetNotebook()->GetWindow()->ChangeSelection(MAIN_PANEL_PAGE);
}


// Create an new Error Dialog
void CallAdmin::ShowError(wxString error, wxString type) {
	// Show an error message and increase attempts
	this->attempts++;

	// Max attempts reached?
	if (this->attempts >= this->config->GetMaxAttempts()) {
		// Create reconnect
		CreateReconnect("CURL Error: " + error);
	}

	if (this->attempts <= this->config->GetMaxAttempts()) {
		this->taskBarIcon->ShowMessage("An error occured", type + " Error : " + error + "\nRetry " + (wxString() << this->attempts) + " of " + (wxString() << this->config->GetMaxAttempts()), this->mainFrame, true);
	}
}


// Close Taskbar Icon and destroy all dialogs
void CallAdmin::ExitProgramm() {
	this->isRunning = false;

	// Hide all windows
	if (this->mainFrame) {
		this->mainFrame->Show(false);
	}

	for (wxVector<CallDialog *>::iterator callDialog = callDialogs.begin(); callDialog != callDialogs.end(); ++callDialog) {
		(*callDialog)->Show(false);
	}

	// Taskbar Icon goodbye :)
	if (this->taskBarIcon) {
		this->taskBarIcon->RemoveIcon();
	}

	// Delete threads before windows
	wxDELETE(this->curlThread);
	wxDELETE(this->steamThread);

	// And also the timer
	if (this->timer) {
		wxDELETE(this->timer);
	}

	// Then process pending events
	if (HasPendingEvents()) {
		ProcessPendingEvents();
	}
	
	// Taskbar goodbye :)
	if (this->taskBarIcon) {
		this->taskBarIcon->Destroy();
		this->taskBarIcon = NULL;
	}

	// Delete call dialogs
	for (wxVector<CallDialog *>::iterator callDialog = callDialogs.begin(); callDialog != callDialogs.end(); ++callDialog) {
		(*callDialog)->Destroy();
	}

	callDialogs.clear();

	// Destroy main frame
	if (this->mainFrame) {
		// Delete notebook and main frame
		delete this->mainFrame->GetNotebook();

		this->mainFrame->Destroy();
		this->mainFrame = NULL;
	}

	// Delete config
	wxDELETE(this->config);
}


// Curl thread handled -> Call function
void CallAdmin::OnCurlThread(CurlThreadData *data) {
	if (!this->isRunning) {
		delete data;
		return;
	}

	// Get Function
	CurlCallback function = data->GetCallbackFunction();

	// Call it
	if (function) {
		function(data->GetError(), data->GetContent(), data->GetExtra());
	}

	// Delete data
	delete data;
}


// Get the content of a page
void CallAdmin::GetPage(CurlCallback callbackFunction, wxString page, int extra) {
	if (this->isRunning && !this->curlThread->GetThread()) {
		this->curlThread->SetCallbackFunction(callbackFunction);
		this->curlThread->SetPage(page);
		this->curlThread->SetExtra(extra);

		this->curlThread->CreateThread(wxTHREAD_DETACHED);
		this->curlThread->GetThread()->Run();
	}
}


// Get the Path of the App
wxString CallAdmin::GetRelativePath(wxString relativeFileOrPath) {
	wxString path = wxStandardPaths::Get().GetExecutablePath();

	// Windows format?
	size_t start = path.find_last_of("\\");

	if (start == 0 || start == wxString::npos) {
		// No... Linux Format ;)
		start = path.find_last_of("/");
	}

	// Strip app name
	path = path.replace(start, path.size(), "");

	// Add file name
#if defined(__WXMSW__)
	return path + "\\" + relativeFileOrPath;
#else
	return path + "/" + relativeFileOrPath;
#endif
}


// Handle Update Page
void CallAdmin::OnUpdate(wxString error, wxString result, int WXUNUSED(extra)) {
	// Log Action
	caLogAction("Retrieve information about the current version", LogLevel::LEVEL_DEBUG);

	wxString newVersion;

	if (result != "") {
		// Everything good :)
		if (error == "") {
			result.Trim();

			if (!result.StartsWith("{") || !result.EndsWith("}")) {
				// Maybe an Error Page?
				return caGetTaskBarIcon()->ShowMessage("Update Check Failed", "Error: Invalid Page Content", caGetMainFrame(), true);
			} else {
				// Find version in brackets
				newVersion = result.substr(1, result.length() - 2);
			}
		} else {
			// Log Action
			return caGetTaskBarIcon()->ShowMessage("Update Check Failed", "Error: " + error, caGetMainFrame(), true);
		}
	}

	// We got something
	if (newVersion != "") {
		// Check Version
		if (newVersion != CALLADMIN_CLIENT_VERSION) {
			// Update About Panel
			caGetAboutPanel()->EnableDownload(true);
			caGetAboutPanel()->UpdateVersionText(newVersion, wxColor("red"));

			caGetMainFrame()->Show(true);
			caGetMainFrame()->Iconize(false);

			// And raise it
			if (!isOtherInFullscreen()) {
				caGetMainFrame()->Raise();
			}

			// Goto About
			caGetNotebook()->GetWindow()->ChangeSelection(ABOUT_PANEL_PAGE);

			caGetTaskBarIcon()->ShowMessage("New Version available", "New version " + newVersion + " is now available!", caGetMainFrame(), false);
		} else {
			caGetTaskBarIcon()->ShowMessage("Up to Date", "Your CallAdmin Client is up to date", caGetMainFrame(), false);
		}
	}
}

