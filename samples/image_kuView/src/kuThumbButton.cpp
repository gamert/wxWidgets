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

BEGIN_EVENT_TABLE(kuThumbButton,wxBitmapButton)
	EVT_BUTTON(wxID_ANY,kuThumbButton::OnButton)
	EVT_MOUSE_EVENTS(kuThumbButton::OnMouseEvents)
	EVT_KEY_DOWN(kuThumbButton::OnKeyDown)
END_EVENT_TABLE()

// -------- kuThumbButton --------
kuThumbButton::kuThumbButton(wxWindow* parent, wxString filename, wxString url)
    :wxBitmapButton(parent,wxID_ANY,*(kuFiWrapper::GetWxImage(filename,url==wxEmptyString ,SCALE_THUMBNAIL)),
                    wxDefaultPosition,wxSize(0,0),
                    wxBU_AUTODRAW,wxDefaultValidator,filename) {
    //wxMessageBox(filename);
    bool isurl = url==wxEmptyString ? false : true;
    /*
    wxImage* image = kuFiWrapper::GetWxImage(filename,isurl,SCALE_THUMBNAIL);
    SetBitmapLabel(*image);
    */
    SetSizeHints(GetBestSize());
    if(isurl)    SetName(url);
    else    SetName(filename);
    mIsUrl = isurl;
}
/*
kuThumbButton::kuThumbButton(wxWindow* parent, wxInputStream& stream, wxString name)
    :wxBitmapButton(parent,wxID_ANY,wxImage(),wxDefaultPosition,wxSize(0,0)) {
    wxImage image(stream);
    int ratio=image.GetWidth()*SCALE_BASE/image.GetHeight();
    if(ratio>SCALE_BASE)
        SetBitmapLabel(image.Scale(SCALE_THUMBNAIL,SCALE_THUMBNAIL*SCALE_BASE/ratio));
    else
        SetBitmapLabel(image.Scale(SCALE_THUMBNAIL*ratio/SCALE_BASE,SCALE_THUMBNAIL));
    SetSizeHints(GetBestSize());
    SetName(name);
    mIsUrl=true;
}
*/
void kuThumbButton::OnButton(wxCommandEvent& event) {
	if(mIsUrl)   // in fact, it is filename only
        wxGetApp().mFrame->mVirtual->Locate(GetName());
	else   wxGetApp().mFrame->mGeneric->Locate(GetName());
    wxGetApp().mFrame->mMultiple->Locate(GetName());
}

void kuThumbButton::OnMouseEvents(wxMouseEvent& event) {
    this->GetParent()->GetEventHandler()->AddPendingEvent(event);
    event.Skip();
}

void kuThumbButton::OnKeyDown(wxKeyEvent& event) {
    this->GetParent()->GetEventHandler()->AddPendingEvent(event);
    event.Skip();
}
