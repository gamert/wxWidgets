/////////////////////////////////////////////////////////////////////////////
// Name:        string.cpp
// Purpose:     wxString class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
  #pragma implementation "string.h"
#endif

/*
 * About ref counting:
 *  1) all empty strings use g_strEmpty, nRefs = -1 (set in Init())
 *  2) AllocBuffer() sets nRefs to 1, Lock() increments it by one
 *  3) Unlock() decrements nRefs and frees memory if it goes to 0
 */

// ===========================================================================
// headers, declarations, constants
// ===========================================================================

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#ifndef WX_PRECOMP
  #include "wx/defs.h"
  #include "wx/string.h"
  #include "wx/intl.h"
  #include "wx/thread.h"
#endif

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef __SALFORDC__
  #include <clib.h>
#endif

#if wxUSE_WCSRTOMBS
  #include <wchar.h>    // for wcsrtombs(), see comments where it's used
#endif // GNU

#ifdef  WXSTRING_IS_WXOBJECT
  IMPLEMENT_DYNAMIC_CLASS(wxString, wxObject)
#endif  //WXSTRING_IS_WXOBJECT

#if wxUSE_UNICODE
#undef wxUSE_EXPERIMENTAL_PRINTF
#define wxUSE_EXPERIMENTAL_PRINTF 1
#endif

// allocating extra space for each string consumes more memory but speeds up
// the concatenation operations (nLen is the current string's length)
// NB: EXTRA_ALLOC must be >= 0!
#define EXTRA_ALLOC       (19 - nLen % 16)

// ---------------------------------------------------------------------------
// static class variables definition
// ---------------------------------------------------------------------------

#ifdef  wxSTD_STRING_COMPATIBILITY
  const size_t wxString::npos = wxSTRING_MAXLEN;
#endif // wxSTD_STRING_COMPATIBILITY

// ----------------------------------------------------------------------------
// static data
// ----------------------------------------------------------------------------

// for an empty string, GetStringData() will return this address: this
// structure has the same layout as wxStringData and it's data() method will
// return the empty string (dummy pointer)
static const struct
{
  wxStringData data;
  wxChar dummy;
} g_strEmpty = { {-1, 0, 0}, wxT('\0') };

#if defined(__VISAGECPP__) && __IBMCPP__ >= 400
// must define this static for VA or else you get multiply defined symbols everywhere
const unsigned int wxSTRING_MAXLEN = UINT_MAX - 100;

#endif

// empty C style string: points to 'string data' byte of g_strEmpty
extern const wxChar WXDLLEXPORT *wxEmptyString = &g_strEmpty.dummy;

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

#if !defined(__WXSW__) && wxUSE_UNICODE
  #ifdef wxUSE_EXPERIMENTAL_PRINTF
    #undef wxUSE_EXPERIMENTAL_PRINTF
  #endif
  #define wxUSE_EXPERIMENTAL_PRINTF 1
#endif

// we want to find out if the current platform supports vsnprintf()-like
// function: for Unix this is done with configure, for Windows we test the
// compiler explicitly.
//
// FIXME currently, this is only for ANSI (!Unicode) strings, so we call this
//       function wxVsnprintfA (A for ANSI), should also find one for Unicode
//       strings in Unicode build
#ifdef __WXMSW__
    #if defined(__VISUALC__) || (defined(__MINGW32__) && wxUSE_NORLANDER_HEADERS)
        #define wxVsnprintfA     _vsnprintf
    #endif
#else   // !Windows
    #ifdef HAVE_VSNPRINTF
        #define wxVsnprintfA       vsnprintf
    #endif
#endif  // Windows/!Windows

#ifndef wxVsnprintfA
    // in this case we'll use vsprintf() (which is ANSI and thus should be
    // always available), but it's unsafe because it doesn't check for buffer
    // size - so give a warning
    #define wxVsnprintfA(buf, len, format, arg) vsprintf(buf, format, arg)

    #if defined(__VISUALC__)
        #pragma message("Using sprintf() because no snprintf()-like function defined")
    #elif defined(__GNUG__) && !defined(__UNIX__)
        #warning "Using sprintf() because no snprintf()-like function defined"
    #elif defined(__MWERKS__)
        #warning "Using sprintf() because no snprintf()-like function defined"
    #endif //compiler
#endif // no vsnprintf

#ifdef _AIX
  // AIX has vsnprintf, but there's no prototype in the system headers.
  extern "C" int vsnprintf(char* str, size_t n, const char* format, va_list ap);
#endif

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

#if defined(wxSTD_STRING_COMPATIBILITY) && wxUSE_STD_IOSTREAM

// MS Visual C++ version 5.0 provides the new STL headers as well as the old
// iostream ones.
//
// ATTN: you can _not_ use both of these in the same program!

istream& operator>>(istream& is, wxString& WXUNUSED(str))
{
#if 0
  int w = is.width(0);
  if ( is.ipfx(0) ) {
    streambuf *sb = is.rdbuf();
    str.erase();
    while ( true ) {
      int ch = sb->sbumpc ();
      if ( ch == EOF ) {
        is.setstate(ios::eofbit);
        break;
      }
      else if ( isspace(ch) ) {
        sb->sungetc();
        break;
      }

      str += ch;
      if ( --w == 1 )
        break;
    }
  }

  is.isfx();
  if ( str.length() == 0 )
    is.setstate(ios::failbit);
#endif
  return is;
}

ostream& operator<<(ostream& os, const wxString& str)
{
  os << str.c_str();
  return os;
}

#endif  //std::string compatibility

extern int WXDLLEXPORT wxVsnprintf(wxChar *buf, size_t len,
                                   const wxChar *format, va_list argptr)
{
#if wxUSE_UNICODE
    // FIXME should use wvsnprintf() or whatever if it's available
    wxString s;
    int iLen = s.PrintfV(format, argptr);
    if ( iLen != -1 )
    {
        wxStrncpy(buf, s.c_str(), iLen);
    }

    return iLen;
#else // ANSI
    // vsnprintf() will not terminate the string with '\0' if there is not
    // enough place, but we want the string to always be NUL terminated
    int rc = wxVsnprintfA(buf, len - 1, format, argptr);
    if ( rc == -1 )
    {
        buf[len] = 0;
    }

    return rc;
#endif // Unicode/ANSI
}

extern int WXDLLEXPORT wxSnprintf(wxChar *buf, size_t len,
                                  const wxChar *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    int iLen = wxVsnprintf(buf, len, format, argptr);

    va_end(argptr);

    return iLen;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this small class is used to gather statistics for performance tuning
//#define WXSTRING_STATISTICS
#ifdef  WXSTRING_STATISTICS
  class Averager
  {
  public:
    Averager(const char *sz) { m_sz = sz; m_nTotal = m_nCount = 0; }
   ~Averager()
   { printf("wxString: average %s = %f\n", m_sz, ((float)m_nTotal)/m_nCount); }

    void Add(size_t n) { m_nTotal += n; m_nCount++; }

  private:
    size_t m_nCount, m_nTotal;
    const char *m_sz;
  } g_averageLength("allocation size"),
    g_averageSummandLength("summand length"),
    g_averageConcatHit("hit probability in concat"),
    g_averageInitialLength("initial string length");

  #define STATISTICS_ADD(av, val) g_average##av.Add(val)
#else
  #define STATISTICS_ADD(av, val)
#endif // WXSTRING_STATISTICS

// ===========================================================================
// wxString class core
// ===========================================================================

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

// constructs string of <nLength> copies of character <ch>
wxString::wxString(wxChar ch, size_t nLength)
{
  Init();

  if ( nLength > 0 ) {
    AllocBuffer(nLength);

#if wxUSE_UNICODE
    // memset only works on char
    for (size_t n=0; n<nLength; n++) m_pchData[n] = ch;
#else
    memset(m_pchData, ch, nLength);
#endif
  }
}

// takes nLength elements of psz starting at nPos
void wxString::InitWith(const wxChar *psz, size_t nPos, size_t nLength)
{
  Init();

  // if the length is not given, assume the string to be NUL terminated
  if ( nLength == wxSTRING_MAXLEN ) {
    wxASSERT_MSG( nPos <= wxStrlen(psz), _T("index out of bounds") );

    nLength = wxStrlen(psz + nPos);
  }

  STATISTICS_ADD(InitialLength, nLength);

  if ( nLength > 0 ) {
    // trailing '\0' is written in AllocBuffer()
    AllocBuffer(nLength);
    memcpy(m_pchData, psz + nPos, nLength*sizeof(wxChar));
  }
}

#ifdef  wxSTD_STRING_COMPATIBILITY

// poor man's iterators are "void *" pointers
wxString::wxString(const void *pStart, const void *pEnd)
{
  InitWith((const wxChar *)pStart, 0,
           (const wxChar *)pEnd - (const wxChar *)pStart);
}

#endif  //std::string compatibility

#if wxUSE_UNICODE

// from multibyte string
wxString::wxString(const char *psz, wxMBConv& conv, size_t nLength)
{
  // first get necessary size
  size_t nLen = psz ? conv.MB2WC((wchar_t *) NULL, psz, 0) : 0;

  // nLength is number of *Unicode* characters here!
  if ((nLen != (size_t)-1) && (nLen > nLength))
    nLen = nLength;

  // empty?
  if ( (nLen != 0) && (nLen != (size_t)-1) ) {
    AllocBuffer(nLen);
    conv.MB2WC(m_pchData, psz, nLen);
  }
  else {
    Init();
  }
}

#else // ANSI

#if wxUSE_WCHAR_T
// from wide string
wxString::wxString(const wchar_t *pwz, wxMBConv& conv)
{
  // first get necessary size
  size_t nLen = pwz ? conv.WC2MB((char *) NULL, pwz, 0) : 0;

  // empty?
  if ( (nLen != 0) && (nLen != (size_t)-1) ) {
    AllocBuffer(nLen);
    conv.WC2MB(m_pchData, pwz, nLen);
  }
  else {
    Init();
  }
}
#endif // wxUSE_WCHAR_T

#endif // Unicode/ANSI

// ---------------------------------------------------------------------------
// memory allocation
// ---------------------------------------------------------------------------

// allocates memory needed to store a C string of length nLen
void wxString::AllocBuffer(size_t nLen)
{
  // allocating 0 sized buffer doesn't make sense, all empty strings should
  // reuse g_strEmpty
  wxASSERT( nLen >  0 );

  // make sure that we don't overflow
  wxASSERT( nLen < (INT_MAX / sizeof(wxChar)) -
                   (sizeof(wxStringData) + EXTRA_ALLOC + 1) );

  STATISTICS_ADD(Length, nLen);

  // allocate memory:
  // 1) one extra character for '\0' termination
  // 2) sizeof(wxStringData) for housekeeping info
  wxStringData* pData = (wxStringData*)
    malloc(sizeof(wxStringData) + (nLen + EXTRA_ALLOC + 1)*sizeof(wxChar));
  pData->nRefs        = 1;
  pData->nDataLength  = nLen;
  pData->nAllocLength = nLen + EXTRA_ALLOC;
  m_pchData           = pData->data();  // data starts after wxStringData
  m_pchData[nLen]     = wxT('\0');
}

// must be called before changing this string
void wxString::CopyBeforeWrite()
{
  wxStringData* pData = GetStringData();

  if ( pData->IsShared() ) {
    pData->Unlock();                // memory not freed because shared
    size_t nLen = pData->nDataLength;
    AllocBuffer(nLen);
    memcpy(m_pchData, pData->data(), nLen*sizeof(wxChar));
  }

  wxASSERT( !GetStringData()->IsShared() );  // we must be the only owner
}

// must be called before replacing contents of this string
void wxString::AllocBeforeWrite(size_t nLen)
{
  wxASSERT( nLen != 0 );  // doesn't make any sense

  // must not share string and must have enough space
  wxStringData* pData = GetStringData();
  if ( pData->IsShared() || pData->IsEmpty() ) {
    // can't work with old buffer, get new one
    pData->Unlock();
    AllocBuffer(nLen);
  }
  else {
    if ( nLen > pData->nAllocLength ) {
      // realloc the buffer instead of calling malloc() again, this is more
      // efficient
      STATISTICS_ADD(Length, nLen);

      nLen += EXTRA_ALLOC;

      wxStringData *pDataOld = pData;
      pData = (wxStringData*)
          realloc(pData, sizeof(wxStringData) + (nLen + 1)*sizeof(wxChar));
      if ( !pData ) {
        // out of memory
        free(pDataOld);

        // FIXME we're going to crash...
        return;
      }

      pData->nAllocLength = nLen;
      m_pchData = pData->data();
    }

    // now we have enough space, just update the string length
    pData->nDataLength = nLen;
  }

  wxASSERT( !GetStringData()->IsShared() );  // we must be the only owner
}

// allocate enough memory for nLen characters
void wxString::Alloc(size_t nLen)
{
  wxStringData *pData = GetStringData();
  if ( pData->nAllocLength <= nLen ) {
    if ( pData->IsEmpty() ) {
      nLen += EXTRA_ALLOC;

      wxStringData* pData = (wxStringData*)
        malloc(sizeof(wxStringData) + (nLen + 1)*sizeof(wxChar));
      pData->nRefs = 1;
      pData->nDataLength = 0;
      pData->nAllocLength = nLen;
      m_pchData = pData->data();  // data starts after wxStringData
      m_pchData[0u] = wxT('\0');
    }
    else if ( pData->IsShared() ) {
      pData->Unlock();                // memory not freed because shared
      size_t nOldLen = pData->nDataLength;
      AllocBuffer(nLen);
      memcpy(m_pchData, pData->data(), nOldLen*sizeof(wxChar));
    }
    else {
      nLen += EXTRA_ALLOC;

      wxStringData *pDataOld = pData;
      wxStringData *p = (wxStringData *)
        realloc(pData, sizeof(wxStringData) + (nLen + 1)*sizeof(wxChar));

      if ( p == NULL ) {
        // don't leak memory
        free(pDataOld);

        // FIXME what to do on memory error?
        return;
      }

      // it's not important if the pointer changed or not (the check for this
      // is not faster than assigning to m_pchData in all cases)
      p->nAllocLength = nLen;
      m_pchData = p->data();
    }
  }
  //else: we've already got enough
}

// shrink to minimal size (releasing extra memory)
void wxString::Shrink()
{
  wxStringData *pData = GetStringData();

  // this variable is unused in release build, so avoid the compiler warning
  // by just not declaring it
#ifdef __WXDEBUG__
  void *p =
#endif
  realloc(pData, sizeof(wxStringData) + (pData->nDataLength + 1)*sizeof(wxChar));

  // we rely on a reasonable realloc() implementation here - so far I haven't
  // seen any which wouldn't behave like this

  wxASSERT( p != NULL );  // can't free memory?
  wxASSERT( p == pData ); // we're decrementing the size - block shouldn't move!
}

// get the pointer to writable buffer of (at least) nLen bytes
wxChar *wxString::GetWriteBuf(size_t nLen)
{
  AllocBeforeWrite(nLen);

  wxASSERT( GetStringData()->nRefs == 1 );
  GetStringData()->Validate(FALSE);

  return m_pchData;
}

// put string back in a reasonable state after GetWriteBuf
void wxString::UngetWriteBuf()
{
  GetStringData()->nDataLength = wxStrlen(m_pchData);
  GetStringData()->Validate(TRUE);
}

void wxString::UngetWriteBuf(size_t nLen)
{
  GetStringData()->nDataLength = nLen;
  GetStringData()->Validate(TRUE);
}

// ---------------------------------------------------------------------------
// data access
// ---------------------------------------------------------------------------

// all functions are inline in string.h

// ---------------------------------------------------------------------------
// assignment operators
// ---------------------------------------------------------------------------

// helper function: does real copy
void wxString::AssignCopy(size_t nSrcLen, const wxChar *pszSrcData)
{
  if ( nSrcLen == 0 ) {
    Reinit();
  }
  else {
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, pszSrcData, nSrcLen*sizeof(wxChar));
    GetStringData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = wxT('\0');
  }
}

// assigns one string to another
wxString& wxString::operator=(const wxString& stringSrc)
{
  wxASSERT( stringSrc.GetStringData()->IsValid() );

  // don't copy string over itself
  if ( m_pchData != stringSrc.m_pchData ) {
    if ( stringSrc.GetStringData()->IsEmpty() ) {
      Reinit();
    }
    else {
      // adjust references
      GetStringData()->Unlock();
      m_pchData = stringSrc.m_pchData;
      GetStringData()->Lock();
    }
  }

  return *this;
}

// assigns a single character
wxString& wxString::operator=(wxChar ch)
{
  AssignCopy(1, &ch);
  return *this;
}

// assigns C string
wxString& wxString::operator=(const wxChar *psz)
{
  AssignCopy(wxStrlen(psz), psz);
  return *this;
}

#if !wxUSE_UNICODE

// same as 'signed char' variant
wxString& wxString::operator=(const unsigned char* psz)
{
  *this = (const char *)psz;
  return *this;
}

#if wxUSE_WCHAR_T
wxString& wxString::operator=(const wchar_t *pwz)
{
  wxString str(pwz);
  *this = str;
  return *this;
}
#endif

#endif

// ---------------------------------------------------------------------------
// string concatenation
// ---------------------------------------------------------------------------

// add something to this string
void wxString::ConcatSelf(int nSrcLen, const wxChar *pszSrcData)
{
  STATISTICS_ADD(SummandLength, nSrcLen);

  // concatenating an empty string is a NOP
  if ( nSrcLen > 0 ) {
    wxStringData *pData = GetStringData();
    size_t nLen = pData->nDataLength;
    size_t nNewLen = nLen + nSrcLen;

    // alloc new buffer if current is too small
    if ( pData->IsShared() ) {
      STATISTICS_ADD(ConcatHit, 0);

      // we have to allocate another buffer
      wxStringData* pOldData = GetStringData();
      AllocBuffer(nNewLen);
      memcpy(m_pchData, pOldData->data(), nLen*sizeof(wxChar));
      pOldData->Unlock();
    }
    else if ( nNewLen > pData->nAllocLength ) {
      STATISTICS_ADD(ConcatHit, 0);

      // we have to grow the buffer
      Alloc(nNewLen);
    }
    else {
      STATISTICS_ADD(ConcatHit, 1);

      // the buffer is already big enough
    }

    // should be enough space
    wxASSERT( nNewLen <= GetStringData()->nAllocLength );

    // fast concatenation - all is done in our buffer
    memcpy(m_pchData + nLen, pszSrcData, nSrcLen*sizeof(wxChar));

    m_pchData[nNewLen] = wxT('\0');          // put terminating '\0'
    GetStringData()->nDataLength = nNewLen; // and fix the length
  }
  //else: the string to append was empty
}

/*
 * concatenation functions come in 5 flavours:
 *  string + string
 *  char   + string      and      string + char
 *  C str  + string      and      string + C str
 */

wxString operator+(const wxString& string1, const wxString& string2)
{
  wxASSERT( string1.GetStringData()->IsValid() );
  wxASSERT( string2.GetStringData()->IsValid() );

  wxString s = string1;
  s += string2;

  return s;
}

wxString operator+(const wxString& string, wxChar ch)
{
  wxASSERT( string.GetStringData()->IsValid() );

  wxString s = string;
  s += ch;

  return s;
}

wxString operator+(wxChar ch, const wxString& string)
{
  wxASSERT( string.GetStringData()->IsValid() );

  wxString s = ch;
  s += string;

  return s;
}

wxString operator+(const wxString& string, const wxChar *psz)
{
  wxASSERT( string.GetStringData()->IsValid() );

  wxString s;
  s.Alloc(wxStrlen(psz) + string.Len());
  s = string;
  s += psz;

  return s;
}

wxString operator+(const wxChar *psz, const wxString& string)
{
  wxASSERT( string.GetStringData()->IsValid() );

  wxString s;
  s.Alloc(wxStrlen(psz) + string.Len());
  s = psz;
  s += string;

  return s;
}

// ===========================================================================
// other common string functions
// ===========================================================================

// ---------------------------------------------------------------------------
// simple sub-string extraction
// ---------------------------------------------------------------------------

// helper function: clone the data attached to this string
void wxString::AllocCopy(wxString& dest, int nCopyLen, int nCopyIndex) const
{
  if ( nCopyLen == 0 ) {
    dest.Init();
  }
  else {
    dest.AllocBuffer(nCopyLen);
    memcpy(dest.m_pchData, m_pchData + nCopyIndex, nCopyLen*sizeof(wxChar));
  }
}

// extract string of length nCount starting at nFirst
wxString wxString::Mid(size_t nFirst, size_t nCount) const
{
  wxStringData *pData = GetStringData();
  size_t nLen = pData->nDataLength;

  // default value of nCount is wxSTRING_MAXLEN and means "till the end"
  if ( nCount == wxSTRING_MAXLEN )
  {
    nCount = nLen - nFirst;
  }

  // out-of-bounds requests return sensible things
  if ( nFirst + nCount > nLen )
  {
    nCount = nLen - nFirst;
  }

  if ( nFirst > nLen )
  {
    // AllocCopy() will return empty string
    nCount = 0;
  }

  wxString dest;
  AllocCopy(dest, nCount, nFirst);

  return dest;
}

// check that the tring starts with prefix and return the rest of the string
// in the provided pointer if it is not NULL, otherwise return FALSE
bool wxString::StartsWith(const wxChar *prefix, wxString *rest) const
{
    wxASSERT_MSG( prefix, _T("invalid parameter in wxString::StartsWith") );

    // first check if the beginning of the string matches the prefix: note
    // that we don't have to check that we don't run out of this string as
    // when we reach the terminating NUL, either prefix string ends too (and
    // then it's ok) or we break out of the loop because there is no match
    const wxChar *p = c_str();
    while ( *prefix )
    {
        if ( *prefix++ != *p++ )
        {
            // no match
            return FALSE;
        }
    }

    if ( rest )
    {
        // put the rest of the string into provided pointer
        *rest = p;
    }

    return TRUE;
}

// extract nCount last (rightmost) characters
wxString wxString::Right(size_t nCount) const
{
  if ( nCount > (size_t)GetStringData()->nDataLength )
    nCount = GetStringData()->nDataLength;

  wxString dest;
  AllocCopy(dest, nCount, GetStringData()->nDataLength - nCount);
  return dest;
}

// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
wxString wxString::AfterLast(wxChar ch) const
{
  wxString str;
  int iPos = Find(ch, TRUE);
  if ( iPos == wxNOT_FOUND )
    str = *this;
  else
    str = c_str() + iPos + 1;

  return str;
}

// extract nCount first (leftmost) characters
wxString wxString::Left(size_t nCount) const
{
  if ( nCount > (size_t)GetStringData()->nDataLength )
    nCount = GetStringData()->nDataLength;

  wxString dest;
  AllocCopy(dest, nCount, 0);
  return dest;
}

// get all characters before the first occurence of ch
// (returns the whole string if ch not found)
wxString wxString::BeforeFirst(wxChar ch) const
{
  wxString str;
  for ( const wxChar *pc = m_pchData; *pc != wxT('\0') && *pc != ch; pc++ )
    str += *pc;

  return str;
}

/// get all characters before the last occurence of ch
/// (returns empty string if ch not found)
wxString wxString::BeforeLast(wxChar ch) const
{
  wxString str;
  int iPos = Find(ch, TRUE);
  if ( iPos != wxNOT_FOUND && iPos != 0 )
    str = wxString(c_str(), iPos);

  return str;
}

/// get all characters after the first occurence of ch
/// (returns empty string if ch not found)
wxString wxString::AfterFirst(wxChar ch) const
{
  wxString str;
  int iPos = Find(ch);
  if ( iPos != wxNOT_FOUND )
    str = c_str() + iPos + 1;

  return str;
}

// replace first (or all) occurences of some substring with another one
size_t wxString::Replace(const wxChar *szOld, const wxChar *szNew, bool bReplaceAll)
{
  size_t uiCount = 0;   // count of replacements made

  size_t uiOldLen = wxStrlen(szOld);

  wxString strTemp;
  const wxChar *pCurrent = m_pchData;
  const wxChar *pSubstr;
  while ( *pCurrent != wxT('\0') ) {
    pSubstr = wxStrstr(pCurrent, szOld);
    if ( pSubstr == NULL ) {
      // strTemp is unused if no replacements were made, so avoid the copy
      if ( uiCount == 0 )
        return 0;

      strTemp += pCurrent;    // copy the rest
      break;                  // exit the loop
    }
    else {
      // take chars before match
      strTemp.ConcatSelf(pSubstr - pCurrent, pCurrent);
      strTemp += szNew;
      pCurrent = pSubstr + uiOldLen;  // restart after match

      uiCount++;

      // stop now?
      if ( !bReplaceAll ) {
        strTemp += pCurrent;    // copy the rest
        break;                  // exit the loop
      }
    }
  }

  // only done if there were replacements, otherwise would have returned above
  *this = strTemp;

  return uiCount;
}

bool wxString::IsAscii() const
{
  const wxChar *s = (const wxChar*) *this;
  while(*s){
    if(!isascii(*s)) return(FALSE);
    s++;
  }
  return(TRUE);
}

bool wxString::IsWord() const
{
  const wxChar *s = (const wxChar*) *this;
  while(*s){
    if(!wxIsalpha(*s)) return(FALSE);
    s++;
  }
  return(TRUE);
}

bool wxString::IsNumber() const
{
  const wxChar *s = (const wxChar*) *this;
  if (wxStrlen(s))
     if ((s[0] == '-') || (s[0] == '+')) s++;
  while(*s){
    if(!wxIsdigit(*s)) return(FALSE);
    s++;
  }
  return(TRUE);
}

wxString wxString::Strip(stripType w) const
{
    wxString s = *this;
    if ( w & leading ) s.Trim(FALSE);
    if ( w & trailing ) s.Trim(TRUE);
    return s;
}

// ---------------------------------------------------------------------------
// case conversion
// ---------------------------------------------------------------------------

wxString& wxString::MakeUpper()
{
  CopyBeforeWrite();

  for ( wxChar *p = m_pchData; *p; p++ )
    *p = (wxChar)wxToupper(*p);

  return *this;
}

wxString& wxString::MakeLower()
{
  CopyBeforeWrite();

  for ( wxChar *p = m_pchData; *p; p++ )
    *p = (wxChar)wxTolower(*p);

  return *this;
}

// ---------------------------------------------------------------------------
// trimming and padding
// ---------------------------------------------------------------------------

// trims spaces (in the sense of isspace) from left or right side
wxString& wxString::Trim(bool bFromRight)
{
  // first check if we're going to modify the string at all
  if ( !IsEmpty() &&
       (
        (bFromRight && wxIsspace(GetChar(Len() - 1))) ||
        (!bFromRight && wxIsspace(GetChar(0u)))
       )
     )
  {
    // ok, there is at least one space to trim
    CopyBeforeWrite();

    if ( bFromRight )
    {
      // find last non-space character
      wxChar *psz = m_pchData + GetStringData()->nDataLength - 1;
      while ( wxIsspace(*psz) && (psz >= m_pchData) )
        psz--;

      // truncate at trailing space start
      *++psz = wxT('\0');
      GetStringData()->nDataLength = psz - m_pchData;
    }
    else
    {
      // find first non-space character
      const wxChar *psz = m_pchData;
      while ( wxIsspace(*psz) )
        psz++;

      // fix up data and length
      int nDataLength = GetStringData()->nDataLength - (psz - (const wxChar*) m_pchData);
      memmove(m_pchData, psz, (nDataLength + 1)*sizeof(wxChar));
      GetStringData()->nDataLength = nDataLength;
    }
  }

  return *this;
}

// adds nCount characters chPad to the string from either side
wxString& wxString::Pad(size_t nCount, wxChar chPad, bool bFromRight)
{
  wxString s(chPad, nCount);

  if ( bFromRight )
    *this += s;
  else
  {
    s += *this;
    *this = s;
  }

  return *this;
}

// truncate the string
wxString& wxString::Truncate(size_t uiLen)
{
  if ( uiLen < Len() ) {
    CopyBeforeWrite();

    *(m_pchData + uiLen) = wxT('\0');
    GetStringData()->nDataLength = uiLen;
  }
  //else: nothing to do, string is already short enough

  return *this;
}

// ---------------------------------------------------------------------------
// finding (return wxNOT_FOUND if not found and index otherwise)
// ---------------------------------------------------------------------------

// find a character
int wxString::Find(wxChar ch, bool bFromEnd) const
{
  const wxChar *psz = bFromEnd ? wxStrrchr(m_pchData, ch) : wxStrchr(m_pchData, ch);

  return (psz == NULL) ? wxNOT_FOUND : psz - (const wxChar*) m_pchData;
}

// find a sub-string (like strstr)
int wxString::Find(const wxChar *pszSub) const
{
  const wxChar *psz = wxStrstr(m_pchData, pszSub);

  return (psz == NULL) ? wxNOT_FOUND : psz - (const wxChar*) m_pchData;
}

// ----------------------------------------------------------------------------
// conversion to numbers
// ----------------------------------------------------------------------------

bool wxString::ToLong(long *val) const
{
    wxCHECK_MSG( val, FALSE, _T("NULL pointer in wxString::ToLong") );

    const wxChar *start = c_str();
    wxChar *end;
    *val = wxStrtol(start, &end, 10);

    // return TRUE only if scan was stopped by the terminating NUL and if the
    // string was not empty to start with
    return !*end && (end != start);
}

bool wxString::ToULong(unsigned long *val) const
{
    wxCHECK_MSG( val, FALSE, _T("NULL pointer in wxString::ToULong") );

    const wxChar *start = c_str();
    wxChar *end;
    *val = wxStrtoul(start, &end, 10);

    // return TRUE only if scan was stopped by the terminating NUL and if the
    // string was not empty to start with
    return !*end && (end != start);
}

bool wxString::ToDouble(double *val) const
{
    wxCHECK_MSG( val, FALSE, _T("NULL pointer in wxString::ToDouble") );

    const wxChar *start = c_str();
    wxChar *end;
    *val = wxStrtod(start, &end);

    // return TRUE only if scan was stopped by the terminating NUL and if the
    // string was not empty to start with
    return !*end && (end != start);
}

// ---------------------------------------------------------------------------
// formatted output
// ---------------------------------------------------------------------------

/* static */
wxString wxString::Format(const wxChar *pszFormat, ...)
{
    va_list argptr;
    va_start(argptr, pszFormat);

    wxString s;
    s.PrintfV(pszFormat, argptr);

    va_end(argptr);

    return s;
}

/* static */
wxString wxString::FormatV(const wxChar *pszFormat, va_list argptr)
{
    wxString s;
    s.Printf(pszFormat, argptr);
    return s;
}

int wxString::Printf(const wxChar *pszFormat, ...)
{
  va_list argptr;
  va_start(argptr, pszFormat);

  int iLen = PrintfV(pszFormat, argptr);

  va_end(argptr);

  return iLen;
}

int wxString::PrintfV(const wxChar* pszFormat, va_list argptr)
{
#if wxUSE_EXPERIMENTAL_PRINTF
  // the new implementation

  // buffer to avoid dynamic memory allocation each time for small strings
  char szScratch[1024];

  Reinit();
  for (size_t n = 0; pszFormat[n]; n++)
    if (pszFormat[n] == wxT('%')) {
      static char s_szFlags[256] = "%";
      size_t flagofs = 1;
      bool adj_left = FALSE, in_prec = FALSE,
           prec_dot = FALSE, done = FALSE;
      int ilen = 0;
      size_t min_width = 0, max_width = wxSTRING_MAXLEN;
      do {
#define CHECK_PREC if (in_prec && !prec_dot) { s_szFlags[flagofs++] = '.'; prec_dot = TRUE; }
        switch (pszFormat[++n]) {
        case wxT('\0'):
          done = TRUE;
          break;
        case wxT('%'):
          *this += wxT('%');
          done = TRUE;
          break;
        case wxT('#'):
        case wxT('0'):
        case wxT(' '):
        case wxT('+'):
        case wxT('\''):
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('-'):
          CHECK_PREC
          adj_left = TRUE;
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('.'):
          CHECK_PREC
          in_prec = TRUE;
          prec_dot = FALSE;
          max_width = 0;
          // dot will be auto-added to s_szFlags if non-negative number follows
          break;
        case wxT('h'):
          ilen = -1;
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('l'):
          ilen = 1;
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('q'):
        case wxT('L'):
          ilen = 2;
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('Z'):
          ilen = 3;
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          break;
        case wxT('*'):
          {
            int len = va_arg(argptr, int);
            if (in_prec) {
              if (len<0) break;
              CHECK_PREC
              max_width = len;
            } else {
              if (len<0) {
                adj_left = !adj_left;
                s_szFlags[flagofs++] = '-';
                len = -len;
              }
              min_width = len;
            }
            flagofs += ::sprintf(s_szFlags+flagofs,"%d",len);
          }
          break;
        case wxT('1'): case wxT('2'): case wxT('3'):
        case wxT('4'): case wxT('5'): case wxT('6'):
        case wxT('7'): case wxT('8'): case wxT('9'):
          {
            int len = 0;
            CHECK_PREC
            while ((pszFormat[n]>=wxT('0')) && (pszFormat[n]<=wxT('9'))) {
              s_szFlags[flagofs++] = pszFormat[n];
              len = len*10 + (pszFormat[n] - wxT('0'));
              n++;
            }
            if (in_prec) max_width = len;
            else min_width = len;
            n--; // the main loop pre-increments n again
          }
          break;
        case wxT('d'):
        case wxT('i'):
        case wxT('o'):
        case wxT('u'):
        case wxT('x'):
        case wxT('X'):
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          s_szFlags[flagofs] = '\0';
          if (ilen == 0 ) {
            int val = va_arg(argptr, int);
            ::sprintf(szScratch, s_szFlags, val);
          }
          else if (ilen == -1) {
            short int val = va_arg(argptr, short int);
            ::sprintf(szScratch, s_szFlags, val);
          }
          else if (ilen == 1) {
            long int val = va_arg(argptr, long int);
            ::sprintf(szScratch, s_szFlags, val);
          }
          else if (ilen == 2) {
#if SIZEOF_LONG_LONG
            long long int val = va_arg(argptr, long long int);
            ::sprintf(szScratch, s_szFlags, val);
#else
            long int val = va_arg(argptr, long int);
            ::sprintf(szScratch, s_szFlags, val);
#endif
          }
          else if (ilen == 3) {
            size_t val = va_arg(argptr, size_t);
            ::sprintf(szScratch, s_szFlags, val);
          }
          *this += wxString(szScratch);
          done = TRUE;
          break;
        case wxT('e'):
        case wxT('E'):
        case wxT('f'):
        case wxT('g'):
        case wxT('G'):
          CHECK_PREC
          s_szFlags[flagofs++] = pszFormat[n];
          s_szFlags[flagofs] = '\0';
          if (ilen == 2) {
            long double val = va_arg(argptr, long double);
            ::sprintf(szScratch, s_szFlags, val);
          } else {
            double val = va_arg(argptr, double);
            ::sprintf(szScratch, s_szFlags, val);
          }
          *this += wxString(szScratch);
          done = TRUE;
          break;
        case wxT('p'):
          {
            void *val = va_arg(argptr, void *);
            CHECK_PREC
            s_szFlags[flagofs++] = pszFormat[n];
            s_szFlags[flagofs] = '\0';
            ::sprintf(szScratch, s_szFlags, val);
            *this += wxString(szScratch);
            done = TRUE;
          }
          break;
        case wxT('c'):
          {
            wxChar val = va_arg(argptr, int);
            // we don't need to honor padding here, do we?
            *this += val;
            done = TRUE;
          }
          break;
        case wxT('s'):
          if (ilen == -1) {
            // wx extension: we'll let %hs mean non-Unicode strings
            char *val = va_arg(argptr, char *);
#if wxUSE_UNICODE
            // ASCII->Unicode constructor handles max_width right
            wxString s(val, wxConvLibc, max_width);
#else
            size_t len = wxSTRING_MAXLEN;
            if (val) {
              for (len = 0; val[len] && (len<max_width); len++);
            } else val = wxT("(null)");
            wxString s(val, len);
#endif
            if (s.Len() < min_width)
              s.Pad(min_width - s.Len(), wxT(' '), adj_left);
            *this += s;
          } else {
            wxChar *val = va_arg(argptr, wxChar *);
            size_t len = wxSTRING_MAXLEN;
            if (val) {
              for (len = 0; val[len] && (len<max_width); len++);
            } else val = wxT("(null)");
            wxString s(val, len);
            if (s.Len() < min_width)
              s.Pad(min_width - s.Len(), wxT(' '), adj_left);
            *this += s;
          }
          done = TRUE;
          break;
        case wxT('n'):
          if (ilen == 0) {
            int *val = va_arg(argptr, int *);
            *val = Len();
          }
          else if (ilen == -1) {
            short int *val = va_arg(argptr, short int *);
            *val = Len();
          }
          else if (ilen >= 1) {
            long int *val = va_arg(argptr, long int *);
            *val = Len();
          }
          done = TRUE;
          break;
        default:
          if (wxIsalpha(pszFormat[n]))
            // probably some flag not taken care of here yet
            s_szFlags[flagofs++] = pszFormat[n];
          else {
            // bad format
            *this += wxT('%'); // just to pass the glibc tst-printf.c
            n--;
            done = TRUE;
          }
          break;
        }
#undef CHECK_PREC
      } while (!done);
    } else *this += pszFormat[n];

#else
  // buffer to avoid dynamic memory allocation each time for small strings
  char szScratch[1024];

  // NB: wxVsnprintf() may return either less than the buffer size or -1 if
  //     there is not enough place depending on implementation
  int iLen = wxVsnprintfA(szScratch, WXSIZEOF(szScratch), pszFormat, argptr);
  if ( iLen != -1 ) {
    // the whole string is in szScratch
    *this = szScratch;
  }
  else {
      bool outOfMemory = FALSE;
      int size = 2*WXSIZEOF(szScratch);
      while ( !outOfMemory ) {
          char *buf = GetWriteBuf(size);
          if ( buf )
            iLen = wxVsnprintfA(buf, size, pszFormat, argptr);
          else
            outOfMemory = TRUE;

          UngetWriteBuf();

          if ( iLen != -1 ) {
              // ok, there was enough space
              break;
          }

          // still not enough, double it again
          size *= 2;
      }

      if ( outOfMemory ) {
          // out of memory
          return -1;
      }
  }
#endif // wxUSE_EXPERIMENTAL_PRINTF/!wxUSE_EXPERIMENTAL_PRINTF

  return Len();
}

// ----------------------------------------------------------------------------
// misc other operations
// ----------------------------------------------------------------------------

// returns TRUE if the string matches the pattern which may contain '*' and
// '?' metacharacters (as usual, '?' matches any character and '*' any number
// of them)
bool wxString::Matches(const wxChar *pszMask) const
{
  // check char by char
  const wxChar *pszTxt;
  for ( pszTxt = c_str(); *pszMask != wxT('\0'); pszMask++, pszTxt++ ) {
    switch ( *pszMask ) {
      case wxT('?'):
        if ( *pszTxt == wxT('\0') )
          return FALSE;

        // pszText and pszMask will be incremented in the loop statement

        break;

      case wxT('*'):
        {
          // ignore special chars immediately following this one
          while ( *pszMask == wxT('*') || *pszMask == wxT('?') )
            pszMask++;

          // if there is nothing more, match
          if ( *pszMask == wxT('\0') )
            return TRUE;

          // are there any other metacharacters in the mask?
          size_t uiLenMask;
          const wxChar *pEndMask = wxStrpbrk(pszMask, wxT("*?"));

          if ( pEndMask != NULL ) {
            // we have to match the string between two metachars
            uiLenMask = pEndMask - pszMask;
          }
          else {
            // we have to match the remainder of the string
            uiLenMask = wxStrlen(pszMask);
          }

          wxString strToMatch(pszMask, uiLenMask);
          const wxChar* pMatch = wxStrstr(pszTxt, strToMatch);
          if ( pMatch == NULL )
            return FALSE;

          // -1 to compensate "++" in the loop
          pszTxt = pMatch + uiLenMask - 1;
          pszMask += uiLenMask - 1;
        }
        break;

      default:
        if ( *pszMask != *pszTxt )
          return FALSE;
        break;
    }
  }

  // match only if nothing left
  return *pszTxt == wxT('\0');
}

// Count the number of chars
int wxString::Freq(wxChar ch) const
{
    int count = 0;
    int len = Len();
    for (int i = 0; i < len; i++)
    {
        if (GetChar(i) == ch)
            count ++;
    }
    return count;
}

// convert to upper case, return the copy of the string
wxString wxString::Upper() const
{ wxString s(*this); return s.MakeUpper(); }

// convert to lower case, return the copy of the string
wxString wxString::Lower() const { wxString s(*this); return s.MakeLower(); }

int wxString::sprintf(const wxChar *pszFormat, ...)
  {
    va_list argptr;
    va_start(argptr, pszFormat);
    int iLen = PrintfV(pszFormat, argptr);
    va_end(argptr);
    return iLen;
  }

// ---------------------------------------------------------------------------
// standard C++ library string functions
// ---------------------------------------------------------------------------
#ifdef  wxSTD_STRING_COMPATIBILITY

wxString& wxString::insert(size_t nPos, const wxString& str)
{
  wxASSERT( str.GetStringData()->IsValid() );
  wxASSERT( nPos <= Len() );

  if ( !str.IsEmpty() ) {
    wxString strTmp;
    wxChar *pc = strTmp.GetWriteBuf(Len() + str.Len());
    wxStrncpy(pc, c_str(), nPos);
    wxStrcpy(pc + nPos, str);
    wxStrcpy(pc + nPos + str.Len(), c_str() + nPos);
    strTmp.UngetWriteBuf();
    *this = strTmp;
  }

  return *this;
}

size_t wxString::find(const wxString& str, size_t nStart) const
{
  wxASSERT( str.GetStringData()->IsValid() );
  wxASSERT( nStart <= Len() );

  const wxChar *p = wxStrstr(c_str() + nStart, str);

  return p == NULL ? npos : p - c_str();
}

// VC++ 1.5 can't cope with the default argument in the header.
#if !defined(__VISUALC__) || defined(__WIN32__)
size_t wxString::find(const wxChar* sz, size_t nStart, size_t n) const
{
  return find(wxString(sz, n), nStart);
}
#endif // VC++ 1.5

// Gives a duplicate symbol (presumably a case-insensitivity problem)
#if !defined(__BORLANDC__)
size_t wxString::find(wxChar ch, size_t nStart) const
{
  wxASSERT( nStart <= Len() );

  const wxChar *p = wxStrchr(c_str() + nStart, ch);

  return p == NULL ? npos : p - c_str();
}
#endif

size_t wxString::rfind(const wxString& str, size_t nStart) const
{
  wxASSERT( str.GetStringData()->IsValid() );
  wxASSERT( nStart <= Len() );

  // TODO could be made much quicker than that
  const wxChar *p = c_str() + (nStart == npos ? Len() : nStart);
  while ( p >= c_str() + str.Len() ) {
    if ( wxStrncmp(p - str.Len(), str, str.Len()) == 0 )
      return p - str.Len() - c_str();
    p--;
  }

  return npos;
}

// VC++ 1.5 can't cope with the default argument in the header.
#if !defined(__VISUALC__) || defined(__WIN32__)
size_t wxString::rfind(const wxChar* sz, size_t nStart, size_t n) const
{
    return rfind(wxString(sz, n == npos ? 0 : n), nStart);
}

size_t wxString::rfind(wxChar ch, size_t nStart) const
{
    if ( nStart == npos )
    {
        nStart = Len();
    }
    else
    {
        wxASSERT( nStart <= Len() );
    }

    const wxChar *p = wxStrrchr(c_str(), ch);

    if ( p == NULL )
        return npos;

    size_t result = p - c_str();
    return ( result > nStart ) ? npos : result;
}
#endif // VC++ 1.5

size_t wxString::find_first_of(const wxChar* sz, size_t nStart) const
{
    const wxChar *start = c_str() + nStart;
    const wxChar *firstOf = wxStrpbrk(start, sz);
    if ( firstOf )
        return firstOf - c_str();
    else
        return npos;
}

size_t wxString::find_last_of(const wxChar* sz, size_t nStart) const
{
    if ( nStart == npos )
    {
        nStart = Len();
    }
    else
    {
        wxASSERT( nStart <= Len() );
    }

    for ( const wxChar *p = c_str() + length() - 1; p >= c_str(); p-- )
    {
        if ( wxStrchr(sz, *p) )
            return p - c_str();
    }

    return npos;
}

size_t wxString::find_first_not_of(const wxChar* sz, size_t nStart) const
{
    if ( nStart == npos )
    {
        nStart = Len();
    }
    else
    {
        wxASSERT( nStart <= Len() );
    }

    size_t nAccept = wxStrspn(c_str() + nStart, sz);
    if ( nAccept >= length() - nStart )
        return npos;
    else
        return nAccept;
}

size_t wxString::find_first_not_of(wxChar ch, size_t nStart) const
{
    wxASSERT( nStart <= Len() );

    for ( const wxChar *p = c_str() + nStart; *p; p++ )
    {
        if ( *p != ch )
            return p - c_str();
    }

    return npos;
}

size_t wxString::find_last_not_of(const wxChar* sz, size_t nStart) const
{
    if ( nStart == npos )
    {
        nStart = Len();
    }
    else
    {
        wxASSERT( nStart <= Len() );
    }

    for ( const wxChar *p = c_str() + nStart - 1; p >= c_str(); p-- )
    {
        if ( !wxStrchr(sz, *p) )
            return p - c_str();
    }

    return npos;
}

size_t wxString::find_last_not_of(wxChar ch, size_t nStart) const
{
    if ( nStart == npos )
    {
        nStart = Len();
    }
    else
    {
        wxASSERT( nStart <= Len() );
    }

    for ( const wxChar *p = c_str() + nStart - 1; p >= c_str(); p-- )
    {
        if ( *p != ch )
            return p - c_str();
    }

    return npos;
}

wxString& wxString::erase(size_t nStart, size_t nLen)
{
  wxString strTmp(c_str(), nStart);
  if ( nLen != npos ) {
    wxASSERT( nStart + nLen <= Len() );

    strTmp.append(c_str() + nStart + nLen);
  }

  *this = strTmp;
  return *this;
}

wxString& wxString::replace(size_t nStart, size_t nLen, const wxChar *sz)
{
  wxASSERT_MSG( nStart + nLen <= Len(),
                _T("index out of bounds in wxString::replace") );

  wxString strTmp;
  strTmp.Alloc(Len());      // micro optimisation to avoid multiple mem allocs

  if ( nStart != 0 )
    strTmp.append(c_str(), nStart);
  strTmp << sz << c_str() + nStart + nLen;

  *this = strTmp;
  return *this;
}

wxString& wxString::replace(size_t nStart, size_t nLen, size_t nCount, wxChar ch)
{
  return replace(nStart, nLen, wxString(ch, nCount));
}

wxString& wxString::replace(size_t nStart, size_t nLen,
                            const wxString& str, size_t nStart2, size_t nLen2)
{
  return replace(nStart, nLen, str.substr(nStart2, nLen2));
}

wxString& wxString::replace(size_t nStart, size_t nLen,
                        const wxChar* sz, size_t nCount)
{
  return replace(nStart, nLen, wxString(sz, nCount));
}

#endif  //std::string compatibility

// ============================================================================
// ArrayString
// ============================================================================

// size increment = max(50% of current size, ARRAY_MAXSIZE_INCREMENT)
#define   ARRAY_MAXSIZE_INCREMENT       4096
#ifndef   ARRAY_DEFAULT_INITIAL_SIZE    // also defined in dynarray.h
  #define   ARRAY_DEFAULT_INITIAL_SIZE    (16)
#endif

#define   STRING(p)   ((wxString *)(&(p)))

// ctor
wxArrayString::wxArrayString(bool autoSort)
{
  m_nSize  =
  m_nCount = 0;
  m_pItems = (wxChar **) NULL;
  m_autoSort = autoSort;
}

// copy ctor
wxArrayString::wxArrayString(const wxArrayString& src)
{
  m_nSize  =
  m_nCount = 0;
  m_pItems = (wxChar **) NULL;
  m_autoSort = src.m_autoSort;

  *this = src;
}

// assignment operator
wxArrayString& wxArrayString::operator=(const wxArrayString& src)
{
  if ( m_nSize > 0 )
    Clear();

  Copy(src);

  return *this;
}

void wxArrayString::Copy(const wxArrayString& src)
{
  if ( src.m_nCount > ARRAY_DEFAULT_INITIAL_SIZE )
    Alloc(src.m_nCount);

  for ( size_t n = 0; n < src.m_nCount; n++ )
    Add(src[n]);
}

// grow the array
void wxArrayString::Grow()
{
  // only do it if no more place
  if( m_nCount == m_nSize ) {
    if( m_nSize == 0 ) {
      // was empty, alloc some memory
      m_nSize = ARRAY_DEFAULT_INITIAL_SIZE;
      m_pItems = new wxChar *[m_nSize];
    }
    else {
      // otherwise when it's called for the first time, nIncrement would be 0
      // and the array would never be expanded
#if defined(__VISAGECPP__) && defined(__WXDEBUG__)
      int array_size = ARRAY_DEFAULT_INITIAL_SIZE;
      wxASSERT( array_size != 0 );
#else
      wxASSERT( ARRAY_DEFAULT_INITIAL_SIZE != 0 );
#endif

      // add 50% but not too much
      size_t nIncrement = m_nSize < ARRAY_DEFAULT_INITIAL_SIZE
                          ? ARRAY_DEFAULT_INITIAL_SIZE : m_nSize >> 1;
      if ( nIncrement > ARRAY_MAXSIZE_INCREMENT )
        nIncrement = ARRAY_MAXSIZE_INCREMENT;
      m_nSize += nIncrement;
      wxChar **pNew = new wxChar *[m_nSize];

      // copy data to new location
      memcpy(pNew, m_pItems, m_nCount*sizeof(wxChar *));

      // delete old memory (but do not release the strings!)
      wxDELETEA(m_pItems);

      m_pItems = pNew;
    }
  }
}

void wxArrayString::Free()
{
  for ( size_t n = 0; n < m_nCount; n++ ) {
    STRING(m_pItems[n])->GetStringData()->Unlock();
  }
}

// deletes all the strings from the list
void wxArrayString::Empty()
{
  Free();

  m_nCount = 0;
}

// as Empty, but also frees memory
void wxArrayString::Clear()
{
  Free();

  m_nSize  =
  m_nCount = 0;

  wxDELETEA(m_pItems);
}

// dtor
wxArrayString::~wxArrayString()
{
  Free();

  wxDELETEA(m_pItems);
}

// pre-allocates memory (frees the previous data!)
void wxArrayString::Alloc(size_t nSize)
{
  wxASSERT( nSize > 0 );

  // only if old buffer was not big enough
  if ( nSize > m_nSize ) {
    Free();
    wxDELETEA(m_pItems);
    m_pItems = new wxChar *[nSize];
    m_nSize  = nSize;
  }

  m_nCount = 0;
}

// minimizes the memory usage by freeing unused memory
void wxArrayString::Shrink()
{
  // only do it if we have some memory to free
  if( m_nCount < m_nSize ) {
    // allocates exactly as much memory as we need
    wxChar **pNew = new wxChar *[m_nCount];

    // copy data to new location
    memcpy(pNew, m_pItems, m_nCount*sizeof(wxChar *));
    delete [] m_pItems;
    m_pItems = pNew;
  }
}

// searches the array for an item (forward or backwards)
int wxArrayString::Index(const wxChar *sz, bool bCase, bool bFromEnd) const
{
  if ( m_autoSort ) {
    // use binary search in the sorted array
    wxASSERT_MSG( bCase && !bFromEnd,
                  wxT("search parameters ignored for auto sorted array") );

    size_t i,
           lo = 0,
           hi = m_nCount;
    int res;
    while ( lo < hi ) {
      i = (lo + hi)/2;

      res = wxStrcmp(sz, m_pItems[i]);
      if ( res < 0 )
        hi = i;
      else if ( res > 0 )
        lo = i + 1;
      else
        return i;
    }

    return wxNOT_FOUND;
  }
  else {
    // use linear search in unsorted array
    if ( bFromEnd ) {
      if ( m_nCount > 0 ) {
        size_t ui = m_nCount;
        do {
          if ( STRING(m_pItems[--ui])->IsSameAs(sz, bCase) )
            return ui;
        }
        while ( ui != 0 );
      }
    }
    else {
      for( size_t ui = 0; ui < m_nCount; ui++ ) {
        if( STRING(m_pItems[ui])->IsSameAs(sz, bCase) )
          return ui;
      }
    }
  }

  return wxNOT_FOUND;
}

// add item at the end
size_t wxArrayString::Add(const wxString& str)
{
  if ( m_autoSort ) {
    // insert the string at the correct position to keep the array sorted
    size_t i,
           lo = 0,
           hi = m_nCount;
    int res;
    while ( lo < hi ) {
      i = (lo + hi)/2;

      res = wxStrcmp(str, m_pItems[i]);
      if ( res < 0 )
        hi = i;
      else if ( res > 0 )
        lo = i + 1;
      else {
        lo = hi = i;
        break;
      }
    }

    wxASSERT_MSG( lo == hi, wxT("binary search broken") );

    Insert(str, lo);

    return (size_t)lo;
  }
  else {
    wxASSERT( str.GetStringData()->IsValid() );

    Grow();

    // the string data must not be deleted!
    str.GetStringData()->Lock();

    // just append
    m_pItems[m_nCount] = (wxChar *)str.c_str(); // const_cast

    return m_nCount++;
  }
}

// add item at the given position
void wxArrayString::Insert(const wxString& str, size_t nIndex)
{
  wxASSERT( str.GetStringData()->IsValid() );

  wxCHECK_RET( nIndex <= m_nCount, wxT("bad index in wxArrayString::Insert") );

  Grow();

  memmove(&m_pItems[nIndex + 1], &m_pItems[nIndex],
          (m_nCount - nIndex)*sizeof(wxChar *));

  str.GetStringData()->Lock();
  m_pItems[nIndex] = (wxChar *)str.c_str();

  m_nCount++;
}

// removes item from array (by index)
void wxArrayString::Remove(size_t nIndex)
{
  wxCHECK_RET( nIndex <= m_nCount, wxT("bad index in wxArrayString::Remove") );

  // release our lock
  Item(nIndex).GetStringData()->Unlock();

  memmove(&m_pItems[nIndex], &m_pItems[nIndex + 1],
          (m_nCount - nIndex - 1)*sizeof(wxChar *));
  m_nCount--;
}

// removes item from array (by value)
void wxArrayString::Remove(const wxChar *sz)
{
  int iIndex = Index(sz);

  wxCHECK_RET( iIndex != wxNOT_FOUND,
               wxT("removing inexistent element in wxArrayString::Remove") );

  Remove(iIndex);
}

// ----------------------------------------------------------------------------
// sorting
// ----------------------------------------------------------------------------

// we can only sort one array at a time with the quick-sort based
// implementation
#if wxUSE_THREADS
  // need a critical section to protect access to gs_compareFunction and
  // gs_sortAscending variables
  static wxCriticalSection *gs_critsectStringSort = NULL;

  // call this before the value of the global sort vars is changed/after
  // you're finished with them
  #define START_SORT()     wxASSERT( !gs_critsectStringSort );                \
                           gs_critsectStringSort = new wxCriticalSection;     \
                           gs_critsectStringSort->Enter()
  #define END_SORT()       gs_critsectStringSort->Leave();                    \
                           delete gs_critsectStringSort;                      \
                           gs_critsectStringSort = NULL
#else // !threads
  #define START_SORT()
  #define END_SORT()
#endif // wxUSE_THREADS

// function to use for string comparaison
static wxArrayString::CompareFunction gs_compareFunction = NULL;

// if we don't use the compare function, this flag tells us if we sort the
// array in ascending or descending order
static bool gs_sortAscending = TRUE;

// function which is called by quick sort
static int LINKAGEMODE wxStringCompareFunction(const void *first, const void *second)
{
  wxString *strFirst = (wxString *)first;
  wxString *strSecond = (wxString *)second;

  if ( gs_compareFunction ) {
    return gs_compareFunction(*strFirst, *strSecond);
  }
  else {
    // maybe we should use wxStrcoll
    int result = wxStrcmp(strFirst->c_str(), strSecond->c_str());

    return gs_sortAscending ? result : -result;
  }
}

// sort array elements using passed comparaison function
void wxArrayString::Sort(CompareFunction compareFunction)
{
  START_SORT();

  wxASSERT( !gs_compareFunction );  // must have been reset to NULL
  gs_compareFunction = compareFunction;

  DoSort();

  // reset it to NULL so that Sort(bool) will work the next time
  gs_compareFunction = NULL;

  END_SORT();
}

void wxArrayString::Sort(bool reverseOrder)
{
  START_SORT();

  wxASSERT( !gs_compareFunction );  // must have been reset to NULL
  gs_sortAscending = !reverseOrder;

  DoSort();

  END_SORT();
}

void wxArrayString::DoSort()
{
  wxCHECK_RET( !m_autoSort, wxT("can't use this method with sorted arrays") );

  // just sort the pointers using qsort() - of course it only works because
  // wxString() *is* a pointer to its data
  qsort(m_pItems, m_nCount, sizeof(wxChar *), wxStringCompareFunction);
}

bool wxArrayString::operator==(const wxArrayString& a) const
{
    if ( m_nCount != a.m_nCount )
        return FALSE;

    for ( size_t n = 0; n < m_nCount; n++ )
    {
        if ( Item(n) != a[n] )
            return FALSE;
    }

    return TRUE;
}

