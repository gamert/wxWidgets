// Copyright (C) 2005  Augustino (augustino@users.sourceforge.net)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "ku.h"

kuCheckListDialog::kuCheckListDialog(wxWindow* parent, kuFrame* frame, const wxString& message, const wxString& caption, const wxArrayString& choices)
    :wxDialog(parent,wxID_ANY,caption) {
	mFrame=frame;
    wxStaticBoxSizer* topSizer = new wxStaticBoxSizer(wxVERTICAL,this);
    // message
    topSizer->Add(new wxStaticText(this,wxID_ANY,message),0,wxALIGN_CENTER);
    // choices
    mListBox = new wxCheckListBox(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,choices);
    topSizer->Add(mListBox,1,wxEXPAND|wxALL,10);
    topSizer->Add(new wxStaticLine(this),0,wxEXPAND);
    // buttons
    wxSizer* btnSizer = CreateButtonSizer(wxOK|wxCANCEL);
    topSizer->Add(btnSizer,0,wxEXPAND|wxTOP,10);
    SetSizer(topSizer);
    SetIcon(mFrame->mIconApp);
    Fit();
}

wxArrayInt kuCheckListDialog::GetSelections() {
	wxArrayInt selections;
    for(unsigned int i=0;i<mListBox->GetCount();i++) {
        if(mListBox->IsChecked(i))   selections.Add(i);
	}
	return selections;
}

void kuCheckListDialog::SetSelections(const wxArrayInt& selections) {
	for(size_t i=0;i<selections.GetCount();i++) {
        mListBox->Check(selections[i]);
	}
}
