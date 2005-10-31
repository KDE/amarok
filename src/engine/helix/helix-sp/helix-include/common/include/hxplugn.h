
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

#ifndef _HXPLUGN_H_
#define _HXPLUGN_H_

#include "hxcom.h"
#include "hxplugncompat.h"

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IUnknown			    IUnknown;
typedef _INTERFACE  IHXPlugin			    IHXPlugin;
typedef _INTERFACE  IHXPluginEnumerator	    IHXPluginEnumerator;
// $Private:
typedef _INTERFACE  IHXPluginSearchEnumerator	    IHXPluginSearchEnumerator;
typedef _INTERFACE  IHXPluginChallenger	    IHXPluginChallenger;
// $EndPrivate.
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXValues			    IHXValues;
typedef _INTERFACE  IHXPreferences		    IHXPreferences;

// Plugin Types.
#define	PLUGIN_FILESYSTEM_TYPE	    "PLUGIN_FILE_SYSTEM"
#define	PLUGIN_FILEFORMAT_TYPE	    "PLUGIN_FILE_FORMAT"
#define	PLUGIN_FILEWRITER_TYPE	    "PLUGIN_FILE_WRITER"
#define	PLUGIN_METAFILEFORMAT_TYPE  "PLUGIN_METAFILE_FORMAT"
#define	PLUGIN_RENDERER_TYPE	    "PLUGIN_RENDERER"
#define PLUGIN_DEPACKER_TYPE        "PLUGIN_DEPACKER"
#define PLUGIN_REVERTER_TYPE	    "PLUGIN_REVERTER"
#define PLUGIN_BROADCAST_TYPE	    "PLUGIN_BROADCAST"
#define PLUGIN_STREAM_DESC_TYPE	    "PLUGIN_STREAM_DESC"
#define PLUGIN_ALLOWANCE_TYPE	    "PLUGIN_ALLOWANCE"
#define PLUGIN_PAC_TYPE		    "PLUGIN_PAC"
#define PLUGIN_CLASS_FACTORY_TYPE   "PLUGIN_CLASS_FACT"

#define	PLUGIN_CLASS		    "PluginType"
#define PLUGIN_FILENAME		    "PluginFilename"
#define PLUGIN_REGKEY_ROOT	    "PluginHandlerData"
#define PLUGIN_PLUGININFO	    "PluginInfo"
#define PLUGIN_GUIDINFO		    "GUIDInfo"
#define PLUGIN_NONHXINFO	    "NonHXDLLs"
#define PLUGIN_IDENTIFIER	    "Plugin#"
// This may no longer be needed...
#define PLUGINDIRECTORYHASH	    "DirHash"	    
// XXXAH WHO is defining this ... I think I know..
#define PLUGIN_DESCRIPTION2	    "Description"	
// XXXAH WHO is defining this ... I think I know..
#define PLUGIN_FILE_HASH	    "FileHash"
#define PLUGIN_INDEX		    "IndexNumber"
#define PLUGIN_FILENAMES	    "FileInfo"
#define PLUGIN_COPYRIGHT2	    "Copyright"
#define PLUGIN_LOADMULTIPLE	    "LoadMultiple"
#define PLUGIN_VERSION		    "Version"
#define PLUGIN_FILESYSTEMSHORT	    "FileShort"
#define PLUGIN_FILESYSTEMPROTOCOL   "FileProtocol"
#define PLUGIN_FILEMIMETYPES	    "FileMime"
#define PLUGIN_FILEEXTENSIONS	    "FileExtensions"
#define PLUGIN_FILEOPENNAMES	    "FileOpenNames"
#define PLUGIN_RENDERER_MIME	    "RendererMime"
#define PLUGIN_RENDERER_GRANULARITY "Renderer_Granularity"
#define PLUGIN_DEPACKER_MIME        "DepackerMime"
#define PLUGIN_REVERTER_MIME	    "ReverterMime"
#define PLUGIN_BROADCASTTYPE	    "BroadcastType"
#define PLUGIN_STREAMDESCRIPTION    "StreamDescription"

// 
#define PLUGIN_GUID_RESPONSE	    "MainGuid"
#define PLUGIN_FACTORY_GUIDS	    "" // These are comma delimited.

//

#define PLUGIN_NUM_PLUGINS	    "NumPlugins"
#define PLUGIN_FILE_CHECKSUM	    "DLLCheckSum"
#define PLUGIN_DLL_SIZE		    "DLLSize"
#define PLUGIN_HAS_FACTORY	    "DLLHasFactory"

/****************************************************************************
 * 
 *  Function:
 * 
 *	HXCreateInstance()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to create an instance of 
 *	any of the objects supported by the DLL. This method is similar to 
 *	Window's CoCreateInstance() in its purpose, except that it only 
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore an outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 * 
 */
#ifdef _MAC_CFM
#pragma export on
#endif

STDAPI HXCreateInstance
		(
		    IUnknown**  /*OUT*/	ppIUnknown
		);
		
#ifdef _MAC_CFM
#pragma export off
#endif


/****************************************************************************
 * 
 *  Function:
 * 
 *	HXShutdown()
 * 
 *  Purpose:
 * 
 *	Function implemented by all plugin DLL's to free any *global* 
 *	resources. This method is called just before the DLL is unloaded.
 *
 */
#ifdef _MAC_CFM
#pragma export on
#endif

STDAPI HXShutdown(void);
		
#ifdef _MAC_CFM
#pragma export off
#endif


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPlugin
 * 
 *  Purpose:
 * 
 *	Interface exposed by a plugin DLL to allow inspection of objects
 *	supported by the plugin DLL.
 * 
 *  IID_IHXPlugin:
 * 
 *	{00000C00-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlugin, 0x00000C00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPlugin

DECLARE_INTERFACE_(IHXPlugin, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXPlugin methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    bMultipleLoad	Whether or not this plugin can be instantiated
     *				multiple times. All File Formats must set
     *				this value to TRUE.  The only other type of
     *				plugin that can specify bMultipleLoad=TRUE is
     *				a filesystem plugin.  Any plugin that sets
     *				this flag to TRUE must not use global variables
     *				of any type.
     *
     *				Setting this flag to TRUE implies that you
     *				accept that your plugin may be instantiated
     *				multiple times (possibly in different
     *				address spaces).  Plugins are instantiated
     *				multiple times only in the server (for
     *				performance reasons).
     *
     *				An example of a plugin, that must set this
     *				flag to FALSE is a filesystem plugin that 
     *				uses a single TCP connection to communicate
     *				with a database.
     *				
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     *	    ulVersionNumber	The version of this plugin.
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bMultipleLoad,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginEnumerator
 * 
 *  Purpose:
 * 
 *	provide methods to enumerate through all the plugins installed
 * 
 *  IID_IHXPluginEnumerator:
 * 
 *	{00000C01-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPluginEnumerator, 0x00000C01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginEnumerator

DECLARE_INTERFACE_(IHXPluginEnumerator, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXPluginEnumerator methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginEnumerator::GetNumOfPlugins
     *
     *	Purpose:    
     *	    return the number of plugins available
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginEnumerator::GetPlugin
     *	Purpose:
     *	    Return an instance (IUnknown) of the plugin
     *
     */
    STDMETHOD(GetPlugin)   (THIS_
			   ULONG32	   /*IN*/  ulIndex,
			   REF(IUnknown*)  /*OUT*/ pPlugin) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginGroupEnumerator
 * 
 *  Purpose:
 * 
 *	Provide a way to enumerate through all of the plugins which
 *	implement a specific interface.
 * 
 *  IID_IHXPluginGroupEnumerator:
 * 
 *	{00000C02-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPluginGroupEnumerator, 0x00000C02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginGroupEnumerator

#define CLSID_IHXPluginGroupEnumerator IID_IHXPluginGroupEnumerator

DECLARE_INTERFACE_(IHXPluginGroupEnumerator, IUnknown)
{
    /*
     * IUnknown methods
     */

    /*
     * IHXPluginGroupEnumerator methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /******************************************************************
     * Method:
     *	    IHXPluginGroupEnumerator::Init
     *
     * Purpose:
     *	    tell the group enumerator which interface to group the plugins
     *     into, this method must be called before the other methods can
     *     be called.
     *
     */
    STDMETHOD(Init)         (THIS_
                            REFIID    iid) PURE;


    /******************************************************************
     * Method:
     *     IHXPluginGroupEnumerator::GetNumOfPlugins
     *
     * Purpose:
     *     return the number of plugins available that support a
     *	   particular interface.
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins) (THIS) PURE;


    /******************************************************************
     * Method:
     *     IHXPluginGroupEnumerator::GetPlugin
     * Purpose:
     *     Return an instance (IUnknown) of the plugin
     *
     */
    STDMETHOD(GetPlugin)   (THIS_
      UINT32    /*IN*/  ulIndex,
      REF(IUnknown*)  /*OUT*/ pPlugin) PURE;

};



// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginSearchEnumerator
 * 
 *  Purpose:
 *	Walk through the result set of a plugin search 
 *
 * {3244B391-42D4-11d4-9503-00902790299C}
 * 
 */

DEFINE_GUID( IID_IHXPluginSearchEnumerator, 
    0x3244b391, 0x42d4, 0x11d4, 0x95, 0x3, 0x0, 0x90, 0x27, 0x90, 0x29, 0x9c);

#undef  INTERFACE
#define INTERFACE   IHXPluginSearchEnumerator

DECLARE_INTERFACE_( IHXPluginSearchEnumerator, IUnknown )
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG32,Release)(THIS) PURE;


    /*
     *	IHXPluginSearchEnumerator methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::GetNumPlugins
     *
     *	Purpose:    
     *	    Returns numbers of plugins found during search
     *
     */
    STDMETHOD_( UINT32, GetNumPlugins)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::GoHead
     *
     *	Purpose:    
     *	    Moves the iterator to the beginning of the collection
     *
     */
    STDMETHOD_(void, GoHead)(THIS) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::GetNextPlugin
     *
     *	Purpose:    
     *	    Returns an instance of the next plugin in the collection
     *
     */
    STDMETHOD(GetNextPlugin)( THIS_ REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::GetNextPluginInfo
     *
     *	Purpose:    
     *	    Gets information about the next plugin in the list
     *
     */
    STDMETHOD(GetNextPluginInfo)( THIS_ REF(IHXValues*) pRetValues ) PURE;
    

    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::GetPluginAt
     *
     *	Purpose:    
     *	    Returns an instance of a plugin at a specific index in the list
     *
     */
    STDMETHOD(GetPluginAt)( THIS_ UINT32 index, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginSearchEnumerator::
     *
     *	Purpose:    
     *	    Returns information about a plugin at a specific index in the list
     *
     */
    STDMETHOD(GetPluginInfoAt)( THIS_ UINT32 index, 
				    REF(IHXValues*) pRetValues ) PURE;

};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginReloader
 * 
 *  Purpose:
 * 
 *	Tells the client core to reload all plugins.
 * 
 *  IID_IHXPluginReloader:
 * 
 *	{00000C03-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPluginReloader, 0x00000C03, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginReloader

DECLARE_INTERFACE_(IHXPluginReloader, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXPluginReloader methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginReloader::ReloadPlugins
     *	Purpose:    
     *	    Causes the client core to reload all plugins.
     *
     */
    STDMETHOD(ReloadPlugins)	(THIS) PURE;
};



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginFactory
 * 
 *  Purpose:
 * 
 *	This interface is implemented by a plugin in order to have more then
 *	    one "RMA plugin" in a single DLL.  I.e., a plugin author could
 *	    use this interface to have 3 different file format plugins in
 *	    a single DLL.
 * 
 *  IID_IHXPluginFactory:
 * 
 *	{00000C04-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPluginFactory, 0x00000C04, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginFactory

DECLARE_INTERFACE_(IHXPluginFactory, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;
    
    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *	IHXPluginFactory methods
     */
    
    /*****************************************************************
     *	Method:
     *	    IHXPluginFactory::GetNumPlugins
     *	Purpose:
     *	    Report the number of Plugins within the DLL.
     *
     *	Parameters:
     */
    STDMETHOD_(UINT16, GetNumPlugins) (THIS)  PURE;

    /*****************************************************************
     *	Method:
     *	    IHXPluginFactory::GetPlugin
     *	Purpose:
     *	    Returns an IUnknown interface to the requested plugin. 
     *	
     *	Parameters:
     */

    STDMETHOD(GetPlugin) (THIS_
			 UINT16 	uIndex, 
			 IUnknown**  	pPlugin) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginChallenger
 * 
 *  Purpose:
 * 
 *	This interface is implemented by a plugin in order to allow
 *	verification of a plugin's authenticity, by issuing a challenge
 *	and receiving a challenge response.
 * 
 *  IID_IHXPluginChallenger:
 * 
 *	{00000C05-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPluginChallenger, 0x00000C05, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginChallenger

typedef struct _HXTimeval HXTimeval;

DECLARE_INTERFACE_(IHXPluginChallenger, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					 REFIID riid,
					 void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;
    
    STDMETHOD_(ULONG32,Release)		(THIS) PURE;
    
    /*
     *	IHXPluginChallenger methods
     */
    
    /*****************************************************************
     *	Method:
     *	    IHXPluginChallenger::Challenge
     *	Purpose:
     *	    Challenge the plugin's authenticity. Returns a challenge
     *	    response which the caller can use to verify that the
     *	    plugin is authentic.
     *
     *	Parameters:
     *	    tVal    A time value which may be used to create the
     *		    challenge response.
     */
    STDMETHOD(Challenge) (THIS_
			 HXTimeval	  /*IN*/  tVal,
			 REF(IHXBuffer*) /*OUT*/ pResponse) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginQuery
 * 
 *  Purpose:
 * 
 *	Queries the plugin handler for information on plugins.
 * 
 *  IID_IHXPluginQuery:
 * 
 *	{00000C06-0901-11d1-8B06-00A024406D59}
 * 
 */

#define PLUGIN_FILE_PATH	    "PluginFilePath"

#define PLUGIN_PATH		"PlgPath"
#define PLUGIN_DESCRIPTION	"PlgDesc"
#define PLUGIN_COPYRIGHT	"PlgCopy"
#define PLUGIN_MOREINFO		"PlgMore"
#define PLUGIN_MIMETYPES	"PlgMime"
#define PLUGIN_EXTENSIONS	"PlgExt"
#define PLUGIN_OPENNAME		"PlgOpen"
#define PLUGIN_MULTIPLE		"PlgMult"

DEFINE_GUID(IID_IHXPluginQuery, 0x00000C06, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPluginQuery

DECLARE_INTERFACE_(IHXPluginQuery, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXPluginQuery methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPluginQuery::GetNumPluginsGivenGroup
     *
     *	Purpose:    
     *	    Gets the number of plugins associated with a particular class id.
     *
     */
    STDMETHOD(GetNumPluginsGivenGroup)	(THIS_ REFIID riid, 
					REF(UINT32) /*OUT*/ unNumPlugins) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginQuery::GetPluginInfo
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD(GetPluginInfo)	(THIS_ REFIID riid, 
				UINT32 unIndex, REF(IHXValues*) /*OUT*/ Values) PURE;
};
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXGenericPlugin
 * 
 *  Purpose:
 * 
 *	Interface exposed by a plugin DLL to inform the client / server core
 *	that your plugin wishes to have InitPlugin called immediately.
 *
 *  IID_IHXGenericPlugin:
 * 
 *	{00000C09-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXGenericPlugin, 0x00000C09, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXGenericPlugin

DECLARE_INTERFACE_(IHXGenericPlugin, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXGenericPlugin methods
     */

    STDMETHOD(IsGeneric)	(THIS_
				REF(HXBOOL)	 /*OUT*/ bIsGeneric) PURE;
};


DEFINE_GUID(IID_IHXPluginHandler,	0x00000200, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

DEFINE_GUID(IID_IHXPlugin2Handler,	0x00000201, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPlugin2Handler

DECLARE_INTERFACE_(IHXPlugin2Handler, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXPlugin2Handler Methods
     */

     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Init
     *
     *	Purpose:    
     *	    Specifies the context and sets the pluginhandler in motion.
     *
     */
    STDMETHOD(Init)    (THIS_ IUnknown* pContext) PURE;
     
     /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetNumPlugins2
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD_(ULONG32,GetNumOfPlugins2)    (THIS) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetPluginInfo
     *
     *	Purpose:    
     *	    Gets the info of a particular plugin.
     *
     */
    STDMETHOD(GetPluginInfo)	(THIS_ 
				UINT32 unIndex, 
				REF(IHXValues*) /*OUT*/ Values) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FlushCache()
     *
     *	Purpose:    
     *	    Flushes the LRU cache -- Unloads all DLLs from memory 
     *	    which currenltly have a refcount of 0.
     */

    STDMETHOD(FlushCache)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetCacheSize
     *
     *	Purpose:    
     *	    This function sets the size of the Cache. The cache is 
     *	    initally set to 1000KB. To disable the cache simply set
     *	    the size to 0.If the cache is disabled a DLL will be 
     *	    unloaded whenever it's refcount becomes zero. Which MAY
     *	    cause performance problems.
     */

    STDMETHOD(SetCacheSize)	(THIS_ ULONG32 nSizeKB) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::GetInstance
     *
     *	Purpose:    
     *	    
     *	    This function will return a plugin instance given a plugin index.
     *		
     */

    STDMETHOD(GetInstance) (THIS_ UINT32 index, REF(IUnknown*) pUnknown) PURE; 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     * 
     */

    STDMETHOD(FindIndexUsingValues)	    (THIS_ IHXValues*, 
						    REF(UINT32) unIndex) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingValues
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    
     */

    STDMETHOD(FindPluginUsingValues)	    (THIS_ IHXValues*, 
						    REF(IUnknown*) pUnk) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindIndexUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. An index
     *	    is returned which can be used to either get the values (using 
     *	    GetPluginInfo) or an instance can be created using GetPluing(). 	
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindIndexUsingStrings)	    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(UINT32) unIndex) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindPluginUsingStrings
     *
     *	Purpose:    
     *	    Finds a plugin  which matches the set of values given. A Plugin
     *	    instance is returned. 
     *	    NOTE: that a max of two values may be given.
     */

    STDMETHOD(FindPluginUsingStrings)	    (THIS_ char* PropName1, 
						    char* PropVal1, 
						    char* PropName2, 
						    char* PropVal2, 
						    char* PropName3, 
						    char* PropVal3, 
						    REF(IUnknown*) pUnk) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::FindImplementationFromClassID
     *
     *	Purpose:    
     *	    Finds a CommonClassFactory plugin which supports the 
     *	    ClassID given. An instance of the Class is returned. 
     */

    STDMETHOD(FindImplementationFromClassID)
    (
	THIS_ 
	REFGUID GUIDClassID, 
	REF(IUnknown*) pIUnknownInstance
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::Close
     *
     *	Purpose:    
     *	    A function which performs all of the functions of delete.
     *	    
     *
     */
    
    STDMETHOD(Close)		(THIS) PURE; 

    /************************************************************************
     *	Method:
     *	    IHXPlugin2Handler::SetRequiredPlugins
     *
     *	Purpose:    
     *	    This function sets the required plugin list
     *	    
     *
     */

    STDMETHOD(SetRequiredPlugins) (THIS_ const char** ppszRequiredPlugins) PURE;


};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginHandler3
 * 
 *  Purpose:
 * 
 *	Extensions to the IHXPlugin2Handler so we can interact with the 
 *	Gemini Object Broker
 * 
 *  IID_IHXPluginHandler3:
 * 
 * 	{32B19771-2299-11d4-9503-00902790299C}
 * 
 */
DEFINE_GUID( IID_IHXPluginHandler3, 0x32b19771, 0x2299, 0x11d4, 0x95, 0x3, 0x0, 0x90, 0x27, 0x90, 0x29, 0x9c);

#undef  INTERFACE
#define INTERFACE   IHXPluginHandler3

DECLARE_INTERFACE_(IHXPluginHandler3, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RegisterContext
     *
     *	Purpose:    
     *	    Sets up the context without loading any plugin info
     *
     */
    STDMETHOD( RegisterContext )( THIS_ IUnknown* pContext ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::AddPluginMountPoint
     *
     *	Purpose:    
     *	    Sets up the plugins stored in this preferences object
     *
     */
    STDMETHOD( AddPluginMountPoint )( THIS_ const char* pName, UINT32 majorVersion,
					    UINT32 minorVersion, IHXBuffer* pPath ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RefreshPluginMountPoint
     *
     *	Purpose:    
     * 	    Refreshes plugin information associated with this 
     *	    preferences object
     */
    STDMETHOD( RefreshPluginMountPoint )( THIS_ const char* pName ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::RemovePluginMountPoint
     *
     *	Purpose:    
     *	    Removes plugins associated with this preferences object
     */
    STDMETHOD( RemovePluginMountPoint )( THIS_ const char* pName ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindImplementationFromClassID
     *
     *	Purpose:    
     *	    Finds a CommonClassFactory plugin which supports the 
     *	    ClassID given. An instance of the Class is returned. 
     *	    The plugin instance is initialized with the specified 
     *	    context
     */

    STDMETHOD( FindImplementationFromClassID )( THIS_ REFGUID GUIDClassID, 
		    REF(IUnknown*) pIUnknownInstance, IUnknown* pIUnkOuter, IUnknown* pContext ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindCLSIDFromName
     *
     *	Purpose:    
     *
     *	    Maps a text name to a CLSID based on information from 
     *	    component plugins
     */
    STDMETHOD( FindCLSIDFromName )( THIS_ const char* pName, REF(IHXBuffer*) pCLSID ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindGroupOfPluginsUsingValues
     *
     *	Purpose:    
     *	    Builds a collection of plugins that match the criteria
     *
     */
    STDMETHOD(FindGroupOfPluginsUsingValues)(THIS_ IHXValues* pValues, 
				    REF(IHXPluginSearchEnumerator*) pIEnumerator) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindGroupOfPluginsUsingStrings
     *
     *	Purpose:    
     *	    Builds a collection of plugins that match the criteria
     *
     */
    STDMETHOD(FindGroupOfPluginsUsingStrings)(THIS_ char* PropName1, 
				    char* PropVal1, 
				    char* PropName2, 
				    char* PropVal2, 
				    char* PropName3, 
				    char* PropVal3, 
				    REF(IHXPluginSearchEnumerator*) pIEnumerator) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::GetPlugin
     *
     *	Purpose:    
     *	    Allocates a plugin based on index.  Supports aggregation
     *
     */
    STDMETHOD(GetPlugin)(THIS_ ULONG32 ulIndex,
				    REF(IUnknown*) pIUnkResult,
				    IUnknown* pIUnkOuter ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindPluginUsingValues
     *
     *	Purpose:    
     *	    Allocates a plugin based on criteria.  Supports aggregation
     *
     */
    STDMETHOD(FindPluginUsingValues)(THIS_ IHXValues*, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter ) PURE;


    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::FindPluginUsingStrings
     *
     *	Purpose:    
     *	    Allocates a plugin based on criteria.  Supports aggregation
     *
     */
    STDMETHOD(FindPluginUsingStrings)(THIS_ char* PropName1, 
				    char* PropVal1, 
				    char* PropName2, 
				    char* PropVal2, 
				    char* PropName3, 
				    char* PropVal3, 
				    REF(IUnknown*) pIUnkResult, 
				    IUnknown* pIUnkOuter ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::UnloadPluginFromClassID
     *
     *	Purpose:    
     *	    Finds a plugin from the classID and unloads it if it supports CanUnload2
	 *		and returns TRUE in response to query
     */

    STDMETHOD( UnloadPluginFromClassID )( THIS_ REFGUID GUIDClassID ) PURE;

     /************************************************************************
     *	Method:
     *	    IHXPluginHandler3::UnloadPackageByName
     *
     *	Purpose:    
     *	    finds a package from the name passed in and attempts to unload it.
     */
    STDMETHOD (UnloadPackageByName) (char const* pName) PURE;

};
// $EndPrivate.

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPluginDatabase
 * 
 *  Purpose:
 * 
 *	Extensions to the plugin handler for optimized searching
 *	on a single key
 * 
 *  IID_IHXPluginDatabase:
 * 
 *	{C2C65401-A478-11d4-9518-00902790299C}
 * 
 */

enum EPluginIndexType
{
    kIndex_StringType,
    kIndex_BufferType,
    kIndex_GUIDType,
    kIndex_MVStringType,

    kIndex_NumTypes
};

DEFINE_GUID( IID_IHXPluginDatabase, 0xc2c65401, 0xa478, 0x11d4, 0x95, 0x18, 0x0, 0x90, 0x27, 0x90, 0x29, 0x9c);

#undef  INTERFACE
#define INTERFACE   IHXPluginDatabase

DECLARE_INTERFACE_(IHXPluginDatabase, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXPluginDatabase::AddPluginIndex
     *
     *	Purpose:    
     *	    Create a new index in the plugin-handler
     *
     */
    STDMETHOD( AddPluginIndex ) ( THIS_ const char* pKeyName, EPluginIndexType indexType, HXBOOL bScanExisting ) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPluginDatabase::RemovePluginIndex
     *
     *	Purpose:    
     *	    Remove an index from the plugin handler
     *
     */
    STDMETHOD( RemovePluginIndex )( THIS_ const char* pKeyName ) PURE;

    
    /************************************************************************
     *	Method:
     *	    IHXPluginDatabase::FindPluginInfoViaIndex
     *
     *	Purpose:    
     *	    Look up a plugin's info based on a single attribute.  Use an index if
     *	    possible, otherwise defer to a linear search
     *
     */
    STDMETHOD( FindPluginInfoViaIndex )( THIS_ const char* pKeyName, const void* pValue, IHXValues** ppIInfo ) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPluginDatabase::FindPluginSetViaIndex
     *
     *	Purpose:    
     *	    // XXXND  Should this take pValue?  Should it just return a list from an index?
     *	    Find a set of plugins matching a single attribute
     *
     */
    STDMETHOD( FindPluginSetViaIndex )( THIS_ const char* pKeyName, const void* pValue, IHXPluginSearchEnumerator** ppIEnumerator ) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXPluginDatabase::FindPluginViaIndex
     *
     *	Purpose:    
     *	    Create a plugin based on a simple attribute.
     *
     */
    STDMETHOD( CreatePluginViaIndex )( THIS_ const char* pKeyName, const void* pValue, IUnknown** ppIUnkPlugin, IUnknown* pIUnkOuter ) PURE;
};

// $EndPrivate.



// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXProxiedPugin
 * 
 *  Purpose:
 * 
 *	Provides The IHXPlugin actually being used.
 * 
 *  IID_IHXProxiedPlugin:
 * 
 *	{00000C10-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXProxiedPlugin, 0x00000C0A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXProxiedPlugin

DECLARE_INTERFACE_(IHXProxiedPlugin, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXProxiedPlugin methods
     */
    
    /************************************************************************
     *	Method:
     *	    IHXProxiedPlugin::GetProxiedPlugin
     *	Purpose:
     *	    Gets the Actual Plugin being used...
     */
    STDMETHOD(GetProxiedPlugin)(THIS_
	    REF(IHXPlugin*) /*OUT*/ pPlugin) PURE;

};
// $EndPrivate.


// $Private:
/****************************************************************************
 * 
 * Component plugin property names
 *
 *   These attribute names are standard.  
 *
 *	ComponentCLSID maps to an IHXBuffer value that contains the
 *	binary CLSID for the component
 *
 *	ComponentName maps to a string value that contains the tag or
 *	actor name for the component
 *
 */
#define PLUGIN_COMPONENT_CLSID	    "ComponentCLSID"
#define PLUGIN_COMPONENT_NAME	    "ComponentName"


/****************************************************************************
 * 
 *  Interface: 
 * 
 *	IHXComponentPlugin
 * 
 *  Purpose:
 * 
 * 	Allows the plugin handler to iterator over multiple plugins in a DLL
 * 
 *	{F8A31571-22AC-11d4-9503-00902790299C}
 * 
 */
DEFINE_GUID( IID_IHXComponentPlugin, 0xf8a31571, 0x22ac, 0x11d4, 0x95, 0x3, 0x0, 0x90, 0x27, 0x90, 0x29, 0x9c);

#undef  INTERFACE
#define INTERFACE   IHXComponentPlugin

DECLARE_INTERFACE_(IHXComponentPlugin, IUnknown)
{
    /*
     *	IHXComponentPlugin methods
     */

    /************************************************************************
     *	Method:
     *	    IHXComponentPlugin::GetNumberComponents
     *	Purpose:
     */
    STDMETHOD_(UINT32, GetNumComponents)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXComponentPlugin::GetPackageName
     *	Purpose:
     */
    STDMETHOD_(char const*, GetPackageName)(THIS) CONSTMETHOD PURE;

    /************************************************************************
     *  Method:
     *	    IHXComponentPlugin::GetComponentInfoAtIndex
     *  Purpose:
     */
    STDMETHOD(GetComponentInfoAtIndex) (THIS_
				    UINT32 	    /*IN*/  nIndex,
				    REF(IHXValues*)  /*OUT*/ pInfo) PURE;

    /************************************************************************
     *  Method:
     *	    IHXComponentPlugin::CreateComponentInstance
     *  Purpose:
     */
    STDMETHOD(CreateComponentInstance)(THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter) PURE;
};

/****************************************************************************
 * 
 *  Interface: 
 * 
 *	IHXPluginNamespace
 * 
 *  Purpose:
 * 
 * 	Allows the plugin handler to retrieve a plugin's namespace
 * 
 *	{F09E8891-8E2D-11d4-82DB-00D0B74C2D25}
 * 
 */
DEFINE_GUID(IID_IHXPluginNamespace, 0xf09e8891, 0x8e2d, 0x11d4, 0x82, 0xdb, 0x0, 0xd0, 0xb7, 0x4c, 0x2d, 0x25);

#undef  INTERFACE
#define INTERFACE   IHXPluginNamespace

DECLARE_INTERFACE_(IHXPluginNamespace, IUnknown)
{
    /************************************************************************
     *  Method:
     *	    IHXPluginNamespace::GetPluginNamespace
     *  Purpose:
     */
    STDMETHOD(GetPluginNamespace)	(THIS_
					REF(IHXBuffer*)  /*OUT*/ pBuffer) PURE;

};
// $EndPrivate.

#endif /* _HXPLUGN_H_ */
