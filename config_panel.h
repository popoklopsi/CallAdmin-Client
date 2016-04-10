#ifndef CONFIG_PANEL_H
#define CONFIG_PANEL_H

/**
 * -----------------------------------------------------
 * File        config_panel.h
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
#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <wx/filepicker.h>
#include "panel.h"

#define CONFIG_PANEL_PAGE 2
#define CONFIG_PANEL_LABEL "Settings"


class ConfigPanel : public Panel, public wxPanel {
private:
	wxTextCtrl *pageText;
	wxTextCtrl *keyText;
	wxSlider *stepSlider;
	wxStaticText *stepSliderValue;
	wxSlider *timeoutSlider;
	wxStaticText *timeoutSliderValue;
	wxSlider *attemptsSlider;
	wxStaticText *attemptsSliderValue;
	wxSlider *callsSlider;
	wxStaticText *callsSliderValue;
	wxFilePickerCtrl *soundFilePicker;
	wxCheckBox *steamEnable;
	wxCheckBox *showAsNotification;
	wxCheckBox *hideMinimize;
	wxCheckBox *minimizeExit;

public:
	ConfigPanel();

	// Load controls
	bool InitPanel();

	int GetPage() {
		return CONFIG_PANEL_PAGE;
	}

	wxString GetLabel() {
		return CONFIG_PANEL_LABEL;
	}

	wxPanel *GetPanel() {
		return this;
	}

	// Parse the config, set the panel values and restart progress
	void ParseConfig();

protected:
	void OnSliderChanged(wxCommandEvent &event);
	void OnSoundFileChanged(wxFileDirPickerEvent &event);
	void OnSoundFileDefault(wxCommandEvent &event);
	void OnSteamUpdate(wxCommandEvent &event);
	void OnShowAsNotificationUpdate(wxCommandEvent &event);
	void OnHideUpdate(wxCommandEvent &event);
	void OnSet(wxCommandEvent &event);

	wxDECLARE_EVENT_TABLE();
};


#endif
