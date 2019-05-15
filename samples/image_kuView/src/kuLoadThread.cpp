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

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(kuLoadArray);

int arraycompare(kuLoadTicket** arg1, kuLoadTicket** arg2) {
    return (*arg1)->Path.Cmp((*arg2)->Path);
}

kuLoadThread::kuLoadThread(kuFrame* frame)
    :wxThread() {
    mFrame = frame;
    mLoadQueue.Clear();
    mBmpHash.clear();
    mBmpCache.Clear();
    wxString self = THREAD_NAME_CURRENT;
    wxLogDebug(self + wxT("kuLoadThread: created..."));
}

void* kuLoadThread::Entry() {
    wxString self = THREAD_NAME_CURRENT;
    while(!TestDestroy()) {
        if(mLoadQueue.GetCount()) {    // has ticket to do
            // pop the first ticket
            mQueueMutex.Lock();
            kuLoadTicket ticket = mLoadQueue[0];
            mLoadQueue.RemoveAt(0);
            mQueueMutex.Unlock();

            // do the ticket
            // check if exists
            wxLogDebug(self + wxT("Entry: getting lock for check"));
            mBmpMutex.Lock();
            wxLogDebug(self + wxT("Entry: got lock for check"));
            if(mBmpHash.find(ticket.Path) != mBmpHash.end()) {    // has it already
                if(mBmpHash[ticket.Path].Size == ticket.Size) {
                   mBmpMutex.Unlock();
                   wxLogDebug(self + wxT("Entry: unlock"));
                   continue;
                } else {    // kill it since the size is incorrect
                    EraseBmpHash(ticket.Path);
                }
            }
            mBmpMutex.Unlock();
            wxLogDebug(self + wxT("Entry: unlock"));

            // load bmp
            wxLogDebug(self + wxT("Entry: ")+ticket.Path);
            kuBmpValue value;
            value.Size = ticket.Size;
            value.Bmp  = kuFiWrapper::GetFiBitmap(ticket.Path, ticket.IsUrl, ticket.Size);

            // if queue is full, kill the first one
            wxLogDebug(self + wxT("Entry: getting lock for update"));
            mBmpMutex.Lock();
            wxLogDebug(self + wxT("Entry: got lock for update"));
            while(mBmpCache.GetCount() >= (size_t)wxGetApp().mOptions.mLoadCache) {
                wxLogDebug(self + wxT("Entry: remove ")+mBmpCache[0]);
                EraseBmpHash(mBmpCache[0]);
            }

            // put bmp into hash
            AddBmpHash(ticket.Path, value);
            mBmpMutex.Unlock();
            wxLogDebug(self + wxT("Entry: unlock"));

        } else    wxThread::Sleep(200);
    }
    ClearBmpHash();
    wxGetApp().mWaitLoader = false;
    wxLogDebug(self + wxT("Entry: exiting"));
    return NULL;
}

/*
   clear bmps in hash
*/
void kuLoadThread::ClearBmpHash() {
    if(mBmpHash.empty())    return;

    wxString self = THREAD_NAME_CURRENT;
    wxLogDebug(self + wxT("ClearBmpHash: getting lock"));
    mBmpMutex.Lock();
    wxLogDebug(self + wxT("ClearBmpHash: got lock"));

    for(size_t i=0; i<mBmpCache.GetCount(); i++) {
        FIBITMAP* bmp = mBmpHash[mBmpCache[i]].Bmp;
        if(bmp)    FreeImage_Unload(bmp);
    }

    mBmpHash.clear();
    mBmpCache.Clear();

    mBmpMutex.Unlock();
    wxLogDebug(self + wxT("ClearBmpHash: unlock"));
}

/*
   add a bmp in hash
*/
void kuLoadThread::AddBmpHash(wxString filename, kuBmpValue& value) {
    wxString self = THREAD_NAME_CURRENT;
    // delete old one if exists in hash
    if(mBmpHash.find(filename) != mBmpHash.end()) {
        EraseBmpHash(filename);
    }
    mBmpHash[filename] = value;

    // add to cache
    if(mBmpCache.Index(filename) != wxNOT_FOUND) {
        wxLogDebug(self + wxT("AddBmpHash: has it in cache already?!"));
    } else    mBmpCache.Add(filename);

    wxLogDebug(self + wxString::Format(wxT("AddBmpHash: added! #hash=%d, #cache=%d"), mBmpHash.size(), mBmpCache.GetCount()));
}

/*
   erase a bmp in hash
*/
void kuLoadThread::EraseBmpHash(wxString filename) {
    if(mBmpHash.find(filename) == mBmpHash.end())    return;

    wxString self = THREAD_NAME_CURRENT;
    FIBITMAP* bmp = mBmpHash[filename].Bmp;
    // unload it if exists
    if(bmp) {
        if(mBmpCache.Index(filename) == wxNOT_FOUND)    wxLogDebug(self + wxT("EraseBmpHash: not in cache?!"));
        FreeImage_Unload(bmp);
        wxLogDebug(self + wxString::Format(wxT("EraseBmpHash: erased! #hash=%d, #cache=%d"), mBmpHash.size(), mBmpCache.GetCount()));
    } else {
        wxLogDebug(self + wxT("EraseBmpHash: is null?!"));
    }
    // erase it
    mBmpHash.erase(filename);

    // remove from cache
    mBmpCache.Remove(filename);
    while(mBmpCache.Index(filename) != wxNOT_FOUND) {    // sometimes it is not removed...
        wxLogDebug(self + wxT("EraseBmpHash: is not removed from cache?!"));
        mBmpCache.Remove(filename);
    }
}

/*
   Clear() is used by UI thread
   clear ticket queue
*/
bool kuLoadThread::Clear() {
    mQueueMutex.Lock();
    mLoadQueue.Clear();
    mQueueMutex.Unlock();
    return true;
}

/*
   Append() is used by UI thread
   append ticket to prefetch bmp
*/
bool kuLoadThread::Append(wxString filename, bool isurl, int size) {
    // create ticket
    kuLoadTicket ticket;
    ticket.Path  = filename;
    ticket.IsUrl = isurl;
    ticket.Size  = size;
    // append ticket to queue
    mQueueMutex.Lock();
    mLoadQueue.Add(ticket);
    mQueueMutex.Unlock();
    return true;
}

/*
   Replace() is used by UI thread
   after rotate/scale bmp, should replace the one in hash.
*/
bool kuLoadThread::Replace(wxString filename, FIBITMAP* bmp) {
    if(mBmpHash.find(filename) == mBmpHash.end())    return false;

    wxString self = THREAD_NAME_CURRENT;
    wxLogDebug(self + wxT("Replace: getting lock"));
    mBmpMutex.Lock();
    wxLogDebug(self + wxT("Replace: got lock"));
    FIBITMAP* tmp = mBmpHash[filename].Bmp;
    // unload it if exists
    if(tmp)    FreeImage_Unload(tmp);
    // clone it
    mBmpHash[filename].Bmp = FreeImage_Clone(bmp);
    mBmpMutex.Unlock();
    wxLogDebug(self + wxT("Replace: unlock"));
    return true;
}

/*
   GetFiBitmap() is used by UI thread
   try to get it from hash. load and put into hash if not found.
*/
FIBITMAP* kuLoadThread::GetFiBitmap(wxString filename, bool isurl, int size) {
    FIBITMAP* bmp = NULL;
    bool hit = false;

    wxString self = THREAD_NAME_CURRENT;
    wxLogDebug(self + wxString::Format(wxT("GetFiBitmap: getting %s, %d"), filename.wc_str(), size));

    // search it
    wxLogDebug(self + wxT("GetFiBitmap: getting lock for check"));
    mBmpMutex.Lock();
    wxLogDebug(self + wxT("GetFiBitmap: got lock for check"));
    if(mBmpHash.find(filename) != mBmpHash.end()) {
        // kill old if its size is not correct
        if(mBmpHash[filename].Size == size)    hit = true;
        else    EraseBmpHash(filename);
    }

    if(hit) {
        wxLogDebug(self + wxT("GetFiBitmap: hit! ")+filename);
        bmp = mBmpHash[filename].Bmp;
        // move it to end in cache
        mBmpCache.Remove(filename);
        mBmpCache.Add(filename);
    } else {
        mBmpMutex.Unlock();
        wxLogDebug(self + wxT("GetFiBitmap: unlock"));
        wxLogDebug(self + wxString::Format(wxT("GetFiBitmap: miss! hash size = %d"), mBmpHash.size()));
        // load it
        bmp = kuFiWrapper::GetFiBitmap(filename, isurl, size);
        kuBmpValue value;
        value.Size = size;
        value.Bmp  = bmp;

        // put it
        wxLogDebug(self + wxT("GetFiBitmap: getting lock for update"));
        mBmpMutex.Lock();
        wxLogDebug(self + wxT("GetFiBitmap: got lock for update"));
        AddBmpHash(filename, value);
    }
    FIBITMAP* clone = FreeImage_Clone(bmp);
    mBmpMutex.Unlock();
    wxLogDebug(self + wxT("GetFiBitmap: unlock"));
    // return a clone bmp. UI thread have to manage by itself
    return clone;
}
