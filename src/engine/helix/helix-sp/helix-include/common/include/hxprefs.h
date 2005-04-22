
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

/****************************************************************************
 * 
 *
 *  Persistent Preferences Interfaces
 *
 *  Here are the preference entries set by the client core and renderers:
 *	    KEY				DEFAULT VALUES
 *	=================	    ====================
 *	AttemptMulticast		    1
 *	AttemptTCP			    1
 *	AttemptUDP			    1
 *	AudioQuality			    0
 *	AutoTransport			    1
 *	Bandwidth			    28800
 *	BitsPerSample			    16
 *	BroadcastPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple;type}{ ... }
 *	ClientLicenseKey		    7FF7FF00
 *	EndScan				    10000
 *	FactoryPluginInfo		    
 *	FileFormatPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2;extension1|extension2}{ ... }
 *	FileSystemPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple;protocol;shortname}{ ... }
 *	GeneralPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *	PNAProxyHost
 *	PNAProxyPort			    1090
 *	RTSPProxyHost
 *	RTSPProxyPort			    554
 *	HTTPProxyHost
 *	HTTPProxyPort			    1092
 *	HurledURL			    0
 *	InfoandVolume			    1
 *	LastURL				    
 *	MaxClipCount			    4
 *	MetaFormatPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2;extension1|extension2}{ ... }
 *	MiscPluginInfo			    {dllpath;description;copyright;moreinfo;loadmultiple}{ ... }
 *	MulticastTimeout		    2000
 *	NotProxy	    
 *	OnTop				    0
 *	PerfectPlayMode			    0
 *	PerfectPlayTime			    60
 *	PerfPlayEntireClip		    1
 *	PluginDirectory
 *	Presets#
 *	ProxySupport			    0
 *	RendererPluginInfo		    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype1|mimetype2}{ ... }
 *	SamplingRate			    8000
 *	SeekPage			    40
 *	SendStatistics			    1
 *	ServerTimeOut			    90
 *	ShowPresets			    0
 *	StatusBar			    1
 *	StreamDescriptionPluginInfo	    {dllpath;description;copyright;moreinfo;loadmultiple;mimetype}{ ... }
 *	SyncMultimedia			    1
 *	UDPPort				    7070
 *	UDPTimeout			    10000
 *	UpgradeAvailable		    0
 *	UseUDPPort			    0
 *	Volume				    50
 *	x:Pref_windowPositionX
 *	y:Pref_WindowPositionY
 */

#ifndef _HXPREFS_H_
#define _HXPREFS_H_

#define HXREGISTRY_PREFPROPNAME	    "ApplicationData"
/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXBuffer			IHXBuffer;


// CLSID for creating a preferences objects via a CCF
// {EC5C2B01-D105-11d4-951F-00902790299C}
#define CLSID_HXPreferences IID_IHXPreferences

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPreferences
 * 
 *  Purpose:
 * 
 *	This interface allows you to store persistant preferences in the
 *	server or player's config / registry.
 * 
 *  IID_IHXPreferences:
 * 
 *	{00000500-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPreferences, 0x00000500, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPreferences

DECLARE_INTERFACE_(IHXPreferences, IUnknown)
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
     *	IHXPreferences methods
     */

    /************************************************************************
     *	Method:
     *		IHXPreferences::ReadPref
     *	Purpose:
     *		Read a preference from the registry or configuration.
     */
    STDMETHOD(ReadPref)			(THIS_
					const char* pPrekKey, REF(IHXBuffer*) pBuffer) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferences::WritePref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(WritePref)		(THIS_
					const char* pPrekKey, IHXBuffer* pBuffer) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPreferenceEnumerator
 * 
 *  Purpose:
 * 
 *	Allows preference Enumeration
 *	
 * 
 *  IHXPreferenceEnumerator:
 * 
 *	{00000504-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPreferenceEnumerator, 0x00000504, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPreferenceEnumerator

DECLARE_INTERFACE_(IHXPreferenceEnumerator, IUnknown)
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
     *	IHXPreferenceEnumerator methods
     */

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::EndSubPref
     *	Purpose:
     *		TBD
     */

    STDMETHOD(BeginSubPref) (THIS_ const char* szSubPref) PURE;


    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::EndSubPref
     *	Purpose:
     *		TBD
     */

   STDMETHOD(EndSubPref) (THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::GetPrefKey
     *	Purpose:
     *		TBD
     */

   STDMETHOD(GetPrefKey) (THIS_ UINT32 nIndex, REF(IHXBuffer*) pBuffer) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::ReadPref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(ReadPref)			(THIS_
					    const char* pPrefKey, IHXBuffer*& pBuffer) PURE;

};



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPreferences2
 * 
 *  Purpose:
 * 
 *	New interface which gives sub-preference options abilities.
 *	
 * 
 *  IID_IHXPreferences2:
 * 
 *	{00000503-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPreferences2, 0x00000503, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPreferences2

DECLARE_INTERFACE_(IHXPreferences2, IUnknown)
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
     *	IHXPreferences2 methods
     */

    /************************************************************************
     *	Method:
     *		IHXPreferences2::GetPreferenceEnumerator
     *	Purpose:
     *		Read a preference from the registry or configuration.
     */

    STDMETHOD(GetPreferenceEnumerator)(THIS_ REF(IHXPreferenceEnumerator*) /*OUT*/ pEnum) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferences2::ResetRoot
     *	Purpose:
     *		Reset the root of the preferences
     */

    STDMETHOD(ResetRoot)(THIS_ const char* pCompanyName, const char* pProductName, 
	int nProdMajorVer, int nProdMinorVer) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPreferences3
 * 
 *  Purpose:
 * 
 *	New interface for deleting preferences, and whatever we might think of next!
 *	
 * 
 *  IID_IHXPreferences3:
 * 
 *	{00000505-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPreferences3, 0x00000505, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPreferences3

DECLARE_INTERFACE_(IHXPreferences3, IUnknown)
{
    /************************************************************************
     *	Method:
     *		IHXPreferences3::Open
     *	Purpose:
     *		Open a specified collection of preferences
     */

    STDMETHOD( Open )(THIS_ const char* pCompanyName, const char* pProductName, 
			    ULONG32 nProdMajorVer, ULONG32 nProdMinorVer) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferences3::OpenShared
     *	Purpose:
     *		Have this preference object read/write from the company wide 
     *   	shared location for all products
     */
    STDMETHOD( OpenShared )( THIS_ const char* pCompanyName) PURE;

    /************************************************************************
     *	Method:
     *		IHXPreferences3::DeletePref
     *	Purpose:
     *		Delete a preference
     */

    STDMETHOD(DeletePref)( THIS_ const char* pPrekKey ) PURE;

};
// $EndPrivate.

#endif /* _HXPREFS_H_ */
