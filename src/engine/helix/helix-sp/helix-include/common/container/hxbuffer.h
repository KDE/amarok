
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

#ifndef _HXBUFFER_H_
#define _HXBUFFER_H_

#include "ihxpckts.h"
#include "hxvalue.h"
//#include "hxheap.h"
#include "hxstring.h"

typedef struct 
{
    UCHAR*		m_pData;
    ULONG32		m_ulLength;
    unsigned char	m_FreeWithMallocInterfaceIfAvail;
} _BigData;

// This determines the length of the built in buffer that is used if the
// data length is small enough, to save us from allocating so many little
// pieces of data.
#define PnBufferShort
#ifdef PnBufferShort
// XXXNH: This value was originally 15 and was chosen after some research as
// an optimal size for bulk of the small strings we deal with in our buffers.
// However, since the size of the structure is larger than 15 bytes on
// 64-bit systems we are now using this compile-time size calculation to
// ensure that the structure is of sufficient size.
const int MaxPnbufShortDataLen = HX_MAX(sizeof(_BigData), 15);
#endif

#define NUM_ALLOCATION_EACH_TIME	25
/****************************************************************************
 * 
 *	Class:
 *
 *		CHXBuffer
 *
 *	Purpose:
 *
 *		PN implementation of a basic buffer.
 *
 */
class CHXBuffer 
    : public IHXBuffer
{
protected:

        LONG32					m_lRefCount;
	ULONG32				        m_ulAllocLength;
 	HXBOOL m_bJustPointToExistingData;

#if !defined(HELIX_CONFIG_NOSTATICS)
        // Interface for optional allocator
        static 	IMalloc*		m_zMallocInterface;
#endif

        // number of CHXBuffer allocated at a time to be placed in freeStore
        static 	CHXBuffer*		s_pFreeStore;
        static 	const int		s_iBufferChunk;	

        virtual ~CHXBuffer();

        HXBOOL FreeWithMallocInterface() const;

#ifdef PnBufferShort
    // buffer for small amounts of data
    //UCHAR m_ShortData[MaxPnbufShortDataLen + 1];
#endif

        enum { BigDataTag = 0xEE };
        
        union
        {
	    _BigData m_BigData;

            UCHAR m_ShortData[MaxPnbufShortDataLen + 1];
        };

        bool IsShort() const;
        HX_RESULT SetSize(ULONG32 ulLength, HXBOOL copyExistingData);
    
        UCHAR* Allocate(UINT32 size) const;
        UCHAR* Reallocate(UCHAR*, UINT32 oldSize, UINT32 newSize) const;
        void Deallocate(UCHAR*) const;


public:
        CHXBuffer();
        CHXBuffer(UCHAR* pData, UINT32 ulLength, HXBOOL bOwnBuffer = TRUE);

#if 0
#ifndef __MWERKS__
#if defined (_DEBUG) && defined (_WIN32) && 0
         void * operator new(
        unsigned int,
        int,
        const char *,
        int
        );
#else
        void * 		operator	new (size_t size);
#endif /*defined (_DEBUG) && defined (_WIN32) */
        void		operator	delete(void *p, size_t size);
#endif /*__MWERKS__*/
#endif /*0*/

        inline CHXBuffer& operator=(const char* psz);
        inline CHXBuffer& operator=(const unsigned char* psz);
        inline CHXBuffer& operator=(const CHXString &str);
        /*
         *	IUnknown methods
         */
    STDMETHOD(QueryInterface)	(THIS_
                                                                REFIID riid,
                                                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

        /*
         *	IHXBuffer methods
         */
    STDMETHOD(Get)				(THIS_
                                                                REF(UCHAR*)		pData, 
                                                                REF(ULONG32)	ulLength);

    STDMETHOD(Set)				(THIS_
                                                                const UCHAR*	pData, 
                                                                ULONG32			ulLength);

    STDMETHOD(SetSize)			(THIS_
                                                                ULONG32			ulLength);

    STDMETHOD_(ULONG32,GetSize)	(THIS);

    STDMETHOD_(UCHAR*,GetBuffer)
                                                                (THIS);


public:
    static HX_RESULT FromCharArray
    (
        const char* szIn, 
        IHXBuffer** ppbufOut
    );
    static HX_RESULT FromCharArray
    (
        const char* szIn, 
        UINT32 ulLength, 
        IHXBuffer** ppbufOut
    );
    static void SetAllocator(IMalloc* pMalloc);
    static void ReleaseAllocator();
};

CHXBuffer& CHXBuffer::operator=(const char* psz)
{
        Set((const unsigned char*)psz, strlen(psz)+1);
        return(*this);
}

CHXBuffer& CHXBuffer::operator=(const unsigned char* psz)
{
        Set(psz, strlen((const char*)psz)+1);
        return(*this);
}

CHXBuffer& CHXBuffer::operator=(const CHXString& str)
{
        Set((const unsigned char*)(const char *)str, str.GetLength()+1);
        return(*this);
}


// This class was created in order to be able to have a buffer that consists of
// a subset of another existing buffer without allocating any new data or 
// copying data over. The way to use this class is to instantiate it with 3 
// parameters: 
// 1) A pointer to the superset buffer, 
// 2) The pointer to the point in the the buffer that represents the start of 
//    the subset buffer, and 
// 3) The length of the subset buffer.
//
class CHXBufferFragment : public CHXBuffer
{
public :
    CHXBufferFragment(IHXBuffer * pWrappedBuffer, UCHAR* pModFrameStart, ULONG32 ulFragLen) : CHXBuffer( pModFrameStart, ulFragLen, FALSE ), m_pHXBufferPointedTo(pWrappedBuffer){ if(pWrappedBuffer) {pWrappedBuffer->AddRef();} };
    ~CHXBufferFragment(){ HX_RELEASE(m_pHXBufferPointedTo);}

protected :
    IHXBuffer * m_pHXBufferPointedTo;
};

#endif
