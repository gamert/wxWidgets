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
#include <wx/timer.h>

BEGIN_EVENT_TABLE(kuSingleScrolled,wxScrolledWindow)
    EVT_SIZE(kuSingleScrolled::OnSize)
	EVT_MOTION(kuSingleScrolled::OnMotion)
	EVT_LEFT_DOWN(kuSingleScrolled::OnLeftDown)
	EVT_LEFT_UP(kuSingleScrolled::OnLeftUp)
	EVT_KEY_DOWN(kuSingleScrolled::OnKeyDown)
	EVT_MOUSEWHEEL(kuSingleScrolled::OnMousewheel)
	EVT_CONTEXT_MENU(kuSingleScrolled::OnContextMenu)
	EVT_TIMER(kuID_TIMER_IDLE, kuSingleScrolled::OnIdleTimer)
END_EVENT_TABLE()

// -------- kuSingleScrolled --------
kuSingleScrolled::kuSingleScrolled(wxWindow* parent, kuFrame* frame)
     :wxScrolledWindow(parent,wxID_ANY), mIdleTimer(this, kuID_TIMER_IDLE) {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND));
    mFrame = frame;
    mOrigBmp = NULL;
    mLoadSize = wxSize(0,0);
    SetupPopupMenu();
}

kuSingleScrolled::~kuSingleScrolled() {
    if(mOrigBmp)    FreeImage_Unload(mOrigBmp);
};

void kuSingleScrolled::SetupPopupMenu() {
    /* don't delete it manually
    if(mMenu)   delete mMenu;
    */
    wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(kuID_SAVE,       STRING_MENU_SAVE);
	fileMenu->Append(kuID_SAVEAS,     STRING_MENU_SAVEAS);
	fileMenu->AppendSeparator();
	fileMenu->Append(kuID_PREVIEW,    STRING_MENU_PAGESETUP);
	fileMenu->Append(kuID_PREVIEW,    STRING_MENU_PREVIEW);
	fileMenu->Append(kuID_PRINT,      STRING_MENU_PRINT);
	fileMenu->Append(kuID_RELOAD,     STRING_MENU_RELOAD);
    wxMenu* editMenu = new wxMenu();
	editMenu->Append(kuID_ROTATE_CCW, STRING_MENU_ROTATE_CCW);
	editMenu->Append(kuID_ROTATE_CW,  STRING_MENU_ROTATE_CW);
	editMenu->Append(kuID_RESCALE,STRING_MENU_RESCALE);
	wxMenu* viewMenu = new wxMenu();
	viewMenu->Append(kuID_ZOOM_IN,    STRING_MENU_ZOOM_IN);
	viewMenu->Append(kuID_ZOOM_OUT,   STRING_MENU_ZOOM_OUT);
	viewMenu->Append(kuID_ZOOM_100,   STRING_MENU_ZOOM_100);
	viewMenu->Append(kuID_ZOOM_FIT,   STRING_MENU_ZOOM_FIT);
	viewMenu->Append(kuID_ZOOM_EXT,   STRING_MENU_ZOOM_EXT);
	if(mFrame->CanSetTransparent()) {
	    viewMenu->AppendSeparator();
	    viewMenu->Append(kuID_OPAQUE, STRING_MENU_OPAQUE);
	}
    mMenu = new wxMenu();
    mMenu->Append(kuID_SLIDESHOW,  STRING_MENU_SLIDESHOW_START);
    mMenu->AppendSeparator();
    mMenu->Append(kuID_PREV,   STRING_MENU_PREV);
	mMenu->Append(kuID_NEXT,   STRING_MENU_NEXT);
	mMenu->Append(kuID_HOME,   STRING_MENU_HOME);
	mMenu->Append(kuID_END,    STRING_MENU_END);
	mMenu->AppendSeparator();
	mMenu->Append(kuID_FILE,       STRING_MENU_FILE,     fileMenu);
	mMenu->Append(kuID_EDIT,       STRING_MENU_EDIT,     editMenu);
	mMenu->Append(kuID_VIEW,       STRING_MENU_VIEW,     viewMenu);
	mMenu->AppendSeparator();
	#ifdef __WXMSW__
	mMenu->Append(kuID_WALLPAPER,  STRING_MENU_DESKTOP_WALLPAPER);
	mMenu->Append(kuID_NONEWP,     STRING_MENU_DESKTOP_NONEWP);
	mMenu->Append(kuID_BACKGROUND, STRING_MENU_DESKTOP_BACKGROUND);
	mMenu->AppendSeparator();
	mMenu->Append(kuID_PROPERTIES, STRING_MENU_PROPERTIES);
	#endif
	mMenu->Append(kuID_METADATA,   STRING_MENU_METADATA);
	mMenu->AppendSeparator();
	mMenu->Append(wxID_EXIT,       STRING_MENU_EXIT);
}

void kuSingleScrolled::OnDraw(wxDC& dc) {
    if(mDispImg.Ok()) {
        mLeftTop = wxPoint(0,0);
        int width = mDispSize.x;
        int height = mDispSize.y;
        int xratio = width/GetSize().x;
        int yratio = height/GetSize().y;
        if(xratio<1)    mLeftTop.x = (GetSize().x-width)/2;
        if(yratio<1)    mLeftTop.y = (GetSize().y-height)/2;
        //wxMessageBox(wxString::Format(wxT("ClientSize: %dx%d"),GetSize().x,GetSize().y));
        //wxMessageBox(wxString::Format(wxT("mLeftTop: %dx%d"),mLeftTop.x,mLeftTop.y));
        mRightBottom = wxPoint(mLeftTop.x+width, mLeftTop.y+height);
        if(mFrame->IsFullScreen()) {
            dc.DrawBitmap(wxBitmap(mDispImg.GetSubImage(mView)),
                          mLeftTop.x, mLeftTop.y, true);
        } else {
            dc.DrawBitmap(wxBitmap(mDispImg),
                          mLeftTop.x, mLeftTop.y, true);
        }
	}
}

void kuSingleScrolled::ReloadImage(wxString filename, bool isurl) {
    mFilename = filename;
    mIsUrl = isurl;
	if(filename==wxEmptyString)   return;

	wxBeginBusyCursor();
	//wxStartTimer();
    if(mOrigBmp)    FreeImage_Unload(mOrigBmp);
    int size;
    if(wxGetApp().mOptions.mLoadCompletely)    size = 0;
    else    size = GetLoadSizeMax();
    //wxMessageBox(wxString::Format(wxT("size = %d"), size));
    mOrigBmp = wxGetApp().mLoader->GetFiBitmap(filename, isurl, size);

    mDispSize = mOrigSize = wxSize(FreeImage_GetWidth(mOrigBmp), FreeImage_GetHeight(mOrigBmp));
    //wxMessageBox(wxString::Format(wxT("mOrigSize: %dx%d"),mOrigSize.x,mOrigSize.y));
    //wxMessageBox(wxString::Format(wxT("GetSize: %dx%d"),GetSize().x,GetSize().y));
    // set edited to false when load completely or both of xy are less than window size (-1 for inaccuracy)
	if( size==0 || (mOrigSize.x < (GetSize().x-1) && mOrigSize.y < (GetSize().y-1)) )
	    wxGetApp().SetEdited(false);
	else    wxGetApp().SetEdited(true);
	if(mOrigBmp) {
		mScale = SCALE_BASE;
		mDragStart = wxPoint(0,0);
		// restore scale/rotate/lefttop
        if(wxGetApp().mOptions.mKeepScale)
            SetScale(SCALE_LASTUSED);
        else
            SetScale(SCALE_AUTOFIT);
        if(isurl)   mFrame->SetStatusText(filename.AfterLast(':'));
        else   mFrame->SetStatusText(filename);
        mFrame->SetStatusText(wxString::Format(wxT("%dx%d"),mOrigSize.x,mOrigSize.y),STATUS_FIELD_SIZE);
        if(wxGetApp().mOptions.mKeepRotate)     RestoreRotate(wxGetApp().mOptions.mRotate);
        else    wxGetApp().mOptions.mRotate = 0;
        if(wxGetApp().mOptions.mKeepLeftTop)    RestoreLeftTop(wxGetApp().mOptions.mLeftTop);
        else    wxGetApp().mOptions.mLeftTop = wxPoint(0,0);
        Refresh();
	}
	if(mFrame->mIsThumbnail) {
	    mFrame->mMultiple->Locate(filename);
	}
	//wxMessageBox(wxString::Format(wxT("time = %ld"), wxGetElapsedTime()));
	wxEndBusyCursor();
}

bool kuSingleScrolled::SaveImage(wxString filename, bool ask) {
    if(filename==wxEmptyString)    return false;
    if(mOrigBmp) {
        #ifdef __WXMSW__
        FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilenameU(filename.wc_str(wxConvFile));
        FREE_IMAGE_FORMAT src = FreeImage_GetFIFFromFilenameU(mFilename.wc_str(wxConvFile));
        #else
        FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename.mb_str(wxConvFile));
        FREE_IMAGE_FORMAT src = FreeImage_GetFIFFromFilename(mFilename.mb_str(wxConvFile));
        #endif
        // if both src and dst are jpg, may rotate/clone losslessly
        if( fif==FIF_JPEG && src==FIF_JPEG ) {
            if(wxMessageBox(STRING_LOSSLESS_MESSAGE,wxT("JPEG ")+mFrame->StripCodes(STRING_MENU_FILE),wxYES_NO|wxICON_QUESTION)==wxYES) {
                bool success = false;
                //wxMessageBox(wxString::Format(wxT("mRotate = %d"),wxGetApp().mOptions.mRotate));
                switch ((wxGetApp().mOptions.mRotate+4)%4) {
                    case 1:
                        success = FreeImage_JPEGTransform(mFilename.mb_str(wxConvFile), filename.mb_str(wxConvFile),
                                                          FIJPEG_OP_ROTATE_90,  TRUE);
                        break;
                    case 2:
                        success = FreeImage_JPEGTransform(mFilename.mb_str(wxConvFile), filename.mb_str(wxConvFile),
                                                          FIJPEG_OP_ROTATE_180, TRUE);
                        break;
                    case 3:
                        success = FreeImage_JPEGTransform(mFilename.mb_str(wxConvFile), filename.mb_str(wxConvFile),
                                                          FIJPEG_OP_ROTATE_270, TRUE);
                        break;
                    case 0:
                    default:
                        success = FreeImage_JPEGTransform(mFilename.mb_str(wxConvFile), filename.mb_str(wxConvFile),
                                                          FIJPEG_OP_NONE,       TRUE);
                        break;
                }
                if(success) {
                    wxGetApp().mOptions.mRotate=0;
                    mFrame->SetStatusText(STRING_INFO_SAVED+filename);
                    return true;
                }
            }
        }
        // it may fail when the filename doesn't match locale, just let it falls to original method...
        if(ask && wxMessageBox(STRING_SAVE_MESSAGE,mFrame->StripCodes(STRING_MENU_SAVE),wxOK|wxCANCEL|wxICON_EXCLAMATION)
           ==wxCANCEL) {
            mFrame->SetStatusText(STRING_INFO_CANCELED);
            return false;
        }
        if(fif!=FIF_UNKNOWN) {
            FreeImage_FlipVertical(mOrigBmp);
            int flags = 0;
            if(fif==FIF_JPEG)    flags = JPEG_QUALITYSUPERB;
            #ifdef __WXMSW__
            bool success = FreeImage_SaveU(fif,mOrigBmp,filename.wc_str(wxConvFile),flags);
            #else
            bool success = FreeImage_SaveU(fif,mOrigBmp,filename.c_str(),flags);
            #endif
            FreeImage_FlipVertical(mOrigBmp);
            if(success) {
                mFrame->SetStatusText(STRING_INFO_SAVED+filename);
                return true;
            }
        }
    } else if(ask && wxMessageBox(STRING_SAVE_MESSAGE,mFrame->StripCodes(STRING_MENU_SAVE),wxOK|wxCANCEL|wxICON_EXCLAMATION)
       ==wxCANCEL) {
        mFrame->SetStatusText(STRING_INFO_CANCELED);
    }
    return false;
}

wxString kuSingleScrolled::GetFilename() {
    return mFilename;
}

/* since cannot know if the width of bmp is larger than height,
   don't know should use width or height of the window area to fit it.
   use previous value, but still be wrong when current pic is x>y and next pic is x<y
*/
int kuSingleScrolled::GetLoadSizeMax() {
    //wxMessageBox(wxString::Format(wxT("LoadSize = (%d, %d), GetSize = (%d, %d)"), mLoadSize.x, mLoadSize.y, GetSize().x, GetSize().y));
    float xratio = (float)mLoadSize.x / GetSize().x;
    float yratio = (float)mLoadSize.y / GetSize().y;
    //wxMessageBox(wxString::Format(wxT("ratio = (%f, %f)"), xratio, yratio));
    if( (mLoadSize.x==GetSize().x && mLoadSize.y==GetSize().y)    // just onSize
       || (mLoadSize.x<GetSize().x-1 && mLoadSize.y<GetSize().y-1) )    // if last pic is smaller than window area
        return wxMax(GetSize().x, GetSize().y);   // load using max of window size
    // assume the last fit window, one of xratio/yratio is ~1
    if(xratio > yratio) {    // x fit
        if(mLoadSize.x > mLoadSize.y)    return GetSize().x;    // x fit and x is larger than y
        else    return mLoadSize.y;
    } else {    // y fit
        if(mLoadSize.y > mLoadSize.x)    return GetSize().y;    // y fit and y is larger than x
        else    return mLoadSize.x;
    }
}

wxSize kuSingleScrolled::GetOrigSize() {
    return mOrigSize;
}

FIBITMAP* kuSingleScrolled::GetOrigBmp() {
    if(mOrigBmp) {
        /*
        if(!wxGetApp().mOptions.mLoadCompletely) {
            wxGetApp().mOptions.mLoadCompletely = true;
            ReloadImage(mFilename,false);
            wxGetApp().mOptions.mLoadCompletely = false;
        }
        */
        return mOrigBmp;
    }
    return NULL;
}

void kuSingleScrolled::Rotate90(bool cw) {
    if(!mOrigBmp)    return;
    FIBITMAP* dst = NULL;
#if false
    if(cw)    dst = FreeImage_RotateClassic(mOrigBmp,90);
    else    dst = FreeImage_RotateClassic(mOrigBmp,-90);
    if(dst) {
        wxGetApp().mLoader->Replace(mFilename, dst);
        mOrigBmp = dst;
        mOrigSize = wxSize(FreeImage_GetWidth(mOrigBmp), FreeImage_GetHeight(mOrigBmp));
    }
	SetScale(SCALE_LASTUSED);
	mDispSize = wxSize(mDispImg.GetWidth(),mDispImg.GetHeight());
	mLeftTop = wxPoint(0,0);
	CheckViewSize();
	CheckViewStart();
    wxGetApp().SetEdited(true);
#endif
}

void kuSingleScrolled::OnSize(wxSizeEvent& event) {
	Refresh();
	//wxMessageBox(wxString::Format(wxT("onSize: %d x %d"), GetSize().x, GetSize().y));
	// force load completely when window area are resized
    mLoadSize = GetSize();
}

void kuSingleScrolled::OnMotion(wxMouseEvent& event) {
    mIdleTimer.Stop();
    if(!event.LeftIsDown())    HideCursor(false);
	if(mDispImg.Ok()) {
		if(event.GetX()>mLeftTop.x && event.GetX()<mRightBottom.x
	       && event.GetY()>mLeftTop.y && event.GetY()<mRightBottom.y) {
	       mFrame->SetStatusText(wxString::Format(wxT("(%d,%d)"),
		                                          event.GetX()-mLeftTop.x,
		                                          event.GetY()-mLeftTop.y),STATUS_FIELD_MOTION);
        }
        else   mFrame->SetStatusText(wxEmptyString,STATUS_FIELD_MOTION);
	}
	if(mFrame->IsFullScreen())    mIdleTimer.Start(3000, true);
}

void kuSingleScrolled::OnLeftDown(wxMouseEvent& event) {
    if(event.ControlDown()) {
        #ifdef __WXMSW__
        SetCursor(wxCursor(wxT("CURSOR_MAGNIFIER"), wxBITMAP_TYPE_CUR_RESOURCE));
        #else
        SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
        #endif
    } else    SetCursor(wxCursor(wxCURSOR_HAND));
	mDragStart=event.GetPosition();
	event.Skip();
}

void kuSingleScrolled::OnLeftUp(wxMouseEvent& event) {
	if(mFrame->IsFullScreen()) {
		wxRect view=mView;
		mView.x+=mDragStart.x-event.GetX();
		mView.y+=mDragStart.y-event.GetY();
		CheckViewStart();
		if(mView!=view)   Refresh();
		// keep lefttop
        wxGetApp().mOptions.mLeftTop=wxPoint(mView.x,mView.y);
	}
	else {
	    wxPoint start;
        GetViewStart(&start.x,&start.y);
        //wxMessageBox(wxString::Format(wxT("start=(%d,%$d)"),start.x,start.y));
	    if(event.ControlDown()) {
	        int width = abs(mDragStart.x-event.GetX());
	        int height = abs(mDragStart.y-event.GetY());
            /* draw scale area */
	        wxClientDC dc(this);
            dc.SetPen(*wxRED_PEN);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(mDragStart.x, mDragStart.y, width, height);
            /* calculate scale */
            int fratio;
            int xratio=width*SCALE_BASE/GetClientSize().x;
            int yratio=height*SCALE_BASE/GetClientSize().y;
            fratio = wxMax(xratio, yratio);
            float final=mScale*SCALE_BASE/fratio;
            start.x = (int)((start.x+mDragStart.x-mLeftTop.x)*final/SCALE_BASE);
            start.y = (int)((start.y+mDragStart.y-mLeftTop.y)*final/SCALE_BASE);
            SetScale((int)(final-mScale));
	    } else {
            start.x+=mDragStart.x-event.GetX();
            start.y+=mDragStart.y-event.GetY();
	    }
	    Scroll(start.x,start.y);
        // keep lefttop
        wxGetApp().mOptions.mLeftTop=start;
	}
	//wxMessageBox(wxString::Format(wxT("start=(%d,%$d)"),wxGetApp().mOptions.mViewStart.x,wxGetApp().mOptions.mViewStart.y));
	SetCursor(wxCursor(wxCURSOR_DEFAULT));
}

void kuSingleScrolled::CheckViewSize() {
	int width  = mDispSize.x;
	int height = mDispSize.y;
	if(mFrame->IsFullScreen())   SetScrollRate(0,0);
	else   SetScrollbars(SCROLL_RATE_SINGLE,SCROLL_RATE_SINGLE,width,height);
    mView=wxRect(0,0,width,height);
    if(width>GetClientSize().x)   mView.SetWidth(GetClientSize().x);
    if(height>GetClientSize().y)   mView.SetHeight(GetClientSize().y);
    //wxMessageBox(wxString::Format(wxT("%d,%d,%d,%d"),mView.x,mView.y,mView.GetWidth(),mView.GetHeight()));
}

void kuSingleScrolled::CheckViewStart() {
	if(mView.x<0)   mView.x=0;
    if(mView.y<0)   mView.y=0;
    int diff;
    diff = mDispSize.x-GetClientSize().x;
    if(diff<0)   mView.x = 0;
    else if(mView.x>diff)   mView.x = diff;
    diff = mDispSize.y-GetClientSize().y;
    if(diff<0)   mView.y = 0;
    else if(mView.y>diff)   mView.y = diff;
}

void kuSingleScrolled::RestoreRotate(int rotate) {
    //wxMessageBox(wxString::Format(wxT("rotate=%d"),rotate));
    while(true) {
        if(rotate==0)   break;
        else if(rotate>0) {
            Rotate90(true);
            rotate--;
        }
        else {
            Rotate90(false);
            rotate++;
        }
    }
}

void kuSingleScrolled::RestoreLeftTop(wxPoint lefttop) {
    if(mFrame->IsFullScreen()) {
        wxRect view = mView;
        mView.x = lefttop.x;
        mView.y = lefttop.y;
        CheckViewStart();
        if(mView!=view)   Refresh();
    } else {
        Scroll(lefttop.x,lefttop.y);
    }
}

void kuSingleScrolled::SetFullScreen(bool full) {
    mFrame->GetSizer()->Detach(mFrame->mTopSplitter);
    if(full) {   // switch to fullscreen
        mFrame->GetSizer()->Add(mFrame->mTopSplitter,1,wxEXPAND|wxALL,0);
        mFrame->ShowFullScreen(true);
        if(mFrame->mIsFilesystem)   mFrame->mTopSplitter->Unsplit(mFrame->mDirSplitter);
        if(mFrame->mIsThumbnail)   mFrame->mViewSplitter->Unsplit(mFrame->mMultiple);
        SetBackgroundColour(*wxBLACK);
        if(mFrame->CanSetTransparent())    mFrame->SetTransparent(wxGetApp().mOptions.mOpaque);
    }
    else {
        if(mFrame->CanSetTransparent()) {
            mFrame->SetTransparent(255);
            if(CanSetTransparent())    SetTransparent(wxGetApp().mOptions.mOpaque);
        }
        mFrame->GetSizer()->Add(mFrame->mTopSplitter,1,wxEXPAND|wxALL,2);
        if(wxGetApp().mInit==kuApp::kuID_INIT_FULL) {
            mFrame->Show(false);
            mFrame->ShowFullScreen(false);
            mFrame->Maximize(true);
            mFrame->SetupToolBar();
            mFrame->Show(true);
            //SetScale(SCALE_AUTOFIT);
            //wxSafeYield();
            wxGetApp().mInit = kuApp::kuID_INIT_SHOW;
        } else    mFrame->ShowFullScreen(false);
        if(mFrame->mIsFilesystem)   mFrame->mTopSplitter->SplitVertically(mFrame->mDirSplitter,mFrame->mViewSplitter,SPLITTER_TOP_WIDTH);
        if(mFrame->mIsThumbnail)   mFrame->mViewSplitter->SplitHorizontally(mFrame->mSingle,mFrame->mMultiple,-SPLITTER_VIEW_HEIGHT);
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND));
    }
    HideCursor(full);
    if(mFrame->CanSetTransparent()) {
        mFrame->GetMenuBar()->Enable(kuID_OPAQUE,full||CanSetTransparent());
        mMenu->Enable(kuID_OPAQUE,full||CanSetTransparent());
    }
    SetScale(SCALE_LASTUSED);
    RestoreLeftTop(mLeftTop);
}

bool kuSingleScrolled::SetScale(int diff) {
    //wxMessageBox(wxString::Format(wxT("diff=%d"),diff));
    // check if it is invalid
    if(!mOrigBmp)    return false;
    // best fit
    int width  = mOrigSize.x;
    int height = mOrigSize.y;
    switch (diff) {
        case SCALE_AUTOFIT: {
            int fratio;
            int xratio=width*SCALE_BASE/GetClientSize().x;
            int yratio=height*SCALE_BASE/GetClientSize().y;
            //wxMessageBox(wxString::Format(wxT("%d,%d,%d,%d"),xratio,yratio,GetClientSize().x,GetClientSize().y));
            fratio = wxMax(xratio, yratio);
            fratio>SCALE_BASE ? mScale=SCALE_BASE*SCALE_BASE/fratio : mScale=SCALE_BASE;
            break;
        }
        case SCALE_BESTFIT: {
            int fratio;
            int xratio=width*SCALE_BASE/GetClientSize().x;
            int yratio=height*SCALE_BASE/GetClientSize().y;
            //wxMessageBox(wxString::Format(wxT("%d,%d,%d,%d,%d,%d"),mImage.GetWidth(),mImage.GetHeight(),xratio,yratio,GetClientSize().x,GetClientSize().y));
            fratio = wxMax(xratio, yratio);
            mScale=SCALE_BASE*SCALE_BASE/fratio;
            break;
        }
        case SCALE_EXTEND: {
            int fratio;
            int xratio=width*SCALE_BASE/GetClientSize().x;
            int yratio=height*SCALE_BASE/GetClientSize().y;
            //wxMessageBox(wxString::Format(wxT("%d,%d,%d,%d,%d,%d"),mImage.GetWidth(),mImage.GetHeight(),xratio,yratio,GetClientSize().x,GetClientSize().y));
            fratio = wxMin(xratio, yratio);
            mScale=SCALE_BASE*SCALE_BASE/fratio;
            break;
        }
        case SCALE_ORIGINAL:
            mScale=SCALE_BASE;
            break;
        case SCALE_LASTUSED:
            mScale=wxGetApp().mOptions.mScale;
            break;
        default:
            mScale+=diff;
    }
    wxBeginBusyCursor();
    //wxMessageBox(wxString::Format(wxT("before %d"), wxMax(mDispSize.x, mDispSize.y)));
    mDispSize = wxSize((int)(width*mScale/SCALE_BASE),(int)(height*mScale/SCALE_BASE));
    //wxMessageBox(wxString::Format(wxT("after %d"), wxMax(mDispSize.x, mDispSize.y)));

    // set mLoadSize for guess max width/height of bmp to load
    mLoadSize = mDispSize;

    FIBITMAP* dispBmp;
    if(mScale==SCALE_BASE)    dispBmp = FreeImage_Clone(mOrigBmp);
    else {
        //wxMessageBox(wxString::Format(wxT("mScale=%f"),mScale));
        dispBmp = FreeImage_Rescale(mOrigBmp, mDispSize.x, mDispSize.y, wxGetApp().mOptions.mFilter);
    }
    kuFiWrapper::FiBitmap2WxImage(dispBmp,&mDispImg);
    FreeImage_Unload(dispBmp);
    wxEndBusyCursor();
    //wxMessageBox(wxString::Format(wxT("mScale=%f, mDispSize:%dx%d"),mScale,mDispSize.x,mDispSize.y));

    // show current scale and size
    mFrame->SetStatusText(wxString::Format(wxT("%3.0f%%"),floor(mScale*100/SCALE_BASE+0.1)),STATUS_FIELD_SCALE);
    // keep scale
    wxGetApp().mOptions.mScale = mScale;

	CheckViewSize();
	CheckViewStart();
	RestoreLeftTop(mLeftTop);
	Refresh();
	return true;
}

bool kuSingleScrolled::Rescale(double scale) {
    int width = (int)(mOrigSize.x * scale);
    int height = (int)(mOrigSize.y * scale);
    FIBITMAP* bmp = FreeImage_Rescale(mOrigBmp, width, height, wxGetApp().mOptions.mFilter);
    if(!bmp)    return false;
    wxGetApp().mLoader->Replace(mFilename, bmp);
    if(mOrigBmp)    FreeImage_Unload(mOrigBmp);
    mOrigBmp = bmp;
    mDispSize = mOrigSize = wxSize(FreeImage_GetWidth(mOrigBmp), FreeImage_GetHeight(mOrigBmp));
    wxGetApp().SetEdited(true);
    SetScale(SCALE_ORIGINAL);
    return true;
}

void kuSingleScrolled::OnKeyDown(wxKeyEvent& event) {
	//wxMessageBox(wxString::Format(wxT("%d"),event.GetKeyCode()));
	if(!KeyAction(event.GetKeyCode()))   event.Skip();
}

bool kuSingleScrolled::KeyAction(int keycode) {
    //wxMessageBox(wxString::Format(wxT("%d"),keycode));
    switch (keycode) {
        case WXK_PAGEUP:
            mFrame->Action(kuID_PREV);
            break;
        case WXK_PAGEDOWN:
            mFrame->Action(kuID_NEXT);
            break;
        case WXK_HOME:
            mFrame->Action(kuID_HOME);
            break;
        case WXK_END:
            mFrame->Action(kuID_END);
            break;
        default:
            return false;
    }
    return true;
}

void kuSingleScrolled::OnMousewheel(wxMouseEvent& event) {
    if(event.ControlDown()) {   // zoom
        wxPoint lt, real;
        GetViewStart(&lt.x, &lt.y);
        real.x = (int)((lt.x+event.GetX())*(mScale+SCALE_DIFF)/mScale);
        real.y = (int)((lt.y+event.GetY())*(mScale+SCALE_DIFF)/mScale);
        //wxMessageBox(wxString::Format(wxT("%d,%d"),real.x, real.y));
        real.x -= event.GetX();
        real.y -= event.GetY();
        //wxMessageBox(wxString::Format(wxT("%d,%d"),real.x, real.y));
        mLeftTop = real;
        if(event.GetWheelRotation()>0)   mFrame->Action(kuID_ZOOM_IN);
        else   mFrame->Action(kuID_ZOOM_OUT);
    } else {   // next or prev
        if(event.GetWheelRotation()>0)   mFrame->Action(kuID_PREV);
        else   mFrame->Action(kuID_NEXT);
    }
}

void kuSingleScrolled::OnContextMenu(wxContextMenuEvent& event) {
    PopupMenu(mMenu);
}

void kuSingleScrolled::OnIdleTimer(wxTimerEvent& event) {
    HideCursor();
}

void kuSingleScrolled::HideCursor(bool hide) {
    if(hide) {
        #ifdef __WXMSW__
        SetCursor(wxCursor(wxT("CURSOR_BLANK"), wxBITMAP_TYPE_CUR_RESOURCE));
        #else
        SetCursor(wxCursor(wxCURSOR_BLANK));
        #endif
    } else {
        SetCursor(wxCursor(wxCURSOR_DEFAULT));
    }
}
