
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 */

#ifndef HXSTRING_H
#define HXSTRING_H

#include "hxtypes.h"
#include "hxassert.h"
#include "hlxclib/limits.h"
#include "hlxclib/string.h"

#if defined(HELIX_CONFIG_NOSTATICS)
# include "globals/hxglobalchxstring.h"
#endif


typedef INT32 (*StringGrowthFunc)(INT32 currentSize, INT32 sizeNeeded);

class CHXStringRep
{
public:
    CHXStringRep(INT32 bufSize = 1, bool bSetLength = false);
    CHXStringRep(const char* pStr);
    CHXStringRep(const char* pStr, INT32 bufSize);
    CHXStringRep(char ch, INT32 bufSize);
    ~CHXStringRep();

    void AddRef();
    void Release();

    char* GetBuffer();
    INT32 GetStringSize() const;
    INT32 GetBufferSize() const;

    void SetStringSize(INT32 strSize);
    void Resize(INT32 newSize);
    void ResizeAndCopy(INT32 newSize, bool bSetLength = false);

    void Copy(const char* pStr, INT32 size);

    bool IsShared() const;

private:
    CHXStringRep(const CHXStringRep&);
    CHXStringRep& operator=(const CHXStringRep&);

    INT32 m_refCount;
    INT32 m_strSize;
    INT32 m_bufSize;
    char* m_pData;
};

class HXEXPORT_CLASS CHXString
{
public:
    CHXString(StringGrowthFunc pGrowthFunc = 0);
    CHXString(const CHXString& rhs);
    CHXString(char ch, int length = 1, StringGrowthFunc pGrowthFunc = 0);
    CHXString(const char* pStr, StringGrowthFunc pGrowthFunc = 0);
    CHXString(const char* pStr, int length, StringGrowthFunc pGrowthFunc = 0);
    CHXString(const unsigned char* pStr, StringGrowthFunc pGrowthFunc = 0);
    ~CHXString();

    // Attributes & Operations
    // as an array of characters
    UINT32 GetLength() const;
    HXBOOL IsEmpty() const;
    void Empty();

    char GetAt(INT32 i) const;
    char operator[](short i) const;
    char operator[](unsigned short i) const;
    char operator[](int i) const;
    char operator[](unsigned int i) const;
    char operator[](long i) const;
    char operator[](unsigned long i) const;

    void SetAt(INT32 i, char ch);
    operator const char*() const;

    bool operator>(const CHXString& rhs) const;
    bool operator>(const char* pStr) const;
    bool operator>(const unsigned char* pStr) const;
    bool operator>=(const CHXString& rhs) const;
    bool operator>=(const char* pStr) const;
    bool operator>=(const unsigned char* pStr) const;
    bool operator==(const CHXString& rhs) const;
    bool operator==(const char* pStr) const;
    bool operator==(const unsigned char* pStr) const;
    bool operator!=(const CHXString& rhs) const;
    bool operator!=(const char* pStr) const;
    bool operator!=(const unsigned char* pStr) const;
    bool operator<=(const CHXString& rhs) const;
    inline bool operator<=(const char* pStr) const;
    inline bool operator<=(const unsigned char* pStr) const;
    bool operator<(const CHXString& rhs) const;
    inline bool operator<(const char* pStr) const;
    inline bool operator<(const unsigned char* pStr) const;

    const CHXString& operator=(const CHXString& rhs);
    const CHXString& operator=(char ch);
    const CHXString& operator=(const char* pStr);
    const CHXString& operator=(const unsigned char* pStr);

    const CHXString& operator+=(const CHXString& rhs);
    const CHXString& operator+=(char ch);
    const CHXString& operator+=(const char* pStr);

    friend CHXString  operator+(const CHXString& strA,
                                 const CHXString& strB);

    friend CHXString  operator+(const CHXString& str, char ch);
    friend CHXString  operator+(char ch , const CHXString& str);
    friend CHXString  operator+(const CHXString& strA, const char* pStrB);
    friend CHXString  operator+(const char* pStrA, const CHXString& strB);

    INT32 Compare(const char* pStr) const;
    INT32 CompareNoCase(const char* pStr) const;

    void Center(short length);
    CHXString Mid(INT32 i, INT32 length) const;
    CHXString Mid(INT32 i) const;
    CHXString Left(INT32 length) const;
    CHXString Right(INT32 length) const;

    ULONG32 CountFields(char delim) const;
    CHXString GetNthField(char delim, ULONG32 i, UINT64& state) const;
    CHXString NthField(char delim, ULONG32 i) const;

    CHXString SpanIncluding(const char* pCharSet) const;
    CHXString SpanExcluding(const char* pCharSet) const;

    void MakeUpper();
    void MakeLower();

    void TrimRight();
    void TrimLeft();

    INT32 Find(char ch) const;
    INT32 ReverseFind(char ch) const;
    HXBOOL FindAndReplace(const char* pSearch , const char* pReplace,
                        HXBOOL bReplaceAll = FALSE);

    INT32 Find(const char* pStr) const;

    void Format(const char* pFmt, ...);

    void AppendULONG(ULONG32 value);

    void AppendEndOfLine();

    char* GetBuffer(INT32 minSize);
    void ReleaseBuffer(INT32 newSize = -1);
    char* GetBufferSetLength(INT32 newSize);
    void FreeExtra();

    INT32 GetAllocLength() const;
    INT32 SetMinBufSize(INT32 minSize);

#if defined(_MACINTOSH) || defined(_MAC_UNIX)
    const CHXString& SetFromStr255(const Str255 );

    const CHXString& AppendFromStr255(const Str255 );
    const CHXString& InsertFromStr255(const Str255 );

    const CHXString& SetFromIndString(short , short );

    const CHXString& operator =(FSSpec );
    operator const FSSpec(void);

    HX_RESULT MakeStr255(Str255& ) const;

#if !defined(_CARBON) && !defined(_MAC_UNIX)
    operator Str255* (void);
    operator const Str255* (void) const;
    operator ConstStr255Param (void) const;
#else
    const CHXString& operator =(const FSRef& );
    // ConstStr255Param operator disallowed since
    // it's not implemented in Carbon
    //operator ConstStr255Param (void) const;
    operator const FSRef(void);

    const CHXString& operator =(CFStringRef );

    HX_RESULT SetFromCFString(CFStringRef , CFStringEncoding );

    HX_RESULT SetFromHFSUniStr255(const HFSUniStr255& , CFStringEncoding );
    HX_RESULT MakeHFSUniStr255(HFSUniStr255& , CFStringEncoding ) const;
#endif /* !defined(_CARBON) */

#endif /* _MACINTOSH */

protected:

    void Init(const char* pStr, UINT32 size = UINT_MAX );
    void Nuke();
    void ConcatInPlace(const char* pStr, const UINT32 size);
    void EnsureUnique(); // Make sure that m_pRep is not shared
    void Release();
    static UINT32 SafeStrlen(const char* );

//#define CHXSMEMCHK
#ifdef CHXSMEMCHK
    INT32 m_memchkCopied;
    INT32 m_memchkChanged;
    INT32 m_memchkDataLength;
    static INT32 g_memchkCopiedNotChanged;
    static INT32 g_memchkCopiedChanged;
    static INT32 g_memchkCounter;
    static INT32 g_memchkHighCount;
    static INT32 g_memchkTotalBytesNotChanged;
    char*        m_memchkData;
    static memchkWhatever Dummy;

    static void memchkLogStats(void);
#endif /* CHXSMEMCHK */

private:
    static INT32 MinimalGrowth(INT32 currentSize, INT32 sizeNeeded);
    static INT32 DoublingGrowth(INT32 currentSize, INT32 sizeNeeded);

    void Append(const char* pStr, INT32 size);
    void Grow(INT32 newSize);

    CHXStringRep* m_pRep;
    StringGrowthFunc m_pGrowthFunc;
};

#if !defined(HELIX_CONFIG_NOSTATICS)
extern const CHXString HXEmptyString;
#else
extern const char* const _g_emptyString;
#define HXEmptyString HXGlobalCHXString::Get(&_g_emptyString)
#endif

inline
char* CHXStringRep::GetBuffer()
{
    return m_pData;
}

inline
INT32 CHXStringRep::GetStringSize() const
{
    return m_strSize;
}

inline
INT32 CHXStringRep::GetBufferSize() const
{
    return m_bufSize;
}

inline
void CHXStringRep::SetStringSize(INT32 strSize)
{
    HX_ASSERT(strSize >= 0);
    HX_ASSERT(strSize < m_bufSize);
    HX_ASSERT((size_t)strSize == strlen(m_pData));
    m_strSize = strSize;
}

inline
bool CHXStringRep::IsShared() const
{
    return (m_refCount > 1);
}

inline
UINT32 CHXString::GetLength() const
{
    return (m_pRep) ? m_pRep->GetStringSize() : 0;
}

inline
HXBOOL CHXString::IsEmpty() const
{
    return (GetLength() == 0);
}

inline
char CHXString::GetAt(INT32 i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i >= 0);
    HX_ASSERT(i < m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](short i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i >= 0);
    HX_ASSERT(i < m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](unsigned short i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT((INT32)i < m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](int i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i >= 0);
    HX_ASSERT(i < m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](unsigned int i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i < (unsigned int)m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](long i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i >= 0);
    HX_ASSERT(i < m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
char CHXString::operator[](unsigned long i) const
{
    HX_ASSERT(m_pRep);
    HX_ASSERT(i < (unsigned long)m_pRep->GetBufferSize());
    return m_pRep->GetBuffer()[i];
}

inline
CHXString::operator const char*() const
{
    return (m_pRep) ? m_pRep->GetBuffer() : (const char*)(&m_pRep);
}

inline
INT32 CHXString::Compare(const char* pStr) const
{
    return strcmp((const char*)(*this), pStr);
}

inline
INT32 CHXString::CompareNoCase(const char* pStr) const
{
    return strcasecmp((const char*)(*this), pStr);
}

inline
bool CHXString::operator>(const char* pStr) const
{
    return (Compare(pStr) > 0);
}

inline
bool CHXString::operator>(const CHXString& rhs) const
{
    return (*this > ((const char*)rhs));
}

inline
bool CHXString::operator>(const unsigned char* pStr) const
{
    return (*this > ((const char*)pStr));
}

inline
bool operator>(const char* pA, const CHXString& b)
{
    return (b < pA);
}

inline
bool operator>(const unsigned char* pA, const CHXString& b)
{
    return (b < pA);
}

inline
bool CHXString::operator>=(const char* pStr) const
{
    return (Compare(pStr) >= 0);
}

inline
bool CHXString::operator>=(const CHXString& rhs) const
{
    return (*this >= ((const char*)rhs));
}

inline
bool CHXString::operator>=(const unsigned char* pStr) const
{
    return (*this >= ((const char*)pStr));
}

inline
bool operator>=(const char* pA, const CHXString& b)
{
    return (b <= pA);
}

inline
bool operator>=(const unsigned char* pA, const CHXString& b)
{
    return (b <= pA);
}

inline
bool CHXString::operator==(const char* pStr) const
{
    return (strcmp(((const char*)*this), pStr) == 0);
}

inline
bool CHXString::operator==(const CHXString& rhs) const
{
    return ((m_pRep == rhs.m_pRep) ||
            ((GetLength() == rhs.GetLength()) &&
             (*this == ((const char*)rhs))));
}

inline
bool CHXString::operator==(const unsigned char* pStr) const
{
    return (*this == ((const char*)pStr));
}

inline
bool operator==(const char* pA, const CHXString& b)
{
    return (b == pA);
}

inline
bool operator==(const unsigned char* pA, const CHXString& b)
{
    return (b == pA);
}

inline
bool CHXString::operator!=(const char* pStr) const
{
    return (strcmp(((const char*)*this), pStr) != 0);
}

inline
bool CHXString::operator!=(const CHXString& rhs) const
{
    return ((m_pRep != rhs.m_pRep) &&
            ((GetLength() != rhs.GetLength()) ||
             (*this != ((const char*)rhs))));
}

inline
bool CHXString::operator!=(const unsigned char* pStr) const
{
    return (*this != ((const char*)pStr));
}

inline
bool operator!=(const char* pA, const CHXString& b)
{
    return (b != pA);
}

inline
bool operator!=(const unsigned char* pA, const CHXString& b)
{
    return (b != pA);
}

inline
bool CHXString::operator<=(const char* pStr) const
{
    return (Compare(pStr) <= 0);
}

inline
bool CHXString::operator<=(const CHXString& rhs) const
{
    return (*this <= ((const char*)rhs));
}

inline
bool CHXString::operator<=(const unsigned char* pStr) const
{
    return (*this <= ((const char*)pStr));
}

inline
bool operator<=(const char* pA, const CHXString& b)
{
    return (b >= pA);
}

inline
bool operator<=(const unsigned char* pA, const CHXString& b)
{
    return (b >= pA);
}

inline
bool CHXString::operator<(const char* pStr) const
{
    return (Compare(pStr) < 0);
}

inline
bool CHXString::operator<(const CHXString& rhs) const
{
    return (*this < ((const char*)rhs));
}

inline
bool CHXString::operator<(const unsigned char* pStr) const
{
    return (*this < ((const char*)pStr));
}

inline
bool operator<(const char* pA, const CHXString& b)
{
    return (b > pA);
}

inline
bool operator<(const unsigned char* pA, const CHXString& b)
{
    return (b > pA);
}

inline
UINT32 CHXString::SafeStrlen(const char* pStr)
{
    return (pStr) ? strlen(pStr) : 0;
}
#endif /* HXSTRING_H */
