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

#include <wx/wx.h>
#include <wx/bookctrl.h>
#include <wx/colordlg.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/dirctrl.h>
#include <wx/dynlib.h>
#include <wx/fs_filter.h>
#include <wx/fs_arc.h>
#include <wx/grid.h>
#include <wx/image.h>
#include <wx/mimetype.h>
#include <wx/numdlg.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/propdlg.h>
#include <wx/splash.h>
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/tarstrm.h>
#include <wx/taskbar.h>
#include <wx/thread.h>
#include <wx/tokenzr.h>
#include <wx/treectrl.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "FreeImage.h"

#define SCALE_BASE      (10000)
#define SCALE_DIFF      (500)
#define SCALE_ORIGINAL  (0)
#define SCALE_BESTFIT   (-1)
#define SCALE_EXTEND    (-2)
#define SCALE_AUTOFIT   (-3)
#define SCALE_LASTUSED  (-4)
#define SCALE_THUMBNAIL (100)

#define WALLPAPER_CENTER  (0)
#define WALLPAPER_TILE    (1)
#define WALLPAPER_STRETCH (2)
#define WALLPAPER_BESTFIT (3)
#define WALLPAPER_EXTEND  (4)

#define STATUS_FIELD_SIZE   (2)
#define STATUS_FIELD_SCALE  (3)
#define STATUS_FIELD_MOTION (4)

#define SCROLL_RATE_SINGLE   (1)
#define SCROLL_RATE_MULTIPLE (5)

#define SPLITTER_TOP_WIDTH   (250)
#define SPLITTER_DIR_HEIGHT  (200)
#define SPLITTER_VIEW_HEIGHT (200)

#define STRING_APPNAME wxT("kuView")
#define STRING_VERSION wxT("Version: 1.5 stable")
//#define STRING_VERSION wxT("Version: 2009-04-30")
#define STRING_PROJECT wxT("Project: http://kujawiak.sourceforge.net/")
#define STRING_AUTHOR  wxT("Author: augustino@users.sourceforge.net")

#define STRING_LANGUAGE_FAILED  wxT("Cannot find its mo file!")
#define STRING_LANGUAGE_MESSAGE _("Please select language:")

#define STRING_INFO_ENUMERATE_ARCHIVE _("Enumerating items in Archive Panel...")
#define STRING_INFO_DELETE_ARCHIVE    _("Deleting all items in Archive Panel...")
#define STRING_INFO_THUMBS            _("Loading thumbs in selected directory...")
#define STRING_INFO_SAVED             _("File has been saved: ")
#define STRING_INFO_CANCELED          _("Operation has been canceled.")
#define STRING_INFO_FILE_CUT          _("Cut: ")
#define STRING_INFO_FILE_COPY         _("Copy: ")
#define STRING_INFO_FILE_COPYING      _("Copying: ")
#define STRING_INFO_FILE_MOVING       _("Moving: ")
#define STRING_INFO_FILE_DELETING     _("Deleting: ")
#define STRING_INFO_FILE_RENAMESUCCEED  _("File has been renamed: ")
#define STRING_INFO_FILE_COPYSUCCEED    _("File has been copied: ")
#define STRING_INFO_FILE_MOVESUCCEED    _("File has been moved: ")
#define STRING_INFO_FILE_DELETESUCCEED  _("File has been deleted: ")
#define STRING_INFO_METADATA_NOTFOUND   _("File doesn't contain any metadata...")
#define STRING_WARNING_INTERRUPTED      _("Interrupted by user.")
#define STRING_WARNING_ISNOTFILE        _("Please select a picture.")
#define STRING_WARNING_DIRNOFILES       _("Please select a directory which contains pictures.")
#define STRING_ERROR_BUSY               _("Please wait or interrupt current operation!")
#define STRING_ERROR_SAVEFAILED         _("Cannot save: ")
#define STRING_ERROR_FILE_RENAMEFAILED  _("Cannot rename: ")
#define STRING_ERROR_FILE_COPYFAILED    _("Cannot copy: ")
#define STRING_ERROR_FILE_MOVEFAILED    _("Cannot move: ")
#define STRING_ERROR_FILE_DELETEFAILED  _("Cannot delete: ")
#define STRING_ERROR_FILE_MKDIRFAILED   _("Cannot mkdir: ")
#define STRING_ERROR_FILE_RMDIRFAILED   _("Cannot rmdir: ")
#define STRING_ERROR_FILE_EXIST         _("Already exists: ")
#define STRING_ERROR_FILE_NOTEXIST      _("Doesn't exist: ")
#define STRING_ERROR_FILE_DESISSRC      _("Destination is source!\n")
#define STRING_ERROR_FILE_DESINSRC      _("Destination is in source!\n")
#define STRING_ERROR_WALLPAPER_ARCHIVE  _("Setting a picture in archive as wallpaper is not supported!")
#define STRING_ERROR_PREVIEW_FAILED     _("Cannot preview! Please check printer setting...")
#define STRING_ERROR_PRINT_FAILED       _("There was an error during printing!\nPlease check printer and try again...")

#define STRING_MENU_FILE       _("&File")
#define STRING_MENU_SAVE       _("&Save\tCtrl+S")
#define STRING_MENU_SAVEAS     _("Save &As...")
#define STRING_MENU_PAGESETUP  _("Pa&ge Setup")
#define STRING_MENU_PREVIEW    _("Print Pre&view...\tShift+P")
#define STRING_MENU_PRINT      _("&Print\tCtrl+P")
#ifdef __WXMSW__
#define STRING_MENU_PROPERTIES _("Prope&rties\t[")
#endif
#define STRING_MENU_METADATA   _("M&etadata\t]")
#define STRING_MENU_INTERRUPT  _("&Interrupt\tCtrl+I")
#define STRING_MENU_RELOAD     _("&Reload\tF5")
#define STRING_MENU_MINIMIZE   _("Minimi&ze\tCtrl+Z")
#define STRING_MENU_RESTORE    _("&Restore")
#define STRING_MENU_EXIT       _("&Quit\tCtrl+Q")

#define STRING_MENU_EDIT       _("&Edit")
#define STRING_MENU_ROTATE_CCW _("Rotate -90\tAlt+LEFT")
#define STRING_MENU_ROTATE_CW  _("Rotate +90\tAlt+RIGHT")
#define STRING_MENU_RESCALE    _("&Scale")

#define STRING_MENU_TRAVERSE   _("&Traverse")
#define STRING_MENU_GOHOME     _("Go &Home\tAlt+HOME")
#define STRING_MENU_PREV       _("&Previous in Directory")
#define STRING_MENU_NEXT       _("&Next in Directory")
#define STRING_MENU_HOME       _("&First in Directory")
#define STRING_MENU_END        _("La&st in Directory")
#define STRING_MENU_UP         _("Move &Up in Tree")
#define STRING_MENU_DOWN       _("Move &Down in Tree")
#define STRING_MENU_LEFT       _("Move &Left in Tree")
#define STRING_MENU_RIGHT      _("Move &Right in Tree")

#define STRING_MENU_VIEW       _("&View")
#define STRING_MENU_FULLSCREEN _("F&ull Screen\tENTER")
#define STRING_MENU_ZOOM_IN    _("Zoom &In\t+")
#define STRING_MENU_ZOOM_OUT   _("Zoom &Out\t-")
#define STRING_MENU_ZOOM_100   _("Zoom &100%\t*")
#define STRING_MENU_ZOOM_FIT   _("Zoom &Fit\t/")
#define STRING_MENU_ZOOM_EXT   _("Zoom &Extend\t\\")
#define STRING_MENU_OPAQUE     _("Opa&que")
#define STRING_MENU_SLIDESHOW_START    _("&Slide Show\tSPACE")
#define STRING_MENU_SLIDESHOW_PAUSE    _("Pause &Slide Show\tSPACE")
#define STRING_MENU_SLIDESHOW_CONTINUE _("Continue &Slide Show\tSPACE")

#define STRING_MENU_PANEL       _("&Panel")
#define STRING_MENU_FILESYSTEM  _("&Filesystem\tF10")
#define STRING_MENU_ARCHIVE     _("&Archive\tF11")
#define STRING_MENU_THUMBNAIL   _("&Thumbnail\tF12")

#define STRING_MENU_MANAGE      _("&Manage")
#define STRING_MENU_MKDIR       _("&New Directory")
#define STRING_MENU_RENAME      _("&Rename File\tF2")
#define STRING_MENU_FILE_CUT    _("Cu&t File\tCtrl+X")
#define STRING_MENU_FILE_COPY   _("&Copy File\tCtrl+C")
#define STRING_MENU_FILE_PASTE  _("&Paste File\tCtrl+V")
#define STRING_MENU_FILE_DELETE _("&Delete File\tDEL")
#define STRING_MENU_FILE_MOVETO _("&Move File To...")
#define STRING_MENU_FILE_COPYTO _("Copy File &To...")

#define STRING_MENU_OPTION      _("&Option")
#define STRING_MENU_SHELL       _("&Shell Integration")
#define STRING_MENU_ASSOCIATION _("File &Association")
#define STRING_MENU_OPTION_DESKTOP     _("&Desktop")
#define STRING_MENU_DESKTOP_WALLPAPER  _("Set as &Wallpaper")
#define STRING_MENU_DESKTOP_NONEWP     _("&None Wallpaper")
#define STRING_MENU_DESKTOP_BACKGROUND _("&Background Color")
#define STRING_MENU_LANGUAGE    _("&Language")
#define STRING_MENU_OPTION_VIEW _("&View")
#define STRING_MENU_FIXED_SCALE   _("Fixed &Scale")
#define STRING_MENU_FIXED_ROTATE  _("Fixed &Rotation")
#define STRING_MENU_FIXED_LEFTTOP _("Fixed &LeftTop")
#define STRING_MENU_LOAD_COMPLETELY _("Load &Completely")
#define STRING_MENU_PREFETCH        _("&Prefetch")
#define STRING_MENU_OPTION_RESCALE     _("Scale &Filter")
#define STRING_MENU_RESCALE_BOX        _("Bo&x")
#define STRING_MENU_RESCALE_BILINEAR   _("Bi&linear")
#define STRING_MENU_RESCALE_BSPLINE    _("B-&Spline")
#define STRING_MENU_RESCALE_BICUBIC    _("&Bicubic")
#define STRING_MENU_RESCALE_CATMULLROM _("&Catmull-Rom")
#define STRING_MENU_RESCALE_LANCZOS3   _("&Lanczos")
#define STRING_MENU_OPTION_SLIDESHOW   _("Slide Sho&w")
#define STRING_MENU_SLIDESHOW_REPEAT   _("Repeat")

#define STRING_MENU_HELP     _("&Help")
#define STRING_MENU_ABOUT    _("&About")

#define STRING_CONTINUE_MESSAGE _("\nDo you want to skip it and continue?")
#define STRING_CONTINUE_CAPTION _("Confirm")
#define STRING_LOSSLESS_MESSAGE _("Rotate/Clone original file losslessly?")
#define STRING_SAVE_MESSAGE     _("This function is NOT complete!\nFile quality and information may be STRIPPED!!\nDo you want to continue?")
#define STRING_LOAD_MESSAGE     _("Current file has been modified,\nreload original file now?")
#define STRING_MKDIR_MESSAGE    _("Please input directory name:")
#define STRING_RENAME_MESSAGE   _("Please input new name:")
#define STRING_DELETE_MESSAGE   _("The file will be deleted!\n")
#ifdef __WXMSW__
#define STRING_SHELL_CHOICE_DRIVE     _("Drive")
#define STRING_SHELL_CHOICE_DIRECTORY _("Directory")
#define STRING_SHELL_CHOICE_FILE      _("File")
#define STRING_SHELL_MESSAGE _("Please select where to add\n\"Browse with kuView\" entry:\n(ContextMenu on right click)")
#define STRING_SHELL_BROWSE  _("Browse with kuView")
#define STRING_ASSOCIATION_MESSAGE _("Please select file extension:\n(WITHOUT restore function!)")
#define STRING_WALLPAPER_CHOICE_CENTER  _("Center")
#define STRING_WALLPAPER_CHOICE_TILE    _("Tile")
#define STRING_WALLPAPER_CHOICE_STRETCH _("Stretch")
#define STRING_WALLPAPER_CHOICE_BESTFIT _("Scale to Fit")
#define STRING_WALLPAPER_CHOICE_EXTEND  _("Scale to Extend")
#define STRING_WALLPAPER_MESSAGE        _("Please select how to set wallpaper:")
#define STRING_NONEWP_MESSAGE           _("Do you want to disable wallpaper?")
#endif
#define STRING_OPAQUE_MESSAGE    _("Please input a number between 0 and 255:\n(0 is fully transparent, and 255 is fully opaque)")
#define STRING_OPAQUE_PROMPT     _("opaque value:")
#define STRING_SLIDESHOW_MESSAGE _("Please input interval in second:")
#define STRING_SLIDESHOW_CAPTION _("Interval")
#define STRING_BUTTON_CLOSE   _("&Close")
#define STRING_BUTTON_PRINT   _("&Print...")
#define STRING_BUTTON_ALIGN   _("&Align...")
#define STRING_BUTTON_SCALE   _("&Scale...")
#define STRING_ALIGN_MESSAGE  _("Please select the position button for alignment:")
#define STRING_ALIGN_CENTER   _("Center")
#define STRING_ALIGN_LEFT     _("Left")
#define STRING_ALIGN_RIGHT    _("Right")
#define STRING_ALIGN_TOP      _("Top")
#define STRING_ALIGN_BOTTOM   _("Bottom")
#define STRING_RESCALE_MESSAGE  _("Please enter the scale ratio:")
#define STRING_RESCALE_ERROR    _("It is not a valid number!\nPlease try again...")

#ifdef __WXMSW__
#define STRING_REG_SHORTNAME wxT("kuView")
#define STRING_REG_COMMAND   wxT("command")
#define STRING_REG_OPEN      wxT("open")
#define STRING_REG_ICON      wxT("DefaultIcon")
#define STRING_REG_SHELLCMD  wxT("shell\\open\\command")
#define STRING_REG_MAIN      wxT("HKEY_CLASSES_ROOT\\kuView")
#define STRING_REG_CLASSEXT  wxT("HKEY_CLASSES_ROOT\\")
#define STRING_REG_USEREXT   wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\")
#define STRING_REG_APP       wxT("HKEY_CLASSES_ROOT\\Applications\\kuview.exe")
#define STRING_REG_DRIVE     wxT("HKEY_CLASSES_ROOT\\Drive\\shell\\kuView")
#define STRING_REG_DIRECTORY wxT("HKEY_CLASSES_ROOT\\Directory\\shell\\kuView")
#define STRING_REG_FILE      wxT("HKEY_CLASSES_ROOT\\*\\shell\\kuView")
#define STRING_REG_DESKTOP   wxT("HKEY_CURRENT_USER\\Control Panel\\Desktop\\")
#define STRING_REG_MSIE      wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\Desktop\\General\\")
#define STRING_REG_THEME     wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\LastTheme\\")
#define STRING_REG_WP_MAIN      wxT("Wallpaper")
#define STRING_REG_WP_STYLE     wxT("WallpaperStyle")
#define STRING_REG_WP_TILE      wxT("TileWallpaper")
#define STRING_REG_WP_BACKUP    wxT("BackupWallpaper")
#define STRING_REG_WP_CONVERTED wxT("ConvertedWallpaper")
#define STRING_REG_WP_ORIGINAL  wxT("OriginalWallpaper")
#define STRING_REG_WP_SOURCE    wxT("WallpaperSource")
//#define STRING_WALLPAPER_BMPFILE        wxT("\\Local Settings\\Application Data\\Microsoft\\Wallpaper1.bmp")
#define STRING_WALLPAPER_BMPFILE        wxT("\\.kuView_Wallpaper.bmp")
#define STRING_WALLPAPER_UPDATE_COMMAND wxT("\\System32\\RUNDLL32.EXE user32.dll UpdatePerUserSystemParameters 1 True")
#endif

//#define STRING_SHORTCUT_HOME _("Home Directory")

#define STRING_FILTER_STANDARD         wxT("Image Files")
#define STRING_FILTER_ARCHIVE          wxT("|Archive Files(*.zip;*.tar;*.gz)|*.zip;*.tar;*.gz")
#define STRING_FILTER_ALLFILES         wxT("|All Files(*.*)|*.*")

#define INDEX_FILTER_STANDARD (0)
#define INDEX_FILTER_ARCHIVE  (1)
#define INDEX_FILTER_ALLFILES (2)

#define STRING_METADATA_COMMENTS  _("Comments")         // 0, single comment or keywords
#define STRING_METADATA_MAIN      _("Main")             // 1, Exif-TIFF metadata
#define STRING_METADATA_EXIF      _("Exif")             // 2, Exif-specific metadata
#define STRING_METADATA_GPS       _("GPS")              // 3, Exif GPS metadata
#define STRING_METADATA_MAKERNOTE _("Maker Note")       // 4, Exif maker note metadata
#define STRING_METADATA_INTEROP   _("Interoperability") // 5, Exif interoperability metadata
#define STRING_METADATA_IPTC      _("IPTC")             // 6, IPTC/NAA metadata
#define STRING_METADATA_XMP       _("XMP")              // 7, Abobe XMP metadata
#define STRING_METADATA_GEOTIFF   _("GeoTIFF")          // 8, GeoTIFF metadata
#define STRING_METADATA_ANIMATION _("Animation")        // 9, Animation metadata
#define STRING_METADATA_LABEL_KEY         _("Tag")
#define STRING_METADATA_LABEL_VALUE       _("Value")
#define STRING_METADATA_LABEL_DESCRIPTION _("Description")

// marcos for threads
#define THREAD_NAME_MAIN    wxT("[main] ")
#define THREAD_NAME_LOADER  wxT("[loader] ")
#define THREAD_NAME_CURRENT (wxThread::IsMain() ? THREAD_NAME_MAIN : THREAD_NAME_LOADER)

// command interface
#define CMD_SEP       wxT('|')
#define CMD_LOCATE    wxT("locate")
#define CMD_SLIDESHOW wxT("slideshow")

enum {
    kuID_LOWEST=wxID_HIGHEST,
    // menu file
    kuID_FILE,
    kuID_SAVE,
    kuID_SAVEAS,
    kuID_PAGESETUP,
    kuID_PREVIEW,
    kuID_PRINT,
    #ifdef __WXMSW__
    kuID_PROPERTIES,
    #endif
    kuID_METADATA,
    kuID_INTERRUPT,
    kuID_RELOAD,
    kuID_ESCAPE,
    kuID_MINIMIZE,
    kuID_RESTORE,
    // menu edit
    kuID_EDIT,
    kuID_ROTATE_CCW,
    kuID_ROTATE_CW,
    kuID_RESCALE,
    // menu traverse
    kuID_TRAVERSE,
    kuID_GOHOME,
    kuID_PREV,
    kuID_NEXT,
    kuID_HOME,
    kuID_END,
    kuID_UP,
    kuID_DOWN,
    kuID_LEFT,
    kuID_RIGHT,
    // menu view
    kuID_VIEW,
    kuID_FULLSCREEN,
    kuID_ZOOM_IN,
    kuID_ZOOM_OUT,
    kuID_ZOOM_100,
    kuID_ZOOM_FIT,
    kuID_ZOOM_EXT,
    kuID_ZOOM_AUTO,
    kuID_OPAQUE,
    kuID_SLIDESHOW,
    // menu panel
    kuID_PANEL,
    kuID_FILESYSTEM,
    kuID_ARCHIVE,
    kuID_THUMBNAIL,
    // menu manage
    kuID_MANAGE,
    kuID_MKDIR,
    kuID_RENAME,
    kuID_FILE_CUT,
    kuID_FILE_COPY,
    kuID_FILE_PASTE,
    kuID_FILE_DELETE,
    kuID_FILE_MOVETO,
    kuID_FILE_COPYTO,
    // menu option
    kuID_OPTION,
    kuID_SHELL,
    kuID_ASSOCIATION,
    kuID_WALLPAPER,
    kuID_NONEWP,
    kuID_BACKGROUND,
    kuID_LANGUAGE,
    kuID_OPTION_VIEW,
    kuID_FIXED_SCALE,
    kuID_FIXED_ROTATE,
    kuID_FIXED_LEFTTOP,
    kuID_LOAD_COMPLETELY,
    kuID_PREFETCH,
    kuID_OPTION_RESCALE,
    kuID_RESCALE_BOX,
    kuID_RESCALE_BILINEAR,
    kuID_RESCALE_BSPLINE,
    kuID_RESCALE_BICUBIC,
    kuID_RESCALE_CATMULLROM,
    kuID_RESCALE_LANCZOS3,
    kuID_SLIDESHOW_REPEAT,
    kuID_SLIDESHOW_CURRENT,
    kuID_SLIDESHOW_SIBLING,
    kuID_TIMER_IDLE,
    // other
    kuID_LOCATE,
    kuID_HIGHEST
};

class kuFrame;

// kuGenericDirCtrl
class kuGenericDirCtrl: public wxGenericDirCtrl {
private:
    kuFrame* mFrame;
    void OnTreeSelChanged(wxTreeEvent& event);
    void OnSetFocus(wxFocusEvent& event);
    void OnChoice(wxCommandEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
public:
    kuGenericDirCtrl(wxWindow* parent, kuFrame* frame);
    void           SetupPopupMenu();
    bool           Locate(wxString location);
    wxArrayString* EnumerateChildren(wxString dirname);
    wxString       GetNeighbor();
    wxString       GetDir();
    void           Reload(bool parent);
    void SwitchFilter(int index);
    void AddShortcuts();
    wxMenu* mMenu;
DECLARE_EVENT_TABLE()
};

// kuVirtualDirCtrl
class kuVirtualDirCtrl: public wxTreeCtrl {
private:
    // kuVirtualDirData
    class kuVirtualDirData: public wxTreeItemData {
    public:
        kuVirtualDirData(wxString& filename, bool isdir);
        wxString mFilename;
        bool     mIsDir;
    };
    kuFrame*     mFrame;
    wxString     mArchive;
    bool         mIsDeleting;
    void OnTreeSelChanged(wxTreeEvent& event);
    void EnumerateArchive(wxString archive);
    bool IsImageFile(wxString& filename);
    bool IsDir(wxTreeItemId id);
public:
    kuVirtualDirCtrl(wxWindow* parent, kuFrame* frame);
    void           SetRoot(wxString archive);
    wxTreeItemId   FindTextInChildren(wxTreeItemId pid, wxString& text);
    bool           Locate(wxString location);
    wxString       GetFilePath(bool isurl, bool fileonly, wxTreeItemId id=0);
    wxArrayString* EnumerateChildren(wxString dirname);
DECLARE_EVENT_TABLE()
};

// kuSingleScrolled
class kuSingleScrolled: public wxScrolledWindow {
private:
    kuFrame*  mFrame;
    wxImage   mDispImg;
    FIBITMAP* mOrigBmp;
    wxSize    mOrigSize;
    wxSize    mDispSize;
    wxRect    mView;
    wxPoint   mDragStart;
    wxPoint   mLeftTop;
    wxPoint   mRightBottom;
    wxSize    mLoadSize;
    float     mScale;
    wxString  mFilename;
    bool      mIsUrl;
    wxTimer   mIdleTimer;
    void OnSize(wxSizeEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void CheckViewSize();
    void CheckViewStart();
    void RestoreRotate(int rotate);
    void RestoreLeftTop(wxPoint lefttop);
    void OnMousewheel(wxMouseEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
    void OnIdleTimer(wxTimerEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    bool KeyAction(int keycode);
public:
    kuSingleScrolled(wxWindow* parent, kuFrame* frame);
    ~kuSingleScrolled();
    void           SetupPopupMenu();
    virtual void   OnDraw(wxDC& dc);
    void           ReloadImage(wxString filename, bool isurl);
    bool           SaveImage(wxString filename, bool ask=true);
    wxString       GetFilename();
    int            GetLoadSizeMax();
    wxSize         GetOrigSize();
    FIBITMAP*      GetOrigBmp();
    void Rotate90(bool cw);
    void SetFullScreen(bool full);
    bool SetScale(int diff);
    bool Rescale(double scale);
    void HideCursor(bool hide=true);
    wxMenu* mMenu;
DECLARE_EVENT_TABLE()
};

// kuMultipleScrolled
class kuMultipleScrolled: public wxScrolledWindow {
private:
    kuFrame*  mFrame;
    wxString  mDirname;
    void OnKeyDown(wxKeyEvent& event);
    void OnSize(wxSizeEvent& event);
public:
    kuMultipleScrolled(wxWindow* parent, kuFrame* frame);
    void ReloadThumbs(wxString dirname, bool isurl);
    void RemoveThumb(wxString filename, bool isurl);
    void Locate(wxString filename);
DECLARE_EVENT_TABLE()
};

// kuScrollHandler
class kuScrollHandler: public wxEvtHandler {
private:
    kuFrame* mFrame;
    void OnKeyDown(wxKeyEvent& event);
    void OnLeftDclick(wxMouseEvent& event);
    void OnMiddleUp(wxMouseEvent& event);
    void OnEnterWindow(wxMouseEvent& event);
    void OnMenuRange(wxCommandEvent& event);
public:
    kuScrollHandler(kuFrame* frame);
DECLARE_EVENT_TABLE()
};

// kuThumbButton
class kuThumbButton: public wxBitmapButton {
private:
    kuFrame* mFrame;
    bool     mIsUrl;
    void OnButton(wxCommandEvent& event);
    void OnMouseEvents(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
public:
    kuThumbButton(wxWindow* parent, wxString filename, wxString url);
    //kuThumbButton(wxWindow* parent, wxInputStream& stream, wxString name);
DECLARE_EVENT_TABLE()
};

// kuCheckListDialog
class kuCheckListDialog: public wxDialog {
public:
    kuCheckListDialog(wxWindow* parent, kuFrame* frame, const wxString& message, const wxString& caption, const wxArrayString& choices);
    wxArrayInt GetSelections();
    void       SetSelections(const wxArrayInt& selections);
private:
    kuFrame* mFrame;
    wxCheckListBox* mListBox;
};

// kuMetaSheetDialog
class kuMetaSheetDialog: public wxPropertySheetDialog {
public:
    kuMetaSheetDialog(wxWindow* parent, kuFrame* frame, const wxString& filename, const wxArrayString& metadata);
private:
    kuFrame* mFrame;
};

// kuPrintout
class kuPrintout: public wxPrintout {
private:
    kuSingleScrolled*      mSingle;
    wxPageSetupDialogData* mPageSetupDlgData;
    FIBITMAP* mPrintBmp;
    wxImage   mPrintImg;
public:
    kuPrintout(kuSingleScrolled* single, wxPageSetupDialogData* data);
    ~kuPrintout();
    bool OnPrintPage(int page);
    bool HasPage(int page);
    bool OnBeginDocument(int startPage, int endPage);
    void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
    void DrawPage();
};

// kuPreviewFrame
class kuPreviewFrame: public wxPreviewFrame {
private:
public:
    kuPreviewFrame(wxPrintPreview* preview, wxWindow* parent, const wxString& title);
    void CreateControlBar();
};

// kuPreviewControlBar
class kuPreviewControlBar: public wxPreviewControlBar {
private:
    wxButton* m_alignButton;
    wxButton* m_scaleButton;
    enum {
        kuID_PREVIEW_BTNALIGN = wxID_PREVIEW_GOTO + 1,
        kuID_PREVIEW_BTNSCALE
    };
public:
    kuPreviewControlBar(wxPrintPreviewBase *preview, long buttons, wxWindow *parent, const wxPoint& pos, const wxSize& size);
    void CreateButtons();
    void OnAlign(wxCommandEvent& event);
    void OnScale(wxCommandEvent& event);
    void UpdatePreview();
};

// kuAlignSelector
class kuAlignSelector: public wxDialog {
private:
    wxStaticText* mSelect;
    int           mAlign;
public:
    kuAlignSelector(wxWindow* parent);
    bool Show(bool show=true);
    void OnButton(wxCommandEvent& event);
    int  GetAlign();
    void SetAlign(int align);
    enum {
        kuID_ALIGN_LOWEST = kuID_HIGHEST,
        kuID_ALIGN_LT,
        kuID_ALIGN_CT,
        kuID_ALIGN_RT,
        kuID_ALIGN_LC,
        kuID_ALIGN_CC,
        kuID_ALIGN_RC,
        kuID_ALIGN_LB,
        kuID_ALIGN_CB,
        kuID_ALIGN_RB,
        kuID_ALIGN_HIGHEST
    };
};

// kuTaskBarIcon
class kuTaskBarIcon: public wxTaskBarIcon {
private:
    kuFrame* mFrame;
    virtual wxMenu* CreatePopupMenu();
    void OnLeftDclick(wxTaskBarIconEvent& event);
    void OnRestore(wxCommandEvent& event);
public:
    kuTaskBarIcon(kuFrame* frame);
DECLARE_EVENT_TABLE()
};

// kuStatusBar
class kuStatusBar: public wxStatusBar {
private:
    kuFrame* mFrame;
    wxGauge* mGauge;
    void OnSize(wxSizeEvent& event);
public:
    kuStatusBar(kuFrame* frame);
    void SetGaugeRange(int range);
    void IncrGaugeValue();
    void ClearGaugeValue();
DECLARE_EVENT_TABLE()
};

class kuFrame: public wxFrame {
private:
    wxArrayString mFileClipboard;
    bool          mFileIsMove;
    wxString      mLastDirPath;
    wxPageSetupDialogData mPageSetupDlgData;
    void       ToggleFilesystem(bool filesystem);
    void       ToggleFilesystem();
    void       ToggleArchive();
    void       ToggleThumbnail(bool thumbnail);
    void       ToggleThumbnail();
    bool       PlaySlideshow(long interval);
    wxArrayInt ShellIntegration(wxArrayInt style);
    void       FileAssociation(wxString ext, bool associate);
    bool       SetAsWallpaper(wxString filename, int pos);
    bool       SetWallpaperToNone();
    bool       CopyToFileClipboard(wxString target, bool iscut);
    bool       PasteDirOrFile(wxArrayString& srcs, wxString desdir, bool ismove);
    bool       DeleteDir(wxString dirname, bool recycle);
    bool       DeleteFile(wxString filename, bool recycle);
#ifdef __WXMSW__
    bool       MswCopyDirOrFile(wxString src, wxString des, bool ismove);
    bool       MswDeleteDirOrFile(wxString target, bool recycle);
#else
    bool       CopyDir(wxString srcdir, wxString desdir, bool ismove);
    bool       CopyFile(wxString srcfile, wxString desfile, bool ismove);
#endif
    bool       RenameDirOrFile(wxString src, wxString des);
    bool       DirIsParent(wxString parent, wxString child);
    bool       Traverse(wxTreeCtrl* tree, int direct);
    void       ReloadPath(wxString path, bool recreate);
    void       CheckEdited();

public:
	kuFrame();
    void     OnQuit(wxCommandEvent& event);
	void     OnTool(wxCommandEvent& event);
	void     OnSize(wxSizeEvent& event);
	void     OnClose(wxCloseEvent& event);
    void     OnAbout(wxCommandEvent& event);
	void     SetupMenuBar();
	void     SetupToolBar(bool create=false);
	void     SetupStatusBar();
	void     SetupWindows(bool isdir, bool isarchive);
	void     SetupAccelerator();
	void     SetupPrinting();
	void     SetDefaults();
    bool     Action(int action, wxString arg1=wxEmptyString, wxString arg2=wxEmptyString);
    void     ToggleArchive(bool archive);
    static wxString StripCodes(const wxChar* cptr);
	wxSplitterWindow* mTopSplitter;
	wxSplitterWindow* mDirSplitter;
	wxSplitterWindow* mViewSplitter;
    kuGenericDirCtrl* mGeneric;
    kuVirtualDirCtrl* mVirtual;
	kuSingleScrolled*   mSingle;
	kuMultipleScrolled* mMultiple;
	kuScrollHandler*  mHandler;
	kuTaskBarIcon*    mTaskBarIcon;
	kuStatusBar*      mStatusBar;
	wxString          mShellExecute;
	wxString          mAssociationExecute;
        wxArrayString     mSlideshowDirs;
        size_t            mSlideshowIndex;
	wxIcon            mIconApp;
	bool              mIsFilesystem;
	bool              mIsArchive;
	bool              mIsThumbnail;
	bool              mIsSlideshow;
	bool              mIsPause;

	DECLARE_EVENT_TABLE()
};

// kuLoadThread
struct kuBmpValue {
    int       Size;
    FIBITMAP* Bmp;
};
WX_DECLARE_STRING_HASH_MAP(kuBmpValue, kuBmpHash);

struct kuLoadTicket {
    wxString  Path;
    bool      IsUrl;
    int       Size;
};
WX_DECLARE_OBJARRAY(kuLoadTicket, kuLoadArray);

class kuLoadThread: public wxThread {
private:
    kuFrame*      mFrame;
    wxArrayString mBmpCache;
    kuBmpHash     mBmpHash;
    wxMutex       mBmpMutex;
    kuLoadArray   mLoadQueue;
    wxMutex       mQueueMutex;
    void AddBmpHash(wxString filename, kuBmpValue& value);
	void EraseBmpHash(wxString filename);
public:
    kuLoadThread(kuFrame* frame);
    virtual void* Entry();
    bool      Clear();
    bool      Append(wxString filename, bool isurl, int size);
    bool      Replace(wxString filename, FIBITMAP* bmp);
    FIBITMAP* GetFiBitmap(wxString filename, bool isurl, int size);
    void      ClearBmpHash();
    void      LogDebug(wxString msg);
};

// kuOptions
class kuOptions {
public:
    bool     mKeepScale;
    float    mScale;
    bool     mKeepRotate;
    int      mRotate;
    bool     mKeepLeftTop;
    wxPoint  mLeftTop;
    bool     mLoadCompletely;
    bool     mPrefetch;
    long     mSlideInterval;
    long     mOpaque;
    double   mPrintScale;
    int      mPrintAlign;
    FREE_IMAGE_FILTER mFilter;
    bool     mSlideshowRepeat;
    int      mSlideshowDirectory;
    int      mLoadSiblings[4];
    int      mLoadCache;
};

// kuFiWrapper
class kuFiWrapper {
private:
public:
    static bool Initialize();
    static bool Finalize();
    static void               ErrorHandler(FREE_IMAGE_FORMAT fif, const char *message);
    static wxString           GetSupportedExtensions();
    static unsigned           ReadProc(void* buffer, unsigned size, unsigned count, fi_handle handle);
    static unsigned           WriteProc(void* buffer, unsigned size, unsigned count, fi_handle handle);
    static int                SeekProc(fi_handle handle, long offset, int origin);
    static long               TellProc(fi_handle handle);
    static wxString           GetMdModelString(FREE_IMAGE_MDTYPE type);
    static FREE_IMAGE_FILTER  GetFilterById(int id);
    static int                GetIdByFilter(FREE_IMAGE_FILTER filter);
    static wxImage*           GetWxImage(wxString filename, bool isurl, int scale);
    static FIBITMAP*          GetFiBitmap(wxString filename, bool isurl, int scale, bool fast=false);
    static bool               FiBitmap2WxImage(FIBITMAP* bmp, wxImage* image);
    static wxImage*           FiBitmap2WxImage(FIBITMAP* bmp);
};

// kuApp
class kuApp: public wxApp {
private:
    bool    mIsInterrupted;
    wxMutex mInterruptMutex;
    bool    mIsBusy;
    wxMutex mBusyMutex;
    bool    mIsEdited;
    wxMutex mEditedMutex;
    bool    mIsSaveAs;
    wxMutex mSaveAsMutex;
public:
    enum {
        kuID_INIT_SHOW=0,
        kuID_INIT_HIDE,
        kuID_INIT_FULL,
        kuID_INIT_ICON,
    };
    kuFiWrapper   mFi;
    kuOptions     mOptions;
    kuFrame*      mFrame;
    kuLoadThread* mLoader;
    wxLocale*     mLocale;
    wxString      mPath;
    bool          mWaitLoader;
    int           mInit;
    bool          mQuit;
    wxArrayString mCmds;
    bool          mIsDoingCmd;
    virtual bool OnInit();
    virtual int  OnExit();
    void PrintCmdHelp();
    bool ParseArguments(int argc, wxChar** argv);
    bool DoInitCmd(wxChar* cmd);
    bool IsPathCmd(wxChar* cmd);
    bool IsActionCmd(wxChar* cmd);
    bool DoActionCmd(wxString& cmd);
    void SetOptions();
    void ClearThread();
    bool SwitchLocale(wxString path, int lang);
    bool GetInterrupt();
    bool SetInterrupt(bool interrupt);
    bool GetBusy();
    bool SetBusy(bool busy);
    bool GetEdited();
    bool SetEdited(bool edited);
    bool SetSaveAs(bool saveas);
    static double GetScaleFromUser(wxWindow* parent, double current=1.0);
};
DECLARE_APP(kuApp)
