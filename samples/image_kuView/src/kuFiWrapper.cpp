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

// -------- kuFiWrapper --------
bool kuFiWrapper::Initialize() {
    FreeImage_Initialise();
    FreeImage_SetOutputMessage(kuFiWrapper::ErrorHandler);
    return true;
}

bool kuFiWrapper::Finalize() {
    FreeImage_DeInitialise();
    return true;
}

void kuFiWrapper::ErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
    wxString self = THREAD_NAME_CURRENT;
    if(self!=THREAD_NAME_MAIN)    return;
    wxString msg(message, wxConvUTF8);
    wxLogDebug(self + wxT("ErrorHandler: ") + msg);
    if(msg.BeforeFirst(':')==wxT("Warning") || msg.BeforeFirst(':')==wxT("Exif")) {
        wxLogStatus(msg);
    } else {
        wxMessageBox(msg);
        if(wxGetApp().SetInterrupt(true) && wxGetApp().mFrame) {
            wxGetApp().mFrame->GetMenuBar()->Enable(kuID_INTERRUPT,false);
            wxGetApp().mFrame->GetToolBar()->EnableTool(kuID_INTERRUPT,false);
        }
    }
}

wxString kuFiWrapper::GetSupportedExtensions() {
    wxString exts;
    for(int i=0; i<FreeImage_GetFIFCount(); i++) {
        wxString list = wxString(FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i), wxConvUTF8);
        list.Replace(wxT(","), wxT(";*."));
        exts = exts + wxT("*.") + list + wxT(";");
    }
    exts.RemoveLast();
    #ifdef __WXMSW__
    return wxT("|") + exts;
    #else
    return wxT("|") + exts+wxT(";")+exts.Upper();
    #endif
}

wxString kuFiWrapper::GetMdModelString(FREE_IMAGE_MDTYPE type) {
    switch (type) {
        case FIMD_COMMENTS:
            return    STRING_METADATA_COMMENTS;
        case FIMD_EXIF_MAIN:
            return    STRING_METADATA_MAIN;
        case FIMD_EXIF_EXIF:
            return    STRING_METADATA_EXIF;
        case FIMD_EXIF_GPS:
            return    STRING_METADATA_GPS;
        case FIMD_EXIF_MAKERNOTE:
            return    STRING_METADATA_MAKERNOTE;
        case FIMD_EXIF_INTEROP:
            return    STRING_METADATA_INTEROP;
        case FIMD_IPTC:
            return    STRING_METADATA_IPTC;
        case FIMD_XMP:
            return    STRING_METADATA_XMP;
        case FIMD_GEOTIFF:
            return    STRING_METADATA_GEOTIFF;
        case FIMD_ANIMATION:
            return    STRING_METADATA_ANIMATION;
        default:
            return wxEmptyString;
    }
}

FREE_IMAGE_FILTER kuFiWrapper::GetFilterById(int id) {
    FREE_IMAGE_FILTER filter;
    switch (id) {
        case kuID_RESCALE_BOX:
            filter = FILTER_BOX;
            break;
        case kuID_RESCALE_BILINEAR:
            filter = FILTER_BILINEAR;
            break;
        case kuID_RESCALE_BSPLINE:
            filter = FILTER_BSPLINE;
            break;
        case kuID_RESCALE_BICUBIC:
            filter = FILTER_BICUBIC;
            break;
        case kuID_RESCALE_CATMULLROM:
            filter = FILTER_CATMULLROM;
            break;
        case kuID_RESCALE_LANCZOS3:
            filter = FILTER_LANCZOS3;
            break;
        default:
            return FILTER_BILINEAR;
    }
    return filter;
}
int kuFiWrapper::GetIdByFilter(FREE_IMAGE_FILTER filter) {
    int id;
    switch (filter) {
        case FILTER_BOX:
            id = kuID_RESCALE_BOX;
            break;
        case FILTER_BILINEAR:
            id = kuID_RESCALE_BILINEAR;
            break;
        case FILTER_BSPLINE:
            id = kuID_RESCALE_BSPLINE;
            break;
        case FILTER_BICUBIC:
            id = kuID_RESCALE_BICUBIC;
            break;
        case FILTER_CATMULLROM:
            id = kuID_RESCALE_CATMULLROM;
            break;
        case FILTER_LANCZOS3:
            id = kuID_RESCALE_LANCZOS3;
            break;
        default:
            return kuID_RESCALE_BILINEAR;
    }
    return id;
}

unsigned kuFiWrapper::ReadProc(void* buffer, unsigned size, unsigned count, fi_handle handle) {
	//return fread(buffer, size, count, (FILE *)handle);
    wxInputStream* in = (wxInputStream*)handle;
	in->Read(buffer, count*size);
    return (unsigned) in->LastRead();
}
unsigned kuFiWrapper::WriteProc(void* buffer, unsigned size, unsigned count, fi_handle handle) {
	//return fwrite(buffer, size, count, (FILE *)handle);
	return 0;
}
int kuFiWrapper::SeekProc(fi_handle handle, long offset, int origin) {
	//return fseek((FILE *)handle, offset, origin);
    wxInputStream* in = (wxInputStream*)handle;
    switch (origin) {
        case SEEK_SET:
            return (int) in->SeekI(offset, wxFromStart);
        case SEEK_CUR:
            return (int) in->SeekI(offset, wxFromCurrent);
        case SEEK_END:
            return (int) in->SeekI(offset, wxFromEnd);
        default:
            break;
    }
    return 0;
}
long kuFiWrapper::TellProc(fi_handle handle) {
	//return ftell((FILE *)handle);
    wxInputStream* in = (wxInputStream*)handle;
    return (long) in->TellI();
}

wxImage* kuFiWrapper::GetWxImage(wxString filename, bool isurl, int scale) {
    if(filename==wxEmptyString)   return NULL;
    FIBITMAP* bmp = GetFiBitmap(filename,isurl,scale);
    wxImage* image = new wxImage();
    FiBitmap2WxImage(bmp,image);
    FreeImage_Unload(bmp);
    return image;
};

FIBITMAP* kuFiWrapper::GetFiBitmap(wxString filename, bool isurl, int scale, bool fast) {
    if(filename==wxEmptyString)   return NULL;
    int flags = 0;
    if(fast)    scale = scale/2;
    FIBITMAP* bmp;
    if(isurl) {
        wxFileSystem* fileSystem = new wxFileSystem();
        wxFSFile* file=fileSystem->OpenFile(filename);
        if(file) {
            FreeImageIO io;
            io.read_proc  = (FI_ReadProc)ReadProc;
            io.write_proc = (FI_WriteProc)WriteProc;
            io.seek_proc  = (FI_SeekProc)SeekProc;
            io.tell_proc  = (FI_TellProc)TellProc;
            FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(&io, (fi_handle)file->GetStream(), 0);
            if(fif == FIF_UNKNOWN) {
                wxString name = filename.AfterLast(':');    // ex: "archive.zip#zip:filename", "archive.tar.gz#gzip:#tar:filename"
                if(name==wxEmptyString)
                    name = filename.BeforeFirst('#').BeforeLast('.');   // ex: "document.ps.gz#gzip:"
                #ifdef __WXMSW__
                fif = FreeImage_GetFIFFromFilenameU(name.wc_str(wxConvFile));
                #else
                fif = FreeImage_GetFIFFromFilename(name.mb_str(wxConvFile));
                #endif
                if(fif == FIF_UNKNOWN)    return NULL;
            }
            if(scale && fif==FIF_JPEG)    flags |= scale <<16;
            bmp = FreeImage_LoadFromHandle(fif, &io, (fi_handle)file->GetStream(), flags);
            delete file;
        } else    return NULL;
        delete fileSystem;
    } else {
        #ifdef __WXMSW__
        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(filename.wc_str(wxConvFile));
        if(fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilenameU(filename.wc_str(wxConvFile));
            if(fif == FIF_UNKNOWN)    return NULL;
        }
        if(scale && fif==FIF_JPEG)    flags |= scale <<16;
        bmp = FreeImage_LoadU(fif,filename.wc_str(wxConvFile),flags);
        #else
        FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename.mb_str(wxConvFile));
        if(fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilename(filename.mb_str(wxConvFile));
            if(fif == FIF_UNKNOWN)    return NULL;
        }
        if(scale && fif==FIF_JPEG)    flags |= scale <<16;
        bmp = FreeImage_Load(fif,filename.mb_str(wxConvFile),flags);
        #endif
    }
    FreeImage_FlipVertical(bmp);
    if(!fast && scale) {
        FIBITMAP* thumb = FreeImage_MakeThumbnail(bmp, scale);
        /* FreeImage_FUNC will clone metadata automatically after 3.11.0
        // set metadata to thumb
        #ifndef __WXMSW__
        FITAG* tag = NULL;
        FIMETADATA* mdhandle = NULL;
        for(int i=0; i<(int)FIMD_CUSTOM; i++) {
            if(!FreeImage_GetMetadataCount((FREE_IMAGE_MDMODEL)i, bmp))    continue;    // no data for this model
            mdhandle = FreeImage_FindFirstMetadata((FREE_IMAGE_MDMODEL)i, bmp, &tag);
            if(mdhandle) {
                do {
                    FreeImage_SetMetadata((FREE_IMAGE_MDMODEL)i, thumb, FreeImage_GetTagKey(tag), tag);
                } while(FreeImage_FindNextMetadata(mdhandle, &tag));
            }
            FreeImage_FindCloseMetadata(mdhandle);
        }
        #else
        FreeImage_CloneMetadata(thumb, bmp);
        #endif
        */
        FreeImage_Unload(bmp);
        return thumb;
    } else    return bmp;
};

bool kuFiWrapper::FiBitmap2WxImage(FIBITMAP* bmp, wxImage* image) {
    unsigned int width  = FreeImage_GetWidth(bmp);
    unsigned int height = FreeImage_GetHeight(bmp);
    unsigned int pitch  = FreeImage_GetPitch(bmp);
    unsigned int bpp    = FreeImage_GetBPP(bmp);
    //FREE_IMAGE_TYPE type = FreeImage_GetImageType(bmp);
    //if( (type!=FIT_BITMAP) || (bpp<24) ) {
    FIBITMAP* tmp;
    tmp = FreeImage_ConvertTo24Bits(bmp);
    if(!tmp)   return false;   // cannot convert to 24bits
    pitch = FreeImage_GetPitch(tmp);
    bpp = 24;

    image->Destroy();
    image->Create(width,height);
    BYTE* bits = FreeImage_GetBits(tmp);
    for(unsigned int y=0; y<height; y++) {
        BYTE* pixel = (BYTE*)bits;
        for(unsigned int x=0; x<width; x++) {
            image->SetRGB(x,y,
                          pixel[FI_RGBA_RED],
                          pixel[FI_RGBA_GREEN],
                          pixel[FI_RGBA_BLUE]);
            pixel += 3;
            if(bpp==32)    image->SetAlpha(x,height-1-y,pixel[FI_RGBA_ALPHA]);
        }
        bits += pitch;
    }
    FreeImage_Unload(tmp);
    return true;
};
wxImage* kuFiWrapper::FiBitmap2WxImage(FIBITMAP* bmp) {
    wxImage* image = new wxImage();
    if(FiBitmap2WxImage(bmp,image))    return image;
    return NULL;
};
