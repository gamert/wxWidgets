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

// -------- implement --------

IMPLEMENT_APP(kuApp)


// -------- kuApp --------
bool kuApp::OnInit() {
    // initialize FreeImage
    kuFiWrapper::Initialize();

	// init archive handlers
	wxFileSystem::AddHandler(new wxArchiveFSHandler);
	wxFileSystem::AddHandler(new wxFilterFSHandler);
    // init language
    mLocale = NULL;
    mFrame = NULL;
	mPath=wxString(*(TCHAR **)argv).BeforeLast(wxFileName::GetPathSeparator());
		  //+wxFileName::GetPathSeparator();

    // parse argv
    if(!ParseArguments(argc, argv)) {
        PrintCmdHelp();
        return false;
    }
    mWaitLoader = false;
    mQuit = false;
    mIsDoingCmd = false;

    mIsInterrupted = false;
    mIsBusy = false;
    mIsEdited = false;
    mIsSaveAs = false;

    SwitchLocale(mPath,wxLANGUAGE_DEFAULT);

    // set options
    SetOptions();

    // create mFrame
    mFrame = new kuFrame();
    mFrame->SetSize(724,500);
    mFrame->SetDefaults();
    // prepare for registry
    mFrame->mAssociationExecute=wxString::Format(wxT("\"%s\""), *(TCHAR **)argv);
    mFrame->mShellExecute=wxString::Format(wxT("\"%s\" \"%%1\""), *(TCHAR **)argv);


    // window state and init cmd
    switch(mInit) {
        case kuID_INIT_SHOW:
            mFrame->SetupWindows(true,false);
            mFrame->Maximize(true);
            mFrame->Show(true);
            break;
        case kuID_INIT_HIDE:
            mFrame->Show(false);
            break;
        case kuID_INIT_FULL:
            mFrame->mSingle->SetFullScreen(true);
            break;
        case kuID_INIT_ICON:
            mFrame->Action(kuID_MINIMIZE);
            break;
        default:
            mFrame->SetupWindows(true,false);
            mFrame->Maximize(true);
            mFrame->Show(true);
            break;
    }

    // run loader thread
    mLoader = new kuLoadThread(mFrame);
    if(mLoader->Create() != wxTHREAD_NO_ERROR)    wxMessageBox(wxT("failed to create thread"));
    else {
        mLoader->SetPriority(30);
        mLoader->Run();
    }

    // run action cmd
    for(size_t i=0; i<mCmds.Count(); i++) {
        DoActionCmd(mCmds[i]);
        // test if quit when doing command
        if(mQuit)    return false;
    }

    return true;
}

int kuApp::OnExit() {
    // finalize FreeImage
    kuFiWrapper::Finalize();
    return wxApp::OnExit();
}

void kuApp::PrintCmdHelp() {
    wxString sep = wxT("\n");
    wxString msg = STRING_VERSION + sep + STRING_PROJECT + sep + STRING_AUTHOR + sep + sep
                   + wxT("Usage:") + sep
                   + wxT("kuview [init_state] [command [args]]") + sep
                   + wxT("\tinit_state = show | hide | full | icon") + sep
                   + wxT("\tcommand [args] = locate [dir|file] | slideshow [interval] [once] [dirs]") + sep + sep
                   + wxT("example 1: start with fullscreen, and play slideshow with interval=5") + sep
                   + wxT("> kuview full slideshow 5 dir1 dir_2 \"dir 3\"") + sep
                   + wxT("example 2: start normally, locate file, and play slideshow") + sep
                   + wxT("> kuview locate file slideshow") + sep
                   + wxT("example 3: start normally, and play slideshow from file which is in dir1 ") + sep
                   + wxT("> kuview locate file slideshow dir1 dir2") + sep
                   + wxT("example 4: start normally, play slideshow without repeat, and locate file") + sep
                   + wxT("> kuview slideshow once dir1 dir2 locate file") + sep;
    wxMessageBox(msg, STRING_APPNAME, wxOK|wxICON_INFORMATION);
}

bool kuApp::ParseArguments(int argc, wxChar** argv) {
    if(argc<=1)    // no argv
        return true;

    wxChar** aptr = argv;
    aptr++;
    wxString first(*aptr);
    if(first.CmpNoCase(wxT("/?"))==0 || first.CmpNoCase(wxT("-h"))==0)    // help
        return false;

    // for init command
    int start = 1;
    if(DoInitCmd(*aptr)) {
        aptr++;
        start = 2;
    }

    mCmds.Clear();
    wxString cmd = wxEmptyString;
    for(int i=start; i<argc; i++) {
        if(IsActionCmd(*aptr)) {
            if(cmd!=wxEmptyString) {
                mCmds.Add(cmd);
                cmd.Clear();
            }
            cmd.Append(wxString(*aptr));
        } else if(IsPathCmd(*aptr)) {    // path arg
            wxFileName path(*aptr);
            if(path.IsRelative())    path.MakeAbsolute();
            if(cmd.CmpNoCase(CMD_LOCATE)==0) {    // locate command
                cmd.Append(CMD_SEP + path.GetFullPath());
            } else if(cmd.BeforeFirst(CMD_SEP).CmpNoCase(CMD_SLIDESHOW)==0) {    // slideshow command
                if(wxDirExists(path.GetFullPath()))    // path.IsDir() doesn't really check it
                    cmd.Append(CMD_SEP + path.GetFullPath());
                else
                    cmd.Append(CMD_SEP + path.GetPath());
            } else {    // add locate for it
                if(cmd!=wxEmptyString) {
                    mCmds.Add(cmd);
                    cmd.Clear();
                }
                cmd.Append(CMD_LOCATE);
                cmd.Append(CMD_SEP + path.GetFullPath());
            }
        } else {    // normal arg
            cmd.Append(CMD_SEP + wxString(*aptr));
        }
        aptr++;
    }
    if(cmd!=wxEmptyString)    mCmds.Add(cmd);
    //for(size_t i=0; i<mCmds.Count(); i++)    wxMessageBox(mCmds[i]);
    return true;
}

bool kuApp::DoInitCmd(wxChar* cmd) {
    wxArrayString enums;
    enums.Add(wxT("show"));
    enums.Add(wxT("hide"));
    enums.Add(wxT("full"));
    enums.Add(wxT("icon"));
    int index = enums.Index(cmd, false);
    if(index==wxNOT_FOUND) {
        mInit = kuID_INIT_SHOW;
        return false;
    }
    mInit = index;
    return true;
}

bool kuApp::IsPathCmd(wxChar* cmd) {
    wxString str(cmd);
    if(wxFileExists(str) || wxDirExists(str))
        return true;
    return false;
}

bool kuApp::IsActionCmd(wxChar* cmd) {
    wxArrayString enums;
    enums.Add(CMD_LOCATE);
    enums.Add(CMD_SLIDESHOW);
    if(enums.Index(cmd, false)==wxNOT_FOUND)
        return false;
    return true;
}

bool kuApp::DoActionCmd(wxString& cmd) {
    mIsDoingCmd = true;
    bool ret = false;
    wxStringTokenizer tkz(cmd, wxT("|"));
    wxArrayString tokens;
    while(tkz.HasMoreTokens())
        tokens.Add(tkz.GetNextToken());

    //for(size_t i=0; i<tokens.Count(); i++)    wxMessageBox(tokens[i]);

    if(tokens[0].CmpNoCase(CMD_LOCATE)==0) {
        if(tokens.GetCount()<2)    return false;
        ret = mFrame->Action(kuID_LOCATE, tokens[1]);
    } else if(tokens[0].CmpNoCase(CMD_SLIDESHOW)==0) {
        long interval;
        size_t start=1;
        if(tokens.Count()>start && tokens[start].IsNumber()) {
            if(tokens[start].ToLong(&interval))
                wxGetApp().mOptions.mSlideInterval = interval;
            start += 1;
        }
        if(tokens.Count()>start && tokens[start].CmpNoCase(wxT("once"))==0) {
            wxGetApp().mOptions.mSlideshowRepeat = false;
            start += 1;
        }
        for(size_t i=start; i<tokens.Count(); i++) {
            mFrame->mSlideshowDirs.Add(tokens[i]);
        }
        // let kuID_SLIDESHOW handle it
        //if(!mFrame->mSlideshowDirs.IsEmpty()) {
        //    mFrame->mSlideshowIndex = 0;
        //    mFrame->mGeneric->Locate(mFrame->mSlideshowDirs[mFrame->mSlideshowIndex]);
        //    wxSafeYield();
        //}
        //
        ret = mFrame->Action(kuID_SLIDESHOW);
    }
    mIsDoingCmd = false;
    return ret;
}

void kuApp::SetOptions() {
    mOptions.mKeepScale = false;
    mOptions.mScale = SCALE_BASE;
    mOptions.mKeepRotate = false;
    mOptions.mRotate = 0;
    mOptions.mKeepLeftTop = false;
    mOptions.mLeftTop = wxPoint(0,0);
    mOptions.mLoadCompletely = false;
    mOptions.mPrefetch = true;
    mOptions.mSlideInterval = 2.0;
    mOptions.mOpaque = 255;
    mOptions.mPrintScale = 1.0;
    mOptions.mPrintAlign = kuAlignSelector::kuID_ALIGN_CC;
    mOptions.mFilter = FILTER_BILINEAR;
    mOptions.mSlideshowRepeat = true;
    mOptions.mSlideshowDirectory = kuID_SLIDESHOW_CURRENT;
    mOptions.mLoadSiblings[0] = 2;
    mOptions.mLoadSiblings[1] = 1;
    mOptions.mLoadSiblings[2] = 3;
    mOptions.mLoadSiblings[3] = 2;
    mOptions.mLoadCache = 10;
}

void kuApp::ClearThread() {
    mWaitLoader = true;
    if(mLoader)    mLoader->Delete();
    while(mWaitLoader)    wxMilliSleep(100);
}

bool kuApp::SwitchLocale(wxString path, int lang) {
	if(mLocale!=NULL)   delete mLocale;
	mLocale = new wxLocale(lang);
	// load mo file
	mLocale->AddCatalogLookupPathPrefix(path);
    bool success=true;   // default is english
    if(lang!=wxLANGUAGE_ENGLISH) {
        // search CanonicalName
        success=mLocale->AddCatalog(wxT("kuview_")+wxLocale::GetLanguageInfo(lang)->CanonicalName);
        // search language code only
        if(!success)
            success=mLocale->AddCatalog(wxT("kuview_")+wxLocale::GetLanguageInfo(lang)->CanonicalName.BeforeFirst('_'));
        // fall to english if no appropriate mo file
        if(!success) {
            delete mLocale;
            mLocale = new wxLocale(wxLANGUAGE_ENGLISH);
        }
    }
    // regenerate menuBar/toolBar
    if(mFrame) {
        mFrame->SetupMenuBar();
        mFrame->SetupToolBar();
        mFrame->mSingle->SetupPopupMenu();
        mFrame->mGeneric->SetupPopupMenu();
    }
    return success;
}

bool kuApp::GetInterrupt() {
    return mIsInterrupted;
}
bool kuApp::SetInterrupt(bool interrupt) {
    wxMutexLocker lock(mInterruptMutex);
    if(!lock.IsOk())   return false;
    mIsInterrupted=interrupt;
    return true;
}

bool kuApp::GetBusy() {
    return mIsBusy;
}
bool kuApp::SetBusy(bool busy) {
    wxMutexLocker lock(mBusyMutex);
    if(!lock.IsOk())   return false;
    mIsBusy=busy;
    if(mFrame!=NULL && mFrame->IsShown()) {
        mFrame->GetMenuBar()->Enable(kuID_INTERRUPT,busy);
        mFrame->GetToolBar()->EnableTool(kuID_INTERRUPT,busy);
    }
    #ifdef __WXMSW__
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, !busy, 0, 0);
    #endif
    return true;
}

bool kuApp::GetEdited() {
    return mIsEdited;
}
bool kuApp::SetEdited(bool edited) {
    if(!mFrame || mFrame->mIsArchive)   return false;   // don't support edit in archive
    wxMutexLocker lock(mEditedMutex);
    if(!lock.IsOk())   return false;
    mIsEdited=edited;
    mFrame->GetMenuBar()->Enable(kuID_SAVE,edited);
    mFrame->GetToolBar()->EnableTool(kuID_SAVE,edited);
    mFrame->mSingle->mMenu->Enable(kuID_SAVE,edited);
    return true;
}

bool kuApp::SetSaveAs(bool saveas) {
    if(!mFrame || mFrame->mIsArchive)   return false;   // don't support edit in archive
    wxMutexLocker lock(mSaveAsMutex);
    if(!lock.IsOk())   return false;
    mIsSaveAs=saveas;
    mFrame->GetMenuBar()->Enable(kuID_SAVEAS,saveas);
    mFrame->mGeneric->mMenu->Enable(kuID_SAVEAS,saveas);
    mFrame->mSingle->mMenu->Enable(kuID_SAVEAS,saveas);
    return true;
}

double kuApp::GetScaleFromUser(wxWindow* parent, double current) {
    wxString str;
    double scale;
    while(true) {
        str = wxGetTextFromUser(STRING_RESCALE_MESSAGE,kuFrame::StripCodes(STRING_MENU_RESCALE),
                                wxString::Format(wxT("%.4f"),current),parent);
        if(str==wxEmptyString)    return -1;
        if(!str.ToDouble(&scale))    wxMessageBox(STRING_RESCALE_ERROR);
        else if(scale>0)    break;
    }
    return scale;
}
