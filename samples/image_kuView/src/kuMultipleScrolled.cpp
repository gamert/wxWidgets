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

BEGIN_EVENT_TABLE(kuMultipleScrolled,wxScrolledWindow)
    EVT_KEY_DOWN(kuMultipleScrolled::OnKeyDown)
    EVT_SIZE(kuMultipleScrolled::OnSize)
END_EVENT_TABLE()

// -------- kuMultipleScrolled --------
kuMultipleScrolled::kuMultipleScrolled(wxWindow* parent, kuFrame* frame)
     :wxScrolledWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxVSCROLL|wxSUNKEN_BORDER) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
    mFrame=frame;
    SetSizer(new wxGridSizer(0));
}

void kuMultipleScrolled::ReloadThumbs(wxString dirname, bool isurl) {
    if(dirname == mDirname)    return;
    mDirname = dirname;
    //wxTreeCtrl* tree=mFrame->mGeneric->GetTreeCtrl();
    //if(tree->GetItemParent(tree->GetSelection())==tree->GetRootItem())   return;
    wxArrayString* children;
    if(isurl)    children = mFrame->mVirtual->EnumerateChildren(dirname);
    else    children = mFrame->mGeneric->EnumerateChildren(dirname);
    if(children==NULL)   return;
    // delete old
    DestroyChildren();
    // add new
    int cols=GetSize().x/(SCALE_THUMBNAIL+10);
    ((wxGridSizer*)GetSizer())->SetCols(cols);
    wxString dir;
    if(isurl)    dir = mFrame->mVirtual->GetFilePath(true,false)+wxFileName::GetPathSeparator();
    else {
        dir=mFrame->mGeneric->GetPath();
        if(dir.Last()!=wxFileName::GetPathSeparator())   dir=dir+wxFileName::GetPathSeparator();
    }
    wxGetApp().SetBusy(true);
    mFrame->SetStatusText(STRING_INFO_THUMBS);
    mFrame->mStatusBar->SetGaugeRange(children->GetCount());
    for(size_t i=0;i<children->GetCount();i++) {
        wxBeginBusyCursor();
        if(isurl)    GetSizer()->Add(new kuThumbButton(this,dir+children->Item(i),children->Item(i)));
        else    GetSizer()->Add(new kuThumbButton(this,dir+children->Item(i),wxEmptyString));
        FitInside();
        mFrame->mStatusBar->IncrGaugeValue();
        wxGetApp().Yield();
        wxEndBusyCursor();
        // check interrupt
        if(wxGetApp().GetInterrupt())   break;
    }
    SetScrollRate(0,SCROLL_RATE_MULTIPLE);

	if(wxGetApp().GetInterrupt()) {
            if(wxGetApp().mQuit)    return;
            mFrame->SetStatusText(STRING_WARNING_INTERRUPTED);
            wxGetApp().SetInterrupt(false);
	} else   mFrame->SetStatusText(wxEmptyString);
	wxGetApp().SetBusy(false);
}

void kuMultipleScrolled::RemoveThumb(wxString filename, bool isurl) {
    if(isurl)   return;   // not support in archive
    GetSizer()->Detach(FindWindow(filename));
}

void kuMultipleScrolled::Locate(wxString filename) {
    wxWindow* window;
    if(mFrame->mIsArchive)   // filename is url
        filename=filename.AfterLast(wxFileName::GetPathSeparator());
    for(size_t i=0;i<GetChildren().GetCount();i++) {
        window=GetChildren().Item(i)->GetData();
        if(window->GetName()==filename) {
            window->SetWindowStyle(wxNO_BORDER);
            window->Refresh();
            /*
            // scroll to start
            wxPoint position=window->GetPosition();
            wxPoint start;
            GetViewStart(&start.x,&start.y);
            Scroll(0,position.y/SCROLL_RATE_MULTIPLE+start.y);
            position=window->GetPosition();
            GetViewStart(&start.x,&start.y);
            // adjust
            if(position.y<0)   Scroll(0,start.y-1);
            */
        }
        else
            window->SetWindowStyle(wxBU_AUTODRAW);
            window->Refresh();
	}
}

void kuMultipleScrolled::OnKeyDown(wxKeyEvent& event) {
	//wxMessageBox(wxString::Format(wxT("%d"),event.GetKeyCode()));
	event.Skip();
}

void kuMultipleScrolled::OnSize(wxSizeEvent& event) {
	int cols=GetSize().x/(SCALE_THUMBNAIL+10);
	((wxGridSizer*)GetSizer())->SetCols(cols);
	FitInside();
    SetScrollRate(0,SCROLL_RATE_MULTIPLE);
}
