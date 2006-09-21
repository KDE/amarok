
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

#ifndef _HXCOM_H_
#define _HXCOM_H_

#include "hxtypes.h"		/* Needed for most type definitions */

// have to use the double expansion to get the prescan level

#define STATCONCAT1(w,x,y,z) STATCONCAT2(w,x,y,z)
#define STATCONCAT2(w,x,y,z) w##x##y##z  

#ifdef _STATICALLY_LINKED
#ifndef _PLUGINNAME
#define ENTRYPOINT(func) STATCONCAT1(entrypoint_error_symbol_should_not_be_needed,_PLUGINNAME,_,func)
#else /* _PLUGINNAME */
#define ENTRYPOINT(func) STATCONCAT1(entrypoint_for_,_PLUGINNAME,_,func)
#endif
#else /* _STATICALLY_LINKED */
#define ENTRYPOINT(func) func
#endif

/*
 * We include objbase.h when building for windows so that hxcom.h can 
 * easily be used in any windows code.
 */
#ifdef _WIN32
#include "hlxclib/sys/socket.h"
#include <objbase.h>
#endif	/* _WIN32 */


#include "hxresult.h"

/*
 *  REF:
 *	Use this for reference parameters, so that C users can 
 *	use the interface as well.
 */
#if defined(__cplusplus)
#define REF(type)   type&
#else
#define REF(type)	const type * const
#endif

/*
 *  CONSTMETHOD:
 *	Use this for constant methods in an interface 
 *	Compiles away under C
 */
#if !defined( CONSTMETHOD )

#if defined(__cplusplus)
#define CONSTMETHOD	const
#else
#define CONSTMETHOD
#endif

#endif
/*
 *  CALL:
 *
 *	Used by C users to easily call a function through an interface
 *
 *  EXAMPLE:
 *
 *	pIFooObject->CALL(IFoo,DoSomething)(bar);
 *
 */
#if !defined(__cplusplus) || defined(CINTERFACE)
#define CALL(iface, func) iface##Vtbl->func
#endif


#define _INTERFACE struct

/*
 * If using windows.h or the windows implementation of COM
 * these defines are not needed.
 */
#if !defined( _OBJBASE_H_ ) 

#ifdef _WIN16
typedef unsigned int	MMRESULT;
#define FAR             _far
#else
#define FAR             
#endif /* WIN16 */
#define PASCAL          _pascal
#define CDECL           _cdecl

/*
 * EXTERN_C
 */
#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C    extern "C"
#else
#define EXTERN_C    extern
#endif
#endif

#ifdef OLDERRORCODES
#ifndef MAKE_HRESULT
#define MAKE_HRESULT(sev,fac,code) ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))))
#endif /*MAKE_HRESULT*/
#endif /* OLDERRORCODES */

/*
 *  STDMETHODCALLTYPE
 */
#ifndef STDMETHODCALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#ifdef _MPPC_
#define STDMETHODCALLTYPE       __cdecl
#else
#define STDMETHODCALLTYPE       __stdcall
#endif
#elif defined(_WIN16)
#define STDMETHODCALLTYPE       __export FAR CDECL
#else
#define STDMETHODCALLTYPE
#endif
#endif

/*
 *  STDMETHODVCALLTYPE
 */
#ifndef STDMETHODVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDMETHODVCALLTYPE      __cdecl
#else
#define STDMETHODVCALLTYPE
#endif
#endif

/*
 *  STDAPICALLTYPE
 */
#ifndef STDAPICALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#define STDAPICALLTYPE          __stdcall
#elif defined(_WIN16)
#define STDAPICALLTYPE          __export FAR PASCAL
#else
#define STDAPICALLTYPE
#endif
#endif

/*
 *  STDAPIVCALLTYPE
 */
#ifndef STDAPIVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDAPIVCALLTYPE         __cdecl
#else
#define STDAPIVCALLTYPE
#endif
#endif

/*
 *  Standard API defines:
 *
 *	NOTE: the 'V' versions allow Variable Argument lists.
 *
 *	STDAPI
 *	STDAPI_(type)
 *	STDAPIV
 *	STDAPIV_(type)
 */
#ifndef STDAPI
#define STDAPI                  EXTERN_C HX_RESULT STDAPICALLTYPE
#endif
#ifndef STDAPI_
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE
#endif
#ifndef STDAPIV
#define STDAPIV                 EXTERN_C HX_RESULT STDAPIVCALLTYPE
#endif
#ifndef STDAPIV_
#define STDAPIV_(type)          EXTERN_C type STDAPIVCALLTYPE
#endif


/*
 *  Standard Interface Method defines:
 *
 *	NOTE: the 'V' versions allow Variable Argument lists.
 *
 *	STDMETHODIMP
 *	STDMETHODIMP_(type)
 *	STDMETHODIMPV
 *	STDMETHODIMPV_(type)
 */
#ifndef STDMETHODIMP
#define STDMETHODIMP            HX_RESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMPV
#define STDMETHODIMPV           HX_RESULT STDMETHODVCALLTYPE
#endif
#ifndef STDMETHODIMPV_
#define STDMETHODIMPV_(type)    type STDMETHODVCALLTYPE
#endif


#if defined(__cplusplus) && !defined(CINTERFACE)
#define _INTERFACE struct
#define STDMETHOD(method)       virtual HX_RESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define PURE                    = 0
#define THIS_
#define THIS                    void

#if defined(_WINDOWS) && defined(EXPORT_CLASSES)
#define DECLARE_INTERFACE(iface)    _INTERFACE HXEXPORT_CLASS iface
#define DECLARE_INTERFACE_(iface, baseiface)    _INTERFACE HXEXPORT_CLASS iface : public baseiface
#else
#define DECLARE_INTERFACE(iface)    _INTERFACE iface
#define DECLARE_INTERFACE_(iface, baseiface)    _INTERFACE iface : public baseiface
#endif // defined(_WINDOWS) && defined(EXPORT_CLASSES)

#if !defined(BEGIN_INTERFACE)
#if defined(_MPPC_)  && \
    ( (defined(_MSC_VER) || defined(__SC__) || defined(__MWERKS__)) && \
    !defined(NO_NULL_VTABLE_ENTRY) )
   #define BEGIN_INTERFACE virtual void a() {}
   #define END_INTERFACE
#else
   #define BEGIN_INTERFACE
   #define END_INTERFACE
#endif
#endif


#else

#define _INTERFACE               struct

#define STDMETHOD(method)       HX_RESULT (STDMETHODCALLTYPE * method)
#define STDMETHOD_(type,method) type (STDMETHODCALLTYPE * method)

#if !defined(BEGIN_INTERFACE)
#if defined(_MPPC_)
    #define BEGIN_INTERFACE       void    *b;
    #define END_INTERFACE
#else
    #define BEGIN_INTERFACE
    #define END_INTERFACE
#endif
#endif


#define PURE
#define THIS_                   INTERFACE FAR* This,
#define THIS                    INTERFACE FAR* This
#ifdef CONST_VTABLE
#undef CONST_VTBL
#define CONST_VTBL const
#define DECLARE_INTERFACE(iface)    typedef _INTERFACE iface { \
                                    const struct iface##Vtbl FAR* lpVtbl; \
                                } iface; \
                                typedef const struct iface##Vtbl iface##Vtbl; \
                                const struct iface##Vtbl
#else
#undef CONST_VTBL
#define CONST_VTBL
#define DECLARE_INTERFACE(iface)    typedef _INTERFACE iface { \
                                    struct iface##Vtbl FAR* lpVtbl; \
                                } iface; \
                                typedef struct iface##Vtbl iface##Vtbl; \
                                struct iface##Vtbl
#endif
#define DECLARE_INTERFACE_(iface, baseiface)    DECLARE_INTERFACE(iface)

#endif

/*
 *  COMMON TYPES
 */

#if !defined(HELIX_FEATURE_FULLGUID)

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#define DEFINE_GUID_ENUM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    name = l + w1 * 3 + w2 * 5 + b1 * 7 + b2 * 11 + b3 * 13 + b4 * 17 +   \
           b5 * 19 + b6 * 23 + b7 * 29 + b8 * 31,

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef enum _GUIDNoConflict
{
    GUID_NULL,
    DEFINE_GUID_ENUM(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46)
    DEFINE_GUID_ENUM(IID_IMalloc,  0x00000002, 0x0000, 0x0000, 0xC0,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46)
#include "hxiids.h"
#include "hxpiids.h"
    NUM_GUIDS
} GUID;
#endif

#else /* #if !defined(HELIX_FEATURE_FULLGUID) */
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct  _GUID
    {
    ULONG32		Data1;
    UINT16		Data2;
    UINT16		Data3;
    UCHAR		Data4[ 8 ];
    }	GUID;
#endif
#endif /* #if !defined(HELIX_FEATURE_FULLGUID) #else */

#if !defined( __IID_DEFINED__ )
#define __IID_DEFINED__
typedef GUID IID;
#define IID_NULL            GUID_NULL
typedef GUID CLSID;
#define CLSID_NULL          GUID_NULL

#if defined(__cplusplus)
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID &
#endif

#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID &
#endif

#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID &
#endif

#else
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID * const
#endif
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID * const
#endif
#endif
#endif


#if defined(HELIX_FEATURE_FULLGUID)

#if !defined (INITGUID) || (defined (_STATICALLY_LINKED) && !defined(NCIHACK))

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name

#define DEFINE_GUID_ENUM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name;

#else /* #if !defined (INITGUID) || (defined (_STATICALLY_LINKED) && !defined(NCIHACK)) */

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DEFINE_GUID_ENUM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } };

#endif /* #if !defined (INITGUID) || (defined (_STATICALLY_LINKED) && !defined(NCIHACK)) #else */

#endif /* #if defined(HELIX_FEATURE_FULLGUID) */

#include "hlxclib/memory.h"		/* for memcmp */

#ifdef __cplusplus
inline HXBOOL IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
#if defined(HELIX_FEATURE_FULLGUID)
   return !memcmp(&rguid1, &rguid2, sizeof(GUID));
#else	// HELIX_FEATURE_FULLGUID
   return (rguid1 == rguid2);
#endif	// HELIX_FEATURE_FULLGUID
    
}

inline void SetGUID(GUID& rguid1, REFGUID rguid2)
{
#if defined(HELIX_FEATURE_FULLGUID)
    memcpy(&rguid1, &rguid2, sizeof(GUID)); /* Flawfinder: ignore */
#else	// HELIX_FEATURE_FULLGUID
    rguid1 = rguid2;
#endif	// HELIX_FEATURE_FULLGUID
}
#else	// __cplusplus
#if defined(HELIX_FEATURE_FULLGUID)
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define SetGUID(rguid1, rguid2) (memcpy(rguid1, rguid2, sizeof(GUID))) /* Flawfinder: ignore */
#else	// HELIX_FEATURE_FULLGUID
#define IsEqualGUID(rguid1, rguid2) ((rguid1) == (rguid2))
#define SetGUID(rguid1, rguid2) ((rguid1) = (rguid2))
#endif	// HELIX_FEATURE_FULLGUID
#endif	// __cplusplus

#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#define SetIID(riid1, riid2)	    SetGUID(riid1, riid2)
#define SetCLSID(rclsid1, rclsid2)  SetGUID(rclsid1, rclsid2)

#ifdef __cplusplus

/*
 * Because GUID is defined elsewhere in WIN32 land, the operator == and !=
 * are moved outside the class to global scope.
 */
#if defined(HELIX_FEATURE_FULLGUID)
inline HXBOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
}

inline HXBOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif	// HELIX_FEATURE_FULLGUID

#endif


/****************************************************************************
 * 
 *  Interface:
 *
 *	IUnknown
 *
 *  Purpose:
 *
 *	Base class of all interfaces. Defines life time management and
 *	support for dynamic cast.
 *
 *  IID_IUnknown:
 *
 *	{00000000-0000-0000-C000000000000046}
 *
 */
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

#undef  INTERFACE
#define INTERFACE   IUnknown

DECLARE_INTERFACE(IUnknown)
{
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IMalloc
 *
 *  Purpose:
 *
 *	Basic memory management interface.
 *
 *  IID_IMalloc:
 *
 *	{00000002-0000-0000-C000000000000046}
 *
 */
DEFINE_GUID(IID_IMalloc, 00000002, 0x0000, 0x0000, 0xC0, 0x00, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

#undef  INTERFACE
#define INTERFACE   IMalloc

DECLARE_INTERFACE_(IMalloc, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IMalloc methods
     */
    STDMETHOD_(void*,Alloc)	    (THIS_
				    UINT32  /*IN*/ count) PURE;

    STDMETHOD_(void*,Realloc)	    (THIS_
				    void*   /*IN*/ pMem,
				    UINT32  /*IN*/ count) PURE;

    STDMETHOD_(void,Free)	    (THIS_
				    void*   /*IN*/ pMem) PURE;

    STDMETHOD_(UINT32,GetSize)	    (THIS_
				    void*   /*IN*/ pMem) PURE;

    STDMETHOD_(HXBOOL,DidAlloc)	    (THIS_
				    void*   /*IN*/ pMem) PURE;

    STDMETHOD_(void,HeapMinimize)   (THIS) PURE;
};

#else /* else case of !defined( _OBJBASE_H_ ) && !defined( _COMPOBJ_H_ ) */

// For now, we always use full guids on Windows
#ifndef HELIX_FEATURE_FULLGUID
#define HELIX_FEATURE_FULLGUID
#endif /* HELIX_FEATURE_FULLGUID */

#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif

#if !defined (INITGUID) || (defined (_STATICALLY_LINKED) && !defined(NCIHACK))
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#define DEFINE_GUID_ENUM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name;
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#define DEFINE_GUID_ENUM(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } };
#endif


/* Even in windows we want these GUID's defined... */

#if !(defined(INITGUID) && defined(USE_IUNKNOWN_AND_IMALLOC_FROM_UUID_LIB))
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

DEFINE_GUID(IID_IMalloc, 00000002, 0x0000, 0x0000, 0xC0, 0x00, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
#endif

#include <memory.h>		/* for memcmp */

#ifdef __cplusplus
inline void SetGUID(REFGUID rguid1, REFGUID rguid2)
{
    memcpy((void*)&rguid1, (void*)&rguid2, sizeof(GUID)); /* Flawfinder: ignore */
}
#else
#define SetGUID(rguid1, rguid2) (memcpy((void*)rguid1, (void*)rguid2, sizeof(GUID))) /* Flawfinder: ignore */
#endif
#define SetIID(riid1, riid2)	    SetGUID(riid1, riid2)
#define SetCLSID(rclsid1, rclsid2)  SetGUID(rclsid1, rclsid2)

#endif /* !defined( _OBJBASE_H_ ) && !defined( _COMPOBJ_H_ )*/

#ifdef __cplusplus

/*
 * This operator is defined for all platforms
 */
#if defined(HELIX_FEATURE_FULLGUID)
inline HXBOOL operator<(const GUID& lhs, const GUID& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
}
#endif	// HELIX_FEATURE_FULLGUID

#endif	// __cplusplus


#ifdef IsEqualIID
#undef IsEqualIID
#endif

#ifdef IsEqualCLSID
#undef IsEqualCLSID
#endif

#define IsEqualIID(riid1, riid2) HXIsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) HXIsEqualGUID(rclsid1, rclsid2)

#ifdef __cplusplus
inline HXBOOL HXIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
#if defined(HELIX_FEATURE_FULLGUID)
   return (((UINT32*) &rguid1)[0] == ((UINT32*) &rguid2)[0]  &&
	    ((UINT32*) &rguid1)[1] == ((UINT32*) &rguid2)[1] &&
	    ((UINT32*) &rguid1)[2] == ((UINT32*) &rguid2)[2] &&
	    ((UINT32*) &rguid1)[3] == ((UINT32*) &rguid2)[3]);
#else
   return( (rguid1) == (rguid2) );
#endif
}
#else
#if defined(HELIX_FEATURE_FULLGUID)
#define HXIsEqualGUID(rguid1, rguid2)		\
	(((UINT32*) &rguid1)[0] == ((UINT32*) &rguid2)[0]  &&   \
	    ((UINT32*) &rguid1)[1] == ((UINT32*) &rguid2)[1] && \
	    ((UINT32*) &rguid1)[2] == ((UINT32*) &rguid2)[2] && \
	    ((UINT32*) &rguid1)[3] == ((UINT32*) &rguid2)[3]);
#else
#define HXIsEqualGUID(rguid1, rguid2)		\
  	( (rguid1) == (rguid2) );
#endif
#endif

/****************************************************************************
 *
 *  QueryInterface size reduction structs and functions.
 *
 *  Example Usage:
 *
 *  QInterfaceList qiList[] =
 *   {
 *	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) (IHXPlugin*) this},
 *	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this},
 *	{ GET_IIDHANDLE(IID_IHXFileFormatObject), (IHXFileFormatObject*) this},
 *	{ GET_IIDHANDLE(IID_IHXAtomizerResponse), (IHXAtomizerResponse*) this},
 *	{ GET_IIDHANDLE(IID_IHXAtomizationCommander), (IHXAtomizationCommander*) this},
 *	{ GET_IIDHANDLE(IID_IHXASMSource), (IHXASMSource*) this},
 *	{ GET_IIDHANDLE(IID_IHXPacketFormat), (IHXPacketFormat*) this},
 *	{ GET_IIDHANDLE(IID_IHXFileSwitcher), (IHXFileSwitcher*) m_pFileSwitcher},
 *	{ GET_IIDHANDLE(IID_IHXCommonClassFactory), m_pClassFactory},
 *	{ GET_IIDHANDLE(IID_IHXScheduler), m_pScheduler}
 *   };
 *
 *   return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
 */
#if !defined(HELIX_FEATURE_FULLGUID)

#define IIDHANDLE	    IID
#define GET_IIDHANDLE(x)    (x)
#define DREF_IIDHANDLE(x)   (x)

#else	// HELIX_FEATURE_FULLGUID

#define IIDHANDLE	    const IID*
#define GET_IIDHANDLE(x)    (&(x))
#define DREF_IIDHANDLE(x)   (*(x))

#endif	// HELIX_FEATURE_FULLGUID

typedef struct
{
    IIDHANDLE hiid;
    void* pIFace;
} QInterfaceList;

#define QILISTSIZE(x)	sizeof(x)/sizeof(QInterfaceList)

#if !defined(INITGUID) || (defined(_STATICALLY_LINKED) && !defined(NCIHACK))
EXTERN_C HX_RESULT QIFind(QInterfaceList* qiList, UINT32 ulqiListSize, 
			  REFIID riid, void** ppvObj);
#else	// !INITGUID || (_STATICALLY_LINKED && NCIHACK)
EXTERN_C HX_RESULT QIFind(QInterfaceList* qiList, UINT32 ulqiListSize, 
			  REFIID riid, void** ppvObj)
{
    do
    {
	if (IsEqualIID(DREF_IIDHANDLE(qiList->hiid), riid))
	{
	    *ppvObj = (qiList->pIFace);
	    if (*ppvObj)
	    {
		((IUnknown*) (*ppvObj))->AddRef();
		return HXR_OK;
	    }
	    return HXR_NOINTERFACE;
	}
	qiList++;
    } while ((--ulqiListSize) != 0);

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}
#endif	// !INITGUID || (_STATICALLY_LINKED && NCIHACK)


/****************************************************************************
 *
 *  Putting the following macro in the definition of your class will overload
 *  new and delete for that object.  New will then take an IMalloc* from
 *  which to allocate memory from and store it in the beginning of the
 *  memory which it will return.  Delete will grab this IMalloc* from
 *  the beginning of the mem and use this pointer to deallocate the mem.
 *
 *  Example useage:
 *  class A
 *  {
 *  public:
 *      A(int);
 *      ~A();
 *
 *      IMALLOC_MEM
 *  };
 *
 *  IMalloc* pMalloc;
 *  m_pContext->QueryInterface(IID_IMalloc, (void**)&pMalloc);
 *  A* p = new(pMalloc) A(0);
 *  pMalloc->Release();
 *  delete p;
 */

#define IMALLOC_MEM\
    void* operator new(size_t size, IMalloc* pMalloc)\
    {\
        void* pMem = pMalloc->Alloc(size + sizeof(IMalloc*));\
        *(IMalloc**)pMem = pMalloc;\
        pMalloc->AddRef();\
        return ((unsigned char*)pMem + sizeof(IMalloc*));\
    }\
\
    void operator delete(void* pMem)\
    {\
        pMem = (unsigned char*)pMem - sizeof(IMalloc*);\
        IMalloc* pMalloc = *(IMalloc**)pMem;\
        pMalloc->Free(pMem);\
        pMalloc->Release();\
    }\


/****************************************************************************
 *
 * By default, we attempt to use atomic InterlockedIncrement/Decrement
 * implementations.  Add HELIX_FEATURE_DISABLE_INTERLOCKED_INC_DEC to 
 * your profile's .pf or to your Umakefil/.pcf file to use non-threadsafe
 * implementations.
 */
#include "atomicbase.h" 

#if !defined HAVE_INTERLOCKED_INCREMENT
#undef InterlockedIncrement
#undef InterlockedDecrement
#define InterlockedIncrement(plong) (++(*(plong)))
#define InterlockedDecrement(plong) (--(*(plong)))
#endif /* !defined HAVE_INTERLOCKED_INCREMENT */


#if defined(HELIX_CONFIG_COMPACT_COM_MACROS)

EXTERN_C void HX_AddRefFunc(IUnknown** pUnk);
EXTERN_C void HX_ReleaseFunc(IUnknown** pUnk);

#if defined(INITGUID) && (!defined(_STATICALLY_LINKED) || defined(NCIHACK))

void HX_AddRefFunc(IUnknown** pUnk)
{
    if (*pUnk)
    {
        (*pUnk)->AddRef();
    }
}

void HX_ReleaseFunc(IUnknown** pUnk)
{
    if (*pUnk)
    {
        (*pUnk)->Release();
        *pUnk = 0;
    }
}

#endif // INITGUID && (!_STATICALLY_LINKED || NCIHACK)

#if defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS)

#if defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS_IMPLICIT_CAST)

#define HX_RELEASE(x) (HX_ReleaseFunc(&x))
#define HX_ADDREF(x) (HX_AddRefFunc(&x))

#else // defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS_IMPLICIT_CAST)

// we don't want to build with this; this should be compiled only
// to ensure that we have a COM object in the code base

#define HX_RELEASE(x) \
{ \
	if (x) \
	{ \
		IUnknown* pUnk; \
		(x)->QueryInterface(IID_IUnknown, (void**)&pUnk); \
		if (pUnk) \
		{ \
			pUnk->Release(); \
		} \
		(x)->Release(); \
		(x) = 0; \
	} \
} \

#define HX_ADDREF(x) \
{ \
	if (x) \
	{ \
		IUnknown* pUnk; \
		(x)->QueryInterface(IID_IUnknown, (void**)&pUnk); \
		if (pUnk) \
		{ \
			pUnk->Release(); \
		} \
		(x)->AddRef(); \
	} \
} \

#endif // defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS_IMPLICIT_CAST)

#else // defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS)

#define HX_RELEASE(x) (HX_ReleaseFunc((IUnknown**)&(x)))
#define HX_ADDREF(x) (HX_AddRefFunc((IUnknown**)&(x)))

#endif // defined(HELIX_CONFIG_TYPE_CHECK_COM_MACROS)

#else // defined(HELIX_CONFIG_COMPACT_COM_MACROS)

#define HX_RELEASE(x) ((x) ? ((x)->Release(), (x) = 0) : 0)
#define HX_ADDREF(x) ((x) ? ((x)->AddRef()) : 0)

#endif // defined(HELIX_CONFIG_COMPACT_COM_MACROS)



#endif /* _HXCOM_H_ */
