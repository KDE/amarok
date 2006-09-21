
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

#ifndef _HXWIN_H_
#define _HXWIN_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXSite			IHXSite;
typedef _INTERFACE	IHXSiteUser			IHXSiteUser;
typedef _INTERFACE	IHXSiteWindowed		IHXSiteWindowed;
typedef _INTERFACE	IHXSiteEventHandler            IHXSiteEventHandler;
typedef _INTERFACE	IHXSiteWindowless		IHXSiteWindowless;
typedef _INTERFACE	IHXSiteWatcher			IHXSiteWatcher;
typedef _INTERFACE	IHXValues			IHXValues;
typedef _INTERFACE	IHXSiteFullScreen		IHXSiteFullScreen;
typedef _INTERFACE	IHXLayoutSiteGroupManager	IHXLayoutSiteGroupManager;
typedef _INTERFACE	IHXEventHook			IHXEventHook;
typedef _INTERFACE	IHXColorConverter		IHXColorConverter;
typedef _INTERFACE	IHXSubRectVideoSurface		IHXSubRectVideoSurface;

typedef struct _HXBitmapInfoHeader HXBitmapInfoHeader;
typedef struct _HXxWindow   HXxWindow;
typedef struct _HXxRegion  HXxBoxRegion;
typedef struct _HXxSize	    HXxSize;
typedef struct _HXxPoint    HXxPoint;
typedef struct _HXxRect	    HXxRect;
typedef void* HXxRegion;


/*
 * Styles for IHXDrawFocus
 */
#define HX_SOLID_LINE      1
#define HX_DASHED_LINE     HX_SOLID_LINE<<1
#define HX_DOTTED_LINE     HX_SOLID_LINE<<2
#define HX_CUSTOM_LINE     HX_SOLID_LINE<<3

/*
 * Focus Navigation
 */
typedef enum _HXFocusContext
{
    HXFirstFocus,
    HXUpFocus,
    HXDownFocus,
    HXLeftFocus,
    HXRightFocus,
    HXNextFocus,
    HXPrevFocus,
    HXLastFocus
} HXFocusContext;

typedef enum _HXFocusState
{
    HXNoFocus,
    HXFocused
} HXFocusState;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteWindowed
 *
 *  Purpose:
 *
 *	Interface for IHXSite objects which are associated with platform
 *	specific window objects on Microsoft Windows and X-Windows.
 *
 *  IID_IHXSiteWindowed:
 *
 *	{00000D01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteWindowed, 0x00000D01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXSiteWindowed	IID_IHXSiteWindowed

// $Private:
DEFINE_GUID(IID_IHXGetImplementation, 0x00000D11, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);
// $EndPrivate.

#undef  INTERFACE
#define INTERFACE   IHXSiteWindowed

DECLARE_INTERFACE_(IHXSiteWindowed, IUnknown)
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
     * IHXSiteWindowed methods called by site suppliers 
     * when they want the site associated with a
     * previously created (and externally managed) window.
     * This method will "sub-class" that window (Win32).
     * On Unix, the site supplier must pass events from 
     * the externally managed window to the core via
     * IHXClientEngine::EventOccurred(). Please note that
     * The HXxWindow ptr must remain in scope for the life
     * of Site.
     * 
     */
    STDMETHOD(AttachWindow)	(THIS_
				HXxWindow* /*IN*/ pWindow) PURE;

    STDMETHOD(DetachWindow)	(THIS) PURE;

    /*
     * IHXSiteWindowed methods called by Owners of the site
     * in the event that want a default top level window created
     * for the site.
     */
    STDMETHOD(Create)		(THIS_
				void* ParentWindow, 
				UINT32 style) PURE;

    STDMETHOD(Destroy)		(THIS) PURE;

    /*
     * IHXSiteWindowed method. Returns actual window of the site.
     */
    STDMETHOD_(HXxWindow*,GetWindow)(THIS) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteEventHandler
 *
 *  Purpose:
 *
 *	Interface for allowing client core engine to pass events to a site imeplementor
 *      which it implemented as a factory plugin. 
 *
 *  IID_IHXSiteEventHandler                      
 *
 *	{00000D12-0901-11d1-8B-6-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXSiteEventHandler, 0x00000D12, 0x901, 0x11d1, 0x8b, 0x6, 
	                0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXSiteEventHandler	IID_IHXSiteEventHandler

#undef  INTERFACE
#define INTERFACE   IHXSiteEventHandler

DECLARE_INTERFACE_(IHXSiteEventHandler, IUnknown)
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
     * This method is called from Site Manager EventOccured().
     * The imeplementation of this interface must pass the events 
     * on to the individual CHXSiteWindowed sites.
     */
    STDMETHOD(EventOccurred) (THIS_ HXxEvent* pEvent) PURE;
};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteWindowless
 *
 *  Purpose:
 *
 *	Interface for IHXSite objects which are "windowless" or not 
 *	associated with platform specific window objects.
 *
 *  IID_IHXSiteWindowless:
 *
 *	{00000D02-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteWindowless, 0x00000D02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteWindowless

#define CLSID_IHXSiteWindowless IID_IHXSiteWindowless

DECLARE_INTERFACE_(IHXSiteWindowless, IUnknown)
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
     * IHXSiteWindowless methods called by owners of the site.
     */
    STDMETHOD(EventOccurred)	(THIS_
				HXxEvent* /*IN*/ pEvent) PURE;

    /*
     * IHXSiteWindowless method. Returns some parent window that
     * owns the windowless site. Useful for right-click menus and
     * dialog box calls.
     */
    STDMETHOD_(HXxWindow*,GetParentWindow)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSite
 *
 *  Purpose:
 *
 *	Interface for IHXSite objects.
 *
 *  IID_IHXSite:
 *
 *	{00000D03-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSite, 0x00000D03, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSite

DECLARE_INTERFACE_(IHXSite, IUnknown)
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
     * IHXSite methods usually called by the "context" to 
     * associate users with the site, and to create child sites
     * as appropriate.
     */
    STDMETHOD(AttachUser)	(THIS_
				IHXSiteUser*	/*IN*/	pUser) PURE;

    STDMETHOD(DetachUser)	(THIS) PURE;


    STDMETHOD(GetUser)		(THIS_
				REF(IHXSiteUser*) /*OUT*/ pUser) PURE;

    STDMETHOD(CreateChild)	(THIS_
				REF(IHXSite*)	/*OUT*/ pChildSite) PURE;

    STDMETHOD(DestroyChild)	(THIS_
				IHXSite*	/*IN*/	pChildSite) PURE;

    /*
     * IHXSite methods called by the the "context" in which the site
     * is displayed in order to manage its position. Site users should
     * not generally call these methods.
     */
    STDMETHOD(AttachWatcher)	(THIS_
				IHXSiteWatcher* /*IN*/	pWatcher) PURE;

    STDMETHOD(DetachWatcher)	(THIS) PURE;

    STDMETHOD(SetPosition)	(THIS_
				HXxPoint		position) PURE;

    STDMETHOD(GetPosition)	(THIS_
				REF(HXxPoint)		position) PURE;

    /*
     * IHXSite methods called by the user of the site to get
     * information about the site, and to manipulate the site.
     */
    STDMETHOD(SetSize)		(THIS_
				HXxSize			size) PURE;

    STDMETHOD(GetSize)		(THIS_
				REF(HXxSize)		size) PURE;

    STDMETHOD(DamageRect)	(THIS_
				HXxRect			rect) PURE;

    STDMETHOD(DamageRegion)	(THIS_
				HXxRegion		region) PURE;

    STDMETHOD(ForceRedraw)	(THIS) PURE;
};


// $Private
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteComposition
 *
 *  Purpose:
 *
 *	Interface for IHXSite objects to let them compose composition
 *      frames and display them on a regular basis rather than many
 *      discrete blts.
 *
 *  IID_IHXSiteComposition:
 *
 *	{00000D03-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteComposition, 0x00000D19, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteComposition

DECLARE_INTERFACE_(IHXSiteComposition, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /* IHXSiteComposition methods. */
    STDMETHOD(LockComposition)            (THIS) PURE;
    STDMETHOD(UnlockComposition)          (THIS) PURE;
    STDMETHOD(BltComposition)             (THIS) PURE;
    STDMETHOD(SetCompositionMode)         (THIS_ HXBOOL OnOrOff) PURE;
    STDMETHOD_(HXBOOL, IsCompositionLocked) (THIS) PURE;
    STDMETHOD_(HXBOOL, IsCompositionMode)   (THIS) PURE;
};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteUser
 *
 *  Purpose:
 *
 *	Interface for the user of the IHXSite objects.
 *
 *  IID_IHXSiteUser:
 *
 *	{00000D04-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteUser, 0x00000D04, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteUser

DECLARE_INTERFACE_(IHXSiteUser, IUnknown)
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
     * IHXSiteUser methods usually called by the "context" to 
     * associate users with the site.
     */
    STDMETHOD(AttachSite)	(THIS_
				IHXSite*	/*IN*/ pSite) PURE;

    STDMETHOD(DetachSite)	(THIS) PURE;

    /*
     * IHXSiteUser methods called to inform user of an event.
     */
    STDMETHOD(HandleEvent)	(THIS_
				HXxEvent*	/*IN*/ pEvent) PURE;

    STDMETHOD_(HXBOOL,NeedsWindowedSites)	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteWatcher
 *
 *  Purpose:
 *
 *	Interface for IHXSiteWatcher objects.
 *
 *  IID_IHXSite:
 *
 *	{00000D05-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteWatcher, 0x00000D05, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteWatcher

DECLARE_INTERFACE_(IHXSiteWatcher, IUnknown)
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
     * IHXSiteWatcher methods called by the site when a watcher 
     * is attached to or detached from it.
     */
    STDMETHOD(AttachSite)	(THIS_
				IHXSite*	/*IN*/	pSite) PURE;

    STDMETHOD(DetachSite)	(THIS) PURE;

    /*
     * IHXSiteWatcher methods called by the site an attempt is
     * made to change it's position or size. The site watcher must
     * return HXR_OK for the change to occur. If the site watcher
     * returns any value other than HXR_OK then the size or position
     * will not change. The site watcher can also modify the new
     * size of position.
     */
    STDMETHOD(ChangingPosition)	(THIS_
				HXxPoint		posOld,
				REF(HXxPoint)/*IN-OUT*/	posNew) PURE;

    STDMETHOD(ChangingSize)	(THIS_
				HXxSize			sizeOld,
				REF(HXxSize) /*IN-OUT*/	sizeNew) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteUserSupplier
 *
 *  Purpose:
 *
 *	Interface implemented by renderers and objects with provide layouts to
 *	the client core. This interface is called by the core when it needs a 
 *	new IHXSiteUser, or when it is done using an IHXSiteUser.
 *
 *  IID_IHXSiteUserSupplier:
 *
 *	{00000D06-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteUserSupplier, 0x00000D06, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteUserSupplier

DECLARE_INTERFACE_(IHXSiteUserSupplier, IUnknown)
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
     * IHXSiteUserSupplier methods usually called by the 
     * "context" to ask for additional or to release previously
     * created site users.
     */
    STDMETHOD(CreateSiteUser)	(THIS_
				REF(IHXSiteUser*)/*OUT*/ pSiteUser) PURE;

    STDMETHOD(DestroySiteUser)	(THIS_
				IHXSiteUser*	  /*IN*/ pSiteUser) PURE;

    STDMETHOD_(HXBOOL,NeedsWindowedSites)	(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteSupplier
 *
 *  Purpose:
 *
 *	Interface implemented by users of the client core. This interface is
 *	called by the core when it needs a new IHXSite, or when it is done
 *	using an IHXSite.
 *
 *  IID_IHXSiteSupplier:
 *
 *	{00000D07-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteSupplier, 0x00000D07, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteSupplier

DECLARE_INTERFACE_(IHXSiteSupplier, IUnknown)
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
     * IHXSiteSupplier methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSiteSupplier::SitesNeeded
     *	Purpose:
     *	  Called to inform the site supplier that a site with a particular
     *	  set of characteristics is needed. If the site supplier can 
     *	  fulfill the request it should call the site manager and add one
     *	  or more new sites.
     *    Note that the request for sites is associated with a Request ID
     *    the client core will inform the site supplier when this requested
     *    site is no longer needed.
     */
    STDMETHOD(SitesNeeded)	(THIS_
				UINT32			uReqestID,
				IHXValues*		pSiteProps) PURE;

    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::SitesNotNeeded
     *  Purpose:
     *    Called to inform the site supplier that all sites from a previos
     *	  site request are no longer needed. If the site supplier had 
     *	  previously created non-persistant sites (like popup windows)
     *    to fulfill a request for sites it should call the site manager 
     *    and remove those sites.
     */
    STDMETHOD(SitesNotNeeded)	(THIS_
				UINT32			uReqestID) PURE;


    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::BeginChangeLayout
     *  Purpose:
     *    Called to inform the site supplier a layout change has beginning
     *	  it can expect to receive SitesNeeded() and SitesNotNeeded() calls
     *	  while a layout change is in progress,
     */
    STDMETHOD(BeginChangeLayout) (THIS) PURE;

    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::DoneChangeLayout
     *  Purpose:
     *    Called to inform the site supplier the layout change has been
     *	  completed.
     */
    STDMETHOD(DoneChangeLayout) (THIS) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteManager
 *
 *  Purpose:
 *
 *	Interface implemented by the client core. This interface is called
 *	by users of the client core to inform it of IHXSite's which are
 *	available for layout of renderers
 *
 *  IID_IHXSiteManager:
 *
 *	{00000D08-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteManager, 0x00000D08, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteManager

DECLARE_INTERFACE_(IHXSiteManager, IUnknown)
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
     * IHXSiteManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSiteManager::AddSite
     *	Purpose:
     *	  Called to inform the site manager of the existence of a site.
     */
    STDMETHOD(AddSite)		(THIS_
				IHXSite*		pSite) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSiteManager::RemoveSite
     *	Purpose:
     *	  Called to inform the site manager that a site is no longer 
     *	  available.
     */
    STDMETHOD(RemoveSite)	(THIS_
				IHXSite*		pSite) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteManager2
 *
 *  Purpose:
 *
 *	Interface implemented by the client core. This interface is called
 *	by users of the client core to iterate over the sites known by this
 *	site manager.
 *
 *  IID_IHXSiteManager:
 *
 *	{00000D20-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteManager2, 0x00000D20, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteManager2

DECLARE_INTERFACE_(IHXSiteManager2, IUnknown)
{
    /*
     * IHXSiteManager2 methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSiteManager2::GetNumberOfSites
     *	Purpose:
     *	  Called to get the number of sites that the site mananger currently 
     *    knows about.
     */
    STDMETHOD(GetNumberOfSites)		(THIS_  REF(UINT32) nNumSites ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSiteManager2::GetSiteAt
     *	Purpose:
     *	  Used to iterate over the sites.
     *	  
     */
    STDMETHOD(GetSiteAt)	(THIS_ UINT32 nIndex, REF(IHXSite*) pSite) PURE;
};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXMultiInstanceSiteUserSupplier
 *
 *  Purpose:
 *
 *	This is the interface for a special default object which is available
 *	from the common class factory. This object will act as a site user 
 *	supplier for any renderer (or other site user object) that wants
 *	default support for multiple instances. The site user must work as
 *	a windowless site for this default implementation to work. The 
 *	default object also implements the IHXSite interfave to allow
 *	the site user object to control all the sites through a single 
 *	interface instance.
 *
 *  IID_IHXMultiInstanceSiteUserSupplier:
 *
 *	{00000D09-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXMultiInstanceSiteUserSupplier, 0x00000D09, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXMultiInstanceSiteUserSupplier	\
		IID_IHXMultiInstanceSiteUserSupplier

#undef  INTERFACE
#define INTERFACE   IHXMultiInstanceSiteUserSupplier

DECLARE_INTERFACE_(IHXMultiInstanceSiteUserSupplier, IUnknown)
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
     * IHXMultiInstanceSiteUserSupplier methods called by site users.
     */
    STDMETHOD(SetSingleSiteUser)		(THIS_ 
						IUnknown*	pUnknown) PURE;

    STDMETHOD(ReleaseSingleSiteUser)		(THIS) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteEnumerator
 *
 *  Purpose:
 *
 *      Provides an interface to enumerate sites.  Currently implemented 
 *      in the IHXMultiInstanceSiteUserSupplier supplied by the core player,
 *      it can be used to render to MISUS sites outside of an HX_SURFACE_UPDATE;
 *      this is especially useful when using IHXVideoSurface2.
 *
 *  IID_IHXSiteEnumerator:
 *
 *	{67f8c5bd-4b1d-4c09-8fb7-8ac7c20d29c7}
 *
 */
DEFINE_GUID(IID_IHXSiteEnumerator, 0x67f8c5bd, 0x4b1d,  
            0x4c09, 0x8f, 0xb7, 0x8a, 0xc7, 0xc2, 0x0d, 0x29, 0xc7);

#undef  INTERFACE
#define INTERFACE   IHXSiteEnumerator

DECLARE_INTERFACE_(IHXSiteEnumerator, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    typedef void* SitePosition;

    /*
     * HXSiteEnumerator methods
     */


    /************************************************************************
     *	Method:
     *	    HXSiteEnumerator::GetFirstSite
     *	Purpose:
     *	    Retrieves both the first site in the enumeration and initializes 
     *      sitePosition with the position of the next site (if any).  
     *      Returns HXR_OK if the first site is available, HXR_FAIL if not.
     */
    STDMETHOD(GetFirstSite) (THIS_
                             REF(IHXSite*) /* OUT */ pFirstSite,
                             REF(SitePosition) /* OUT */ nextPosition) PURE;

    /************************************************************************
     *	Method:
     *	    HXSiteEnumerator::GetNextSite
     *	Purpose:
     *	    Retrieves both the next site in the enumeration (as specified by
     *      nextSite) and initializes sitePosition with the position of the 
     *      following site (if any).  
     *      Returns HXR_OK if the first site is available, HXR_FAIL if not.
     */
    STDMETHOD(GetNextSite)  (THIS_
                             REF(IHXSite*) pNextSite,
                             REF(SitePosition) /* IN/OUT */ nextPosition) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *	IHXSiteFullScreen
 *
 *  Purpose:
 *
 *	This is the interface for turning on/off the full screen mode
 *
 *  IID_IHXSiteFullScreen:
 *
 *	{00000D0B-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteFullScreen, 0x00000D0B, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteFullScreen

DECLARE_INTERFACE_(IHXSiteFullScreen, IUnknown)
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
     * IHXSiteFullScreen methods
     */
    STDMETHOD(EnterFullScreen)	(THIS) PURE;

    STDMETHOD(ExitFullScreen)	(THIS) PURE;

    STDMETHOD(TestFullScreen)	(THIS_
				void* hTestBitmap,const char* pszStatusText) PURE;

    STDMETHOD_(HXBOOL, IsFullScreen) (THIS) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 *	IHXLayoutSiteGroupManager
 *
 *  Purpose:
 *
 *	Allow layout site groups to be added and removed
 *
 *  IID_IHXLayoutSiteGroupManager:
 *
 *	{00000D0C-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXLayoutSiteGroupManager, 0x00000D0C, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXLayoutSiteGroupManager

DECLARE_INTERFACE_(IHXLayoutSiteGroupManager, IUnknown)
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
     * IHXLayoutSiteGroupManager methods
     */
    STDMETHOD(AddLayoutSiteGroup)	(THIS_
					IUnknown* pLSG) PURE;

    STDMETHOD(RemoveLayoutSiteGroup)	(THIS_
					IUnknown* pLSG) PURE;
};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 *	IHXEventHookMgr
 *
 *  Purpose:
 *
 *	Add ability to hook events from a named region
 *
 *  IID_IHXEventHookMgr:
 *
 *	{00000D0D-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXEventHookMgr, 0x00000D0D, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXEventHookMgr

DECLARE_INTERFACE_(IHXEventHookMgr, IUnknown)
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
     * IHXEventHookMgr methods
     */
    STDMETHOD(AddHook)		(THIS_
				IHXEventHook* pHook,
				const char* pRegionName,
				UINT16 uLayer) PURE;

    STDMETHOD(RemoveHook)	(THIS_
    				IHXEventHook* pHook,
				const char* pRegionName,
				UINT16 uLayer) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *	IHXEventHook
 *
 *  Purpose:
 *
 *	Object that gets hooked events sent by IHXEventHookMgr
 *
 *  IID_IHXEventHookMgr:
 *
 *	{00000D0E-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXEventHook, 0x00000D0E, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXEventHook

DECLARE_INTERFACE_(IHXEventHook, IUnknown)
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
     * IHXEventHook methods
     */
    STDMETHOD(SiteAdded)	(THIS_
				IHXSite* pSite) PURE;
    STDMETHOD(HandleEvent)	(THIS_
    				IHXSite* pSite,
				HXxEvent* pEvent) PURE;
    STDMETHOD(SiteRemoved)	(THIS_
    				IHXSite* pSite) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *	IHXStatusMessage
 *
 *  Purpose:
 *
 *	This is the interface for setting the status text. 
 *
 *  IID_IHXStatusMessage:
 *
 *	{00000D10-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXStatusMessage, 0x00000D10, 0x901, 
            0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXStatusMessage

DECLARE_INTERFACE_(IHXStatusMessage, IUnknown)
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
     * IHXStatusMessage methods
     */

    STDMETHOD(SetStatus)		(THIS_ const char* pText) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteTransition
 *
 *  Purpose:
 *
 *	Interface for transitioning between IHXSites.
 *
 *  IID_IHXSiteTransition:
 *
 *	{00000D01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteTransition, 0x00000D13, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXSiteTransition IID_IHXSiteTransition

#undef  INTERFACE
#define INTERFACE   IHXSiteTransition

DECLARE_INTERFACE_(IHXSiteTransition, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD(Initialize)	(THIS_
				IHXValues* pParams) PURE;

    STDMETHOD(SetPercentage)	(THIS_
				UINT32 nThousandnthsComplete) PURE;

};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXRegion
 *
 *  Purpose:
 *
 *  Interface for managing HXRegions.
 *
 *  IHXRegion:
 *
 *  {00002200-0903-11d1-8B06-00A024406D59}
 *
 */


DEFINE_GUID(IID_IHXRegion, 0x00000D14, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXRegion IID_IHXRegion

#undef  INTERFACE
#define INTERFACE   IHXRegion

DECLARE_INTERFACE_(IHXRegion, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXRegion::SetRect
     *	Purpose:
     *	    This function creates a rectangular region.
     *
     */
    STDMETHOD(SetRect)      (THIS_ HXxRect*    pRect) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::SetRect
     *	Purpose:
     *	    This function creates a rectangular region.
     *
     */
    STDMETHOD(SetRect)      (THIS_ int x, int y, int x1, int y1) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::SetPoly
     *	Purpose:
     *	    This function creates a region defined by an arbitrary polygon.
     *
     */

    STDMETHOD(SetPoly)      (THIS_ HXxPoint**  pRect, HXBOOL bUseWinding) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::IsEqual
     *	Purpose:
     *	    This function determines if two regions are equal.
     *
     */

    STDMETHOD_(HXBOOL,IsEqual)      (THIS_ IHXRegion* pRegion) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetExtents
     *	Purpose:
     *	    This function allows the user to determine the extents of the region
     *
     */

    STDMETHOD(GetExtents)      (THIS_ REF(HXxRect) rExtents) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::Offset
     *	Purpose:
     *	    This function offsets the region by the spectified origin
     *
     */

    STDMETHOD(Offset)      (THIS_ HXxPoint* pOrigin) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::PointHitTest
     *	Purpose:
     *	    This function returns if TRUE if the specified point is in the region
     *
     */

    STDMETHOD_(HXBOOL,PointHitTest)      (THIS_ HXxPoint* pPoint) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::RectHitTest
     *	Purpose:
     *	This function returns 
     *	    HX_CONTAINED	if the rect is fully contained within the region
     *	    HX_PART		if part of the rect is within the region
     *	    HX_OUT		if no part of the rect is within the region
     *
     */

    STDMETHOD_(INT32,RectHitTest)      (THIS_ HXxRect* pRect) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetNumRects
     *	Purpose:
     *	    This function gets the number of rects which describe the 
     *	    region
     *
     */

    STDMETHOD_(UINT32, GetNumRects)      (THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRectAtIndex
     *	Purpose:
     *	    This function gets the RECT at index nRectIndex
     *
     */

    STDMETHOD(GetRectAtIndex)      (THIS_ UINT32 nRectIndex, REF(HXxRect) rRect) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::UnionRegion
     *	Purpose:
     *		Union Region -- this operator like all of the operators will create an IHXRegion 
     *		if pDest is NULL.
     *
     */

    STDMETHOD(UnionRegion)      (THIS_ REF(IHXRegion*) pDest, IHXRegion* pSrc1) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRectAtIndex
     *	Purpose:
     *	    Copy Region
     *
     */

    STDMETHOD(CopyRegion)      (THIS_ REF(IHXRegion*) pDest) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRectAtIndex
     *	Purpose:
     *	    Diff Region
     *
     */

    STDMETHOD(DiffRegion)      (THIS_ REF(IHXRegion*) pDest, IHXRegion* pSrc1) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRectAtIndex
     *	Purpose:
     *	    And Region
     *
     */

    STDMETHOD(AndRegion)      (THIS_ REF(IHXRegion*) pDest, IHXRegion* pSrc1) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRectAtIndex
     *	Purpose:
     *	    XOR Region
     *
     */

    STDMETHOD(XORRegion)      (THIS_ REF(IHXRegion*) pDest, IHXRegion* pSrc1) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRegion::GetRegion
     *	Purpose:
     *	    Bad hack for the moment to obtain the REGION pointer of an HXRegion.
     *	    Will have to remove this later.
     *
     */

    STDMETHOD_(void*, GetRegion)      (THIS) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXColorConverterManager
 *
 *  Purpose:
 *
 *  Interface for obtaining IHXColorConverters
 *
 *  IHXColorConverterManager:
 *
 *  {00000D15-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXColorConverterManager, 0x00000D15, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXColorConverterManager

DECLARE_INTERFACE_(IHXColorConverterManager, IUnknown)
{
    /*
     * Get ColorConverter is called to obtain a color converter to convert 
     * from a particular bitmap to another bitmap.
     */
    STDMETHOD(GetColorConverter)      (THIS_
                HXBitmapInfoHeader*    /*IN*/  pBitmapInfoIn,
                HXBitmapInfoHeader*    /*IN*/  pBitmapInfoOut,
		REF(IHXColorConverter*) /*OUT*/ pConverter) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXColorConverter
 *
 *  Purpose:
 *
 *  Interface for converting between two bitmaps of different color formats.
 *
 *  IHXColorConverterManager:
 *
 *  {00000D16-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXColorConverter, 0x00000D16, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXColorConverter

DECLARE_INTERFACE_(IHXColorConverter, IUnknown)
{
    /*
     * ColorConvert converts the pBitsIn from one color format to 
     * the format of pBitsOut
     */
    STDMETHOD(ColorConvert)      (THIS_
                UCHAR*	    pBitsIn,
                UCHAR*	    pBitsOut,
		HXxRect*    pRectIn,
		HXxRect*    pRectOut
                ) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXOverlayResponse
 *
 *  Purpose:
 *
 *  Interface for reporting/computing the current statistics relevant to 
 *  Video Presentations.
 *
 *  IHXOverlayResponse:
 *
 *  {00000D22-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXOverlayResponse, 0x00000D22, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXOverlayResponse

DECLARE_INTERFACE_(IHXOverlayResponse, IUnknown)
{
    STDMETHOD(OverlayGranted)      (THIS ) PURE;
    STDMETHOD(OverlayRevoked)      (THIS ) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXOverlayManager
 *
 *  Purpose:
 *
 *  Interface for reporting/computing the current statistics relevant to 
 *  Video Presentations.
 *
 *  IHXOverlayManager:
 *
 *  {00000D21-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXOverlayManager, 0x00000D21, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXOverlayManager

DECLARE_INTERFACE_(IHXOverlayManager, IUnknown)
{
    STDMETHOD(HasOverlay)      (THIS_
                    IHXOverlayResponse* pResp
                    ) PURE;
    
    STDMETHOD(AddStats)      (THIS_
                    IHXOverlayResponse* pResp,
                    UINT32    ulNumPixels
                    ) PURE;

    
    STDMETHOD(RemoveOverlayRequest)(THIS_ IHXOverlayResponse* pResp ) PURE;
};


// $EndPrivate.

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXKeyBoardFocus
 *
 *  Purpose:
 *
 *  Interface for setting/getting the keyboard focus for a particular siteuser
 *
 *  IHXKeyBoardFocus:
 *
 *  {00000D23-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXKeyBoardFocus, 0x00000D23, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXKeyBoardFocus

DECLARE_INTERFACE_(IHXKeyBoardFocus, IUnknown)
{
    STDMETHOD(SetKeyboardFocus)(THIS_ IHXSiteUser* pSiteUser ) PURE;
    STDMETHOD(GetKeyboardFocus)(THIS_ IHXSiteUser* &pSiteUser ) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXDrawFocus
 *
 *  Purpose:
 *
 *  Interface for displaying the site that has the keyboard focus
 *
 *  IHXDrawFocus:
 *
 *  {00000D24-0902-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXDrawFocus, 0x00000D24, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDrawFocus

DECLARE_INTERFACE_(IHXDrawFocus, IUnknown)
{
    /************************************************************************
     *  Method:
     *          IHXDrawFocus::SetStyle
     *  Purpose:
     *          Ask the site to set the focus style.
     *
     *  Syles:
     *          ULONG32 Properties:
     *
     *          LINE_STYLE = HX_SOLID_LINE, HX_DASHED_LINE, HX_DOTTED_LINE,
     *                       HX_CUSTOM_LINE
     *          LINE_WIDTH = Width of the line in pixels
     *          RED = 0-255 color of the primary pixel
     *          GREEN = 0-255 color of the primary pixel
     *          BLUE = 0-255 color of the primary pixel
     *          RED_OFF = 0-255 color of the secondary pixel
     *          GREEN_OFF = 0-255 color of the secondary pixel
     *          BLUE_OFF = 0-255 color of the secondary pixel
     *          CUSTOM_LINE_ENTRIES number of ULONG32s in CUSTOM_LINE_PATTERN
     *
     *          IHXBuffer Properties:
     *
     *          CUSTOM_LINE_PATTERN list of ULONG32s describing the number
     *           of primary and secondary pixels (eq 4241 = "----..----." where
     *           - is a primary pixel and . is a secondary pixel) and
     *           CUSTOM_LINE_ENTRIES equals 4.  Secondary pixels are not
     *           drawn if RED_OFF, GREEN_OFF, and BLUE_OFF are not set.
     */
    STDMETHOD(SetStyle) (THIS_ IHXValues* pProperties) PURE;

    /************************************************************************
     *  Method:
     *          IHXDrawFocus::ClearFocus
     *  Purpose:
     *          Ask the site to clear the current focus drawing.
     */
    STDMETHOD(ClearFocus)(THIS) PURE;

    /************************************************************************
     *  Method:
     *          IHXDrawFocus::SetFocusPolygon
     *  Purpose:
     *          Ask the site to draw polygon around focus
     */
    STDMETHOD(SetFocusPolygon)(THIS_  HXxPoint* pPoints, ULONG32 numPoints) PURE;

    /************************************************************************
     *  Method:
     *          IHXDrawFocus::SetFocusRect
     *  Purpose:
     *          Ask the site to draw rectangle around focus
     */
    STDMETHOD(SetFocusRect) (THIS_ HXxRect* pRect) PURE;

    /************************************************************************
     *  Method:
     *          IHXDrawFocus::SetFocusEllipse
     *  Purpose:
     *          Ask the site to draw ellipse around focus
     */
    STDMETHOD(SetFocusEllipse) (THIS_ HXxRect* pRect) PURE;
};

// $Private:

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXSubRectSite
 *
 *  Purpose:
 *
 *  Interface to determine if a site support sub rect BLT'ing via
 *  the HX_SURFACE_UPDATE2 message. If the site does support sub
 *  rect BLT'ing you can tell the site to send you the
 *  HX_SURFACE_UPDATE2 messages via this interface.
 *
 *  IHXSubRectSite:
 *
 *  {00000D25-0902-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSubRectSite, 0x00000D25, 0x903, 0x11d1, 0x8b,
    0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSubRectSite

DECLARE_INTERFACE_(IHXSubRectSite, IHXSite)
{
    /*
     * Tells the site to send/not-send HX_SURFACE_UPDATE2 messages.
     * These messages contain actuall dirty rects so that the renderer
     * does not need to BLT the entire frame.
     */
    STDMETHOD(SendSubRectMessages) (THIS_ HXBOOL bRet ) PURE;
    /*
     * New damage region call that takes the cross platfrom region
     * defined in hxwintyp.h and invalidates the rects in it
     */
    STDMETHOD(SubRectDamageRegion) (THIS_ HXxBoxRegion* pRegion ) PURE;
    /*
     * Method to get the new video surface that comes with the sub
     * rect BLT'ing support.
     */
    STDMETHOD(GetSubRectVideoSurface) (THIS_ 
                                       REF(IHXSubRectVideoSurface*) pSurface
                                       ) PURE;
};

// $EndPrivate.

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXFocusNavigation
 *
 *  Purpose:
 *
 *  Interface for navigating between and within keyboard focus sites
 *
 *  IHXFocusNavigation:
 *
 *  {B42B7677-F605-438e-9002-E2AAB7784B43}
 *
 */

DEFINE_GUID(IID_IHXFocusNavigation, 0xb42b7677, 0xf605, 0x438e, 0x90,
            0x2, 0xe2, 0xaa, 0xb7, 0x78, 0x4b, 0x43);

#undef  INTERFACE
#define INTERFACE   IHXFocusNavigation

DECLARE_INTERFACE_(IHXFocusNavigation, IUnknown)
{
    /************************************************************************
     *  Method:
     *          IHXFocusNavigation::SetFocus
     *  Purpose:
     *          Ask the renderer to set the focus to the given item.
     */
    STDMETHOD(SetFocus) (THIS_ HXFocusContext eFocus) PURE;

    /************************************************************************
     *  Method:
     *          IHXFocusNavigation::ClearFocus
     *  Purpose:
     *          Ask the renderer to clear the current focus.
     */
    STDMETHOD(ClearFocus) (THIS) PURE;

    /************************************************************************
     *  Method:
     *          IHXFocusNavigation::ActivateFocus
     *  Purpose:
     *          Ask the renderer to activate the focused link. Do nothing if 
     *   there is no focus point.
     */
    STDMETHOD(ActivateFocus) (THIS) PURE;

    /************************************************************************
     *  Method:
     *          IHXFocusNavigation::GetFocusState
     *  Purpose:
     *          Obtain the current focus state
     */
    STDMETHOD_(HXFocusState,GetFocusState) (THIS) PURE;
};

#endif /* _HXWIN_H_ */
