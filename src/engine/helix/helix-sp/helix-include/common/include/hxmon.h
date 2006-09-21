
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

#ifndef _HXMON_H_
#define _HXMON_H_

#include "hlxclib/limits.h"

typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXPlugin			IHXPlugin;
typedef _INTERFACE	IHXBuffer			IHXBuffer;
typedef _INTERFACE	IHXValues			IHXValues;
typedef _INTERFACE	IHXPropWatch			IHXPropWatch;
typedef _INTERFACE	IHXPropWatchResponse		IHXPropWatchResponse;
typedef _INTERFACE	IHXActiveRegistry		IHXActiveRegistry;
typedef _INTERFACE	IHXActivePropUser		IHXActivePropUser;
typedef _INTERFACE	IHXActivePropUserResponse	IHXActivePropUserResponse;
typedef _INTERFACE	IHXRegistryAltStringHandling	IHXRegistryAltStringHandling;

/*
 * Types of the values stored in the registry.
 */
typedef enum _HXPropType
{
    PT_UNKNOWN,
    PT_COMPOSITE,	/* Contains other values (elements)		     */
    PT_INTEGER,		/* 32-bit signed value				     */
    PT_INTREF,		/* Integer reference object -- 32-bit signed integer */
    PT_STRING,		/* Signed char* value				     */
    PT_BUFFER,		/* IHXBuffer object				     */

    /*IHXRegistry2: */
    PT_INTEGER64,	/* 64-bit signed value				     */
    PT_INT64REF 	/* Integer reference object -- 64-bit signed integer */

} HXPropType;


/*
 * 
 *  Interface:
 *
 *	IHXRegistry
 *
 *  Purpose:
 *
 *	This interface provides access to the "Registry" in the server and
 *	client.  The "Registry" is a hierarchical structure of Name/Value
 *	pairs (properties) which is capable of storing many different types
 *	of data including strings, buffers, and integers.  The registry
 *	provides various types of information including statistics,
 *	configuration information, and system status.
 *
 *	Note:  This registry is not related to the Windows system registry.
 *
 *  IID_IHXRegistry:
 *
 *	{00000600-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRegistry, 0x00000600, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXRegistry	IID_IHXRegistry

#undef  INTERFACE
#define INTERFACE   IHXRegistry

DECLARE_INTERFACE_(IHXRegistry, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXRegistry methods
     */

    /************************************************************************
     *  Method:
     *      IHXRegistry::CreatePropWatch
     *  Purpose:
     *      Create a new IHXPropWatch object which can then be queried for 
     *  the right kind of IHXPropWatch object.
     *
     *  pPropWatch - OUT - returns a new addref'ed IHXPropWatch object 
     */
    STDMETHOD(CreatePropWatch)		(THIS_
					REF(IHXPropWatch*) pPropWatch) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddComp
     *  Purpose:
     *      Add a COMPOSITE property to the registry and return its ID
     *  if successful. It returns ZERO (0) if an error occurred
     *  during the operation.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     */
    STDMETHOD_(UINT32, AddComp)		(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddInt
     *  Purpose:
     *      Add an INTEGER property with name in "pName" and value in 
     *  "iValue" to the registry. The return value is the id to
     *  the newly added Property or ZERO if there was an error.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  nValue - IN - integer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddInt)		(THIS_
					const char*	pName, 
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetIntByName
     *  Purpose:
     *      Retreive an INTEGER value from the registry given its Property
     *  name "pName". If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetIntByName)		(THIS_
					const char*	pName,
					REF(INT32)	nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetIntById
     *  Purpose:
     *      Retreive an INTEGER value from the registry given its id "ulId". 
     *  If the Property is found, it will return HXR_OK, otherwise it 
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetIntById)		(THIS_
					const UINT32	ulId,
					REF(INT32)	nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetIntByName
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName". If the value was set, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetIntByName)		(THIS_
					const char*	pName, 
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetIntById
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  its id "id". If the value was set, it will return HXR_OK, otherwise 
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetIntById)		(THIS_
					const UINT32	id,
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddStr
     *  Purpose:
     *      Add an STRING property with name in "pName" and value in 
     *  "pValue" to the registry.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - buffer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddStr)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetStrByName
     *  Purpose:
     *      Retreive an STRING value from the registry given its Property
     *  name "pName". If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetStrByName)		(THIS_
					const char*	 pName,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetStrById
     *  Purpose:
     *      Retreive an STRING value from the registry given its id "ulId". 
     *  If the Property is found, it will return HXR_OK, otherwise it 
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetStrById)		(THIS_
					const UINT32	 ulId,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetStrByName
     *  Purpose:
     *      Modify a Property's STRING value in the registry given the
     *  Property's name "pName". If the value was set, it will return 
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetStrByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetStrById
     *  Purpose:
     *      Modify a Property's STRING value in the registry given the
     *  its id "ulId". If the value was set, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetStrById)		(THIS_
					const UINT32	ulId,
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddBuf
     *  Purpose:
     *      Add an BUFFER property with name in "pName" and value in 
     *  "pValue" to the registry.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - buffer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddBuf)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetBufByName
     *  Purpose:
     *      Retreive the BUFFER from the registry given its Property name 
     *  "pName". If the Property is found, it will return HXR_OK, otherwise 
     *  it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetBufByName)		(THIS_
					const char*	 pName,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetBufById
     *  Purpose:
     *      Retreive the BUFFER from the registry given its id "ulId". If the 
     *  Property is found, it will return HXR_OK, otherwise it returns 
     *  HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetBufById)		(THIS_
					const UINT32	 ulId,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetBufByName
     *  Purpose:
     *      Modify a Property's BUFFER in the registry given the
     *  Property's name "pName". If the value was set, it will return 
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetBufByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetBufById
     *  Purpose:
     *      Modify a Property's BUFFER in the registry given its id "ulId". 
     *  If the value was set, it will return HXR_OK, otherwise it returns 
     *  HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetBufById)		(THIS_
					const UINT32	ulId,
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddIntRef
     *  Purpose:
     *      Add an INTEGER REFERENCE property with name in "pName" and 
     *  value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - the pointer of the integer value is what gets stored
     *                in the registry as the Interger Reference Property's 
     *                value
     */
    STDMETHOD_(UINT32, AddIntRef)	(THIS_
					const char*	pName, 
					INT32*		pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::DeleteByName
     *  Purpose:
     *      Delete a Property from the registry using its name "pName".
     *
     *  pName - IN - name of the Property that is going to be deleted
     */
    STDMETHOD_(UINT32, DeleteByName)	(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::DeleteById
     *  Purpose:
     *      Delete a Property from the registry using its id "ulId".
     *
     *  ulId - IN - unique id of the Property that is going to be deleted
     */
    STDMETHOD_(UINT32, DeleteById)	(THIS_
					const UINT32	ulId) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetTypeByName
     *  Purpose:
     *      Returns the datatype of the Property given its name "pName".
     *
     *  pName - IN - name of the Property whose type is to be retrieved
     */
    STDMETHOD_(HXPropType, GetTypeByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetTypeById
     *  Purpose:
     *      Returns the datatype of the Property given its its id "ulId".
     *
     *  ulId - IN - unique id of the Property whose type is to be retrieved
     */
    STDMETHOD_(HXPropType, GetTypeById)	(THIS_
						const UINT32 ulId) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::FindParentIdByName
     *  Purpose:
     *      Returns the id value of the parent node of the Property whose 
     *  name "pName" has been passed in. If it fails, a ZERO value is 
     *  returned.
     *
     *  pName - IN - name of the Property whose parent's unique id is to be
     *               retrieved
     */
    STDMETHOD_(UINT32, FindParentIdByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::FindParentIdById
     *  Purpose:
     *      Returns the id value of the parent node of the Property whose 
     *  id "ulId" has been passed in. If it fails, a ZERO value is returned.
     *
     *  ulId - IN - unique id of the Property whose parent's id is to be
     *              retrieved
     */
    STDMETHOD_(UINT32, FindParentIdById)	(THIS_
						const UINT32 ulId) const PURE;

    /************************************************************************
     *  Method:
     *      HXRegistry::GetPropName
     *  Purpose:
     *      Returns the Property name in the pName char buffer passed
     *  as a parameter, given the Property's id "ulId".
     *
     *  ulId - IN - unique id of the Property whose name is to be retrieved
     *  pName - OUT - parameter into which the Property name is going to be
     *                returned
     */
    STDMETHOD(GetPropName)		(THIS_
					const UINT32 ulId,
					REF(IHXBuffer*) pName) const PURE;

    /************************************************************************
     *  Method:
     *      HXRegistry::GetId
     *  Purpose:
     *      Returns the Property's id given the Property name.
     *
     *  pName - IN - name of the Property whose unique id is to be 
     *               retrieved
     */
    STDMETHOD_(UINT32, GetId)		(THIS_
					const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListOfRoot
     *  Purpose:
     *      Returns an array of a Properties under the root level of the 
     *  registry's hierarchy.
     *
     *  pValues - OUT - list of property name and unique id at the 
     *                  highest level (root) in the registry
     */
    STDMETHOD(GetPropListOfRoot) 	(THIS_
					REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListByName
     *  Purpose:
     *      Returns an array of Properties immediately under the one whose
     *  name is passed in "pName".
     *
     *  pName - IN - name of the Property whose child property list is to be
     *               retrieved
     *  pValues - OUT - list of property name and unique id under the 
     *                  Property whose name is in "pName"
     */
    STDMETHOD(GetPropListByName) 	(THIS_
					 const char* pName,
					 REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListById
     *  Purpose:
     *      Returns an array of Properties immediately under the one whose
     *  id is passed in "ulId".
     *
     *  ulId - IN - unique id of the Property whose child property list is 
     *              to be retrieved
     *  pValues - OUT - list of property name and unique id under the 
     *                  Property whose is is in "ulId"
     */
    STDMETHOD(GetPropListById) 	 	(THIS_
					 const UINT32 ulId,
					 REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetNumPropsAtRoot
     *  Purpose:
     *      Returns the number of Properties at the root of the registry. 
     */
    STDMETHOD_(INT32, GetNumPropsAtRoot)	(THIS) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetNumPropsByName
     *  Purpose:
     *      Returns the count of the number of Properties under the one 
     *  whose name is specified in "pName".
     *
     *  pName - IN - name of the Property whose number of children is to be
     *               retrieved
     */
    STDMETHOD_(INT32, GetNumPropsByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetNumPropsById
     *  Purpose:
     *      Returns the count of the number of Properties under the one 
     *  whose unique id is specified in "ulId".
     *
     *  ulId - IN - unique id of the Property whose number of children is 
     *              to be retrieved
     */
    STDMETHOD_(INT32, GetNumPropsById)		(THIS_
						const UINT32 ulId) const PURE;
};


/*
 * 
 *  Interface:
 *
 *	IHXPropWatch
 *
 *  Purpose:
 *
 *      This interface allows the user to watch properties so that when
 *  changes happen to the properties the plugins receive notification via
 *  the IHXPropWatchResponse API.
 *
 *  IID_IHXPropWatch:
 *
 *	{00000601-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPropWatch, 0x00000601, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPropWatch

DECLARE_INTERFACE_(IHXPropWatch, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXPropWatch methods
     */

    /************************************************************************
     *  Method:
     *      IHXPropWatch::Init
     *  Purpose:
     *      Initialize with the response object so that the Watch
     *  notifications can be sent back to the respective plugins.
     *
     *  pResponse - IN - pointer to the response object which gets used to
     *                   initialize the IHXPropWatch object. the response
     *                   object gets AddRef'd in the Init method.
     */
    STDMETHOD(Init)		(THIS_
				IHXPropWatchResponse*	pResponse) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::SetWatchOnRoot
     *  Purpose:
     *      The SetWatch method puts a watch at the highest level of
     *  the registry hierarchy. It notifies ONLY IF properties at THIS LEVEL
     *  get added/modified/deleted.
     */
    STDMETHOD_(UINT32, SetWatchOnRoot)	(THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::SetWatchByName
     *  Purpose:
     *      Sets a watch-point on the Property whose name is passed in.
     *  In case the mentioned Property gets modified or deleted a
     *  notification of that will be sent to the object which set the
     *  watch-point.
     *
     *  pName - IN - name of Property on which a watch point is to be added
     */
    STDMETHOD_(UINT32, SetWatchByName)	(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::SetWatchById
     *  Purpose:
     *      Sets a watch-point on the Property whose name is passed in.
     *  In case the mentioned Property gets modified or deleted a
     *  notification of that will be sent to the object which set the
     *  watch-point.
     *
     *  ulId - IN - unique id of Property on which a watch point is to be 
     *              added
     */
    STDMETHOD_(UINT32, SetWatchById)	(THIS_
					const UINT32	ulId) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchOnRoot
     *  Purpose:
     *      It clears the watch on the root of the registry.
     */
    STDMETHOD(ClearWatchOnRoot)		(THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchByName
     *  Purpose:
     *      Clears a watch-point based on the Property's name.
     *
     *  pName - IN - name of Property whose watch point is to be cleared
     */
    STDMETHOD(ClearWatchByName)		(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatch::ClearWatchById
     *  Purpose:
     *      Clears a watch-point based on the Property's id.
     *
     *  ulId - IN - unique id of Property whose watch point is to be cleared
     */
    STDMETHOD(ClearWatchById)		(THIS_
					const UINT32	ulId) PURE;
};


/*
 * 
 *  Interface:
 *
 *	IHXPropWatchResponse
 *
 *  Purpose:
 *
 *	Interface for notification of additions/modifications/deletions of
 *  properties in the registry which are being watched.
 *
 *  IID_IHXPropWatchResponse:
 *
 *	{00000602-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPropWatchResponse, 0x00000602, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPropWatchResponse

DECLARE_INTERFACE_(IHXPropWatchResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPropWatchResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::AddedProp
     *  Purpose:
     *      Gets called when a new Property gets added under the Property 
     *  on which the Watch was set. It passes the id of the Property just 
     *  added, its datatype and the id of its immediate parent COMPOSITE 
     *  property.
     */
    STDMETHOD(AddedProp)	(THIS_
				const UINT32		ulId,
				const HXPropType   	propType,
				const UINT32		ulParentID) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::ModifiedProp
     *  Purpose:
     *      Gets called when a watched Property gets modified. It passes
     *  the id of the Property just modified, its datatype and the
     *  id of its immediate parent COMPOSITE property.
     */
    STDMETHOD(ModifiedProp)	(THIS_
				const UINT32		ulId,
				const HXPropType   	propType,
				const UINT32		ulParentID) PURE;

    /************************************************************************
     *  Method:
     *      IHXPropWatchResponse::DeletedProp
     *  Purpose:
     *      Gets called when a watched Property gets deleted. As can be
     *  seen, it returns the id of the Property just deleted and
     *  its immediate parent COMPOSITE property.
     */
    STDMETHOD(DeletedProp)	(THIS_
				const UINT32		ulId,
				const UINT32		ulParentID) PURE;
};

/*
 * 
 *  Interface:
 *
 *	IHXActiveRegistry
 *
 *  Purpose:
 *
 *	Interface to get IHXActiveUser responsible for a particular property 
 *  from the registry.
 *
 *  IID_IHXActiveRegistry:
 *
 *	{00000603-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXActiveRegistry, 0x00000603, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXActiveRegistry

DECLARE_INTERFACE_(IHXActiveRegistry, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
    * IHXActiveRegistry::SetAsActive
    *
    *     Method to set prop pName to active and register pUser as
    *   the active prop user.
    */
    STDMETHOD(SetAsActive)    (THIS_
				const char* pName,
				IHXActivePropUser* pUser) PURE;

    /************************************************************************
    * IHXActiveRegistry::SetAsInactive
    *
    *	Method to remove an IHXActiveUser from Prop activation.
    */
    STDMETHOD(SetAsInactive)  (THIS_
				const char* pName,
				IHXActivePropUser* pUser) PURE;

    /************************************************************************
    * IHXActiveRegistry::IsActive
    *
    *     Tells if prop pName has an active user that must be queried to
    *   change the value, or if it can just be set.
    */
    STDMETHOD_(HXBOOL, IsActive)	(THIS_
				const char* pName) PURE;

    /************************************************************************
    * IHXActiveRegistry::SetActiveInt
    *
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (THIS_
			    const char* pName,
			    UINT32 ul,
			    IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActiveRegistry::SetActiveStr
    *
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (THIS_
			    const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActiveRegistry::SetActiveBuf
    *
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)	(THIS_
				const char* pName,
				IHXBuffer* pBuffer,
				IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActiveRegistry::DeleteActiveProp
    *
    *	Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp)	(THIS_
				const char* pName,
				IHXActivePropUserResponse* pResponse) PURE;


};


/*
 * 
 *  Interface:
 *
 *	IHXActivePropUser
 *
 *  Purpose:
 *
 *	An IHXActivePropUser can be set as the active user of a property in 
 *  an IHXActiveRegistry. This causes the IHXActivePropUser to be consulted 
 *  every time someone wants to change a property. The difference between this 
 *  and a prop watch is that this is async, and can call a done method with 
 *  failure to cause the prop to not be set, and this get called instead of 
 *  calling into the IHXReg.
 *
 *  IID_IHXActivePropUser:
 *
 *	{00000604-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXActivePropUser, 0x00000604, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXActivePropUser

DECLARE_INTERFACE_(IHXActivePropUser, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
    * IHXActivePropUser::SetActiveInt
    *
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (THIS_
			    const char* pName,
			    UINT32 ul,
			    IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActivePropUser::SetActiveStr
    *
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (THIS_
			    const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActivePropUser::SetActiveBuf
    *
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)	(THIS_
				const char* pName,
				IHXBuffer* pBuffer,
				IHXActivePropUserResponse* pResponse) PURE;

    /************************************************************************
    * IHXActivePropUser::DeleteActiveProp
    *
    *	Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp)	(THIS_
				const char* pName,
				IHXActivePropUserResponse* pResponse) PURE;

};

/*
 * 
 *  Interface:
 *
 *	IHXActivePropUserResponse
 *
 *  Purpose:
 *
 *	Gets responses from IHXActivePropUser for queries to set properties
 *  in the IHXActiveRegistry.
 *
 *
 *  IID_IHXActivePropUserResponse:
 *
 *	{00000605-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXActivePropUserResponse, 0x00000605, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXActivePropUserResponse

DECLARE_INTERFACE_(IHXActivePropUserResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
    * Called with status result on completion of set request.
    */
    STDMETHOD(SetActiveIntDone)   (THIS_
				    HX_RESULT res,
				    const char* pName,
				    UINT32 ul,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo) PURE;

    STDMETHOD(SetActiveStrDone)	  (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pBuffer,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo) PURE;

    STDMETHOD(SetActiveBufDone)	  (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pBuffer,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo) PURE;

    STDMETHOD(DeleteActivePropDone) (THIS_
				    HX_RESULT res,
				    const char* pName,
				    IHXBuffer* pInfo[],
				    UINT32 ulNumInfo) PURE;

};

/*
 * 
 *  Interface:
 *
 *	IHXCopyRegistry
 *
 *  Purpose:
 *
 *	Allows copying from one registry key to another.
 *
 *
 *  IID_IHXCopyRegistry
 *
 *	{00000606-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXCopyRegistry, 0x00000606, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCopyRegistry

DECLARE_INTERFACE_(IHXCopyRegistry, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
    * IHXCopyRegistry::Copy
    *
    *   Here it is! The "Copy" method!
    */
    STDMETHOD (CopyByName)  (THIS_
			    const char* pFrom,
			    const char* pTo) PURE;
};


/*
 * 
 *  Interface:
 *
 *	IHXRegistryAltStringHandling
 *
 *  Purpose:
 *
 *	Tells the registry about alternate handling of PT_STRING types.
 *
 *
 *  IID_IHXRegistryAltStringHandling
 *
 *	{00000607-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRegistryAltStringHandling, 0x00000607, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRegistryAltStringHandling

DECLARE_INTERFACE_(IHXRegistryAltStringHandling, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
    * IHXRegistryAltStringHandling::SetStringAccessAsBufferById
    *
    *   For those times when you added a property as a buffer, but wish it
    *   were a string (and of course, people now rely on the fact that it's
    *   a buffer)...  Create the property as a string and then pass this
    *   method it's ID.  The property will now be accessible/setable as a,
    *   but it will still be a string!
    */
    STDMETHOD (SetStringAccessAsBufferById)  (THIS_
					      UINT32 ulId) PURE;
};



// $Private:
/*
 * 
 *  Interface:
 *
 *      IHXRegistry2
 *
 *  Purpose:
 *
 *      1) Provide atomic update methods
 *      2) Provide INT64 support
 *      3) Provide access to INTREF pointers
 *
 *      All operations occur atomically, ensuring that multiple users of the
 *      registry do not interfere with each other, even on multi-CPU systems.
 *      Note, this is essentially a superset of IHXRegistry.
 *
 *  IID_IHXRegistry2
 *
 *	{00000608-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRegistry2, 0x00000608, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXRegistry2

DECLARE_INTERFACE_(IHXRegistry2, IUnknown)
{

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXRegistry2 methods
     */

    /************************************************************************
     *  Method:
     *      IHXRegistry2::CreatePropWatch
     *  Purpose:
     *      Create a new IHXPropWatch object which can then be queried for 
     *  the right kind of IHXPropWatch object.
     *
     *  pPropWatch - OUT - returns a new addref'ed IHXPropWatch object 
     */
    STDMETHOD(CreatePropWatch)		(THIS_
					REF(IHXPropWatch*) pPropWatch) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddComp
     *  Purpose:
     *      Add a COMPOSITE property to the registry and return its ID
     *  if successful. It returns ZERO (0) if an error occurred
     *  during the operation.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     */
    STDMETHOD_(UINT32, AddComp)		(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddInt
     *  Purpose:
     *      Add an INTEGER property with name in "pName" and value in 
     *  "iValue" to the registry. The return value is the id to
     *  the newly added Property or ZERO if there was an error.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  nValue - IN - integer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddInt)		(THIS_
					const char*	pName, 
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetIntByName
     *  Purpose:
     *      Retreive an INTEGER value from the registry given its Property
     *  name "pName". If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetIntByName)		(THIS_
					const char*	pName,
					REF(INT32)	nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetIntById
     *  Purpose:
     *      Retreive an INTEGER value from the registry given its id "ulId". 
     *  If the Property is found, it will return HXR_OK, otherwise it 
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetIntById)		(THIS_
					const UINT32	ulId,
					REF(INT32)	nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetIntByName
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName". If the value was set, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetIntByName)		(THIS_
					const char*	pName, 
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetIntById
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  its id "id". If the value was set, it will return HXR_OK, otherwise 
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetIntById)		(THIS_
					const UINT32	id,
					const INT32	nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddStr
     *  Purpose:
     *      Add an STRING property with name in "pName" and value in 
     *  "pValue" to the registry and return its ID if successful.
     *  It returns ZERO (0) if an error occurred during the operation.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - buffer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddStr)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetStrByName
     *  Purpose:
     *      Retreive an STRING value from the registry given its Property
     *  name "pName". If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetStrByName)		(THIS_
					const char*	 pName,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetStrById
     *  Purpose:
     *      Retreive an STRING value from the registry given its id "ulId". 
     *  If the Property is found, it will return HXR_OK, otherwise it 
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetStrById)		(THIS_
					const UINT32	 ulId,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetStrByName
     *  Purpose:
     *      Modify a Property's STRING value in the registry given the
     *  Property's name "pName". If the value was set, it will return 
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetStrByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetStrById
     *  Purpose:
     *      Modify a Property's STRING value in the registry given the
     *  its id "ulId". If the value was set, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetStrById)		(THIS_
					const UINT32	ulId,
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddBuf
     *  Purpose:
     *      Add an BUFFER property with name in "pName" and value in 
     *  "pValue" to the registry  and return its ID if successful.
     *  It returns ZERO (0) if an error occurred during the operation.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - buffer value of the Property that is going to be 
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddBuf)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetBufByName
     *  Purpose:
     *      Retreive the BUFFER from the registry given its Property name 
     *  "pName". If the Property is found, it will return HXR_OK, otherwise 
     *  it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetBufByName)		(THIS_
					const char*	 pName,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetBufById
     *  Purpose:
     *      Retreive the BUFFER from the registry given its id "ulId". If the 
     *  Property is found, it will return HXR_OK, otherwise it returns 
     *  HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  pValue - OUT - parameter into which the value of the Property is 
     *                 going to be returned
     */
    STDMETHOD(GetBufById)		(THIS_
					const UINT32	 ulId,
					REF(IHXBuffer*) pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetBufByName
     *  Purpose:
     *      Modify a Property's BUFFER in the registry given the
     *  Property's name "pName". If the value was set, it will return 
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetBufByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetBufById
     *  Purpose:
     *      Modify a Property's BUFFER in the registry given its id "ulId". 
     *  If the value was set, it will return HXR_OK, otherwise it returns 
     *  HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  pValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetBufById)		(THIS_
					const UINT32	ulId,
					IHXBuffer*	pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddIntRef
     *  Purpose:
     *      Add an INTEGER REFERENCE property with name in "pName" and 
     *  value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.  The Property's id is returned if successful.
     *  It returns ZERO (0) if an error occurred during the operation.
     *
     *  pName - IN - name of the Property that is going to be added to 
     *               the registry
     *  pValue - IN - the pointer of the integer value is what gets stored
     *                in the registry as the Interger Reference Property's 
     *                value
     */
    STDMETHOD_(UINT32, AddIntRef)	(THIS_
					const char*	pName, 
					INT32*		pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::DeleteByName
     *  Purpose:
     *      Delete a Property from the registry using its name "pName".
     *
     *  pName - IN - name of the Property that is going to be deleted
     */
    STDMETHOD_(UINT32, DeleteByName)	(THIS_
					const char*	pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::DeleteById
     *  Purpose:
     *      Delete a Property from the registry using its id "ulId".
     *
     *  ulId - IN - unique id of the Property that is going to be deleted
     */
    STDMETHOD_(UINT32, DeleteById)	(THIS_
					const UINT32	ulId) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetTypeByName
     *  Purpose:
     *      Returns the datatype of the Property given its name "pName".
     *
     *  pName - IN - name of the Property whose type is to be retrieved
     */
    STDMETHOD_(HXPropType, GetTypeByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetTypeById
     *  Purpose:
     *      Returns the datatype of the Property given its its id "ulId".
     *
     *  ulId - IN - unique id of the Property whose type is to be retrieved
     */
    STDMETHOD_(HXPropType, GetTypeById)	(THIS_
						const UINT32 ulId) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::FindParentIdByName
     *  Purpose:
     *      Returns the id value of the parent node of the Property whose 
     *  name "pName" has been passed in. If it fails, a ZERO value is 
     *  returned.
     *
     *  pName - IN - name of the Property whose parent's unique id is to be
     *               retrieved
     */
    STDMETHOD_(UINT32, FindParentIdByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::FindParentIdById
     *  Purpose:
     *      Returns the id value of the parent node of the Property whose 
     *  id "ulId" has been passed in. If it fails, a ZERO value is returned.
     *
     *  ulId - IN - unique id of the Property whose parent's id is to be
     *              retrieved
     */
    STDMETHOD_(UINT32, FindParentIdById)	(THIS_
						const UINT32 ulId) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropName
     *  Purpose:
     *      Returns the Property name in the pName char buffer passed
     *  as a parameter, given the Property's id "ulId".
     *
     *  ulId - IN - unique id of the Property whose name is to be retrieved
     *  pName - OUT - parameter into which the Property name is going to be
     *                returned
     */
    STDMETHOD(GetPropName)		(THIS_
					const UINT32 ulId,
					REF(IHXBuffer*) pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetId
     *  Purpose:
     *      Returns the Property's id given the Property name.
     *
     *  pName - IN - name of the Property whose unique id is to be 
     *               retrieved
     */
    STDMETHOD_(UINT32, GetId)		(THIS_
					const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropListOfRoot
     *  Purpose:
     *      Returns an array of a Properties under the root level of the 
     *  registry's hierarchy.
     *
     *  pValues - OUT - list of property name and unique id at the 
     *                  highest level (root) in the registry
     */
    STDMETHOD(GetPropListOfRoot) 	(THIS_
					REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropListByName
     *  Purpose:
     *      Returns an array of Properties immediately under the one whose
     *  name is passed in "pName".
     *
     *  pName - IN - name of the Property whose child property list is to be
     *               retrieved
     *  pValues - OUT - list of property name and unique id under the 
     *                  Property whose name is in "pName"
     */
    STDMETHOD(GetPropListByName) 	(THIS_
					 const char* pName,
					 REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropListById
     *  Purpose:
     *      Returns an array of Properties immediately under the one whose
     *  id is passed in "ulId".
     *
     *  ulId - IN - unique id of the Property whose child property list is 
     *              to be retrieved
     *  pValues - OUT - list of property name and unique id under the 
     *                  Property whose is is in "ulId"
     */
    STDMETHOD(GetPropListById) 	 	(THIS_
					 const UINT32 ulId,
					 REF(IHXValues*) pValues) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetNumPropsAtRoot
     *  Purpose:
     *      Returns the number of Properties at the root of the registry. 
     *
     */
    STDMETHOD_(INT32, GetNumPropsAtRoot)	(THIS) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetNumPropsByName
     *  Purpose:
     *      Returns the count of the number of Properties under the one 
     *  whose name is specified in "pName".
     *
     *  pName - IN - name of the Property whose number of children is to be
     *               retrieved
     */
    STDMETHOD_(INT32, GetNumPropsByName)	(THIS_
						const char* pName) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetNumPropsById
     *  Purpose:
     *      Returns the count of the number of Properties under the one 
     *  whose unique id is specified in "ulId".
     *
     *  ulId - IN - unique id of the Property whose number of children is 
     *              to be retrieved
     */
    STDMETHOD_(INT32, GetNumPropsById)		(THIS_
						const UINT32 ulId) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::ModifyIntByName
     *  Purpose:
     *      Changes the INTEGER value in the registry given its Property
     *  name "pName" and an amount to change it by.  Modifies the value
     *  of the integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification.  If the
     *  Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyIntByName)         (THIS_
                                        const char*     pName,
					INT32           nDelta,
                                        REF(INT32)      nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::ModifyIntById
     *  Purpose:
     *      Changes the INTEGER value in the registry given its id "ulID"
     *  and an amount to change it by.  Modifies the value of the
     *  integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification.  If the
     *  Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyIntById)           (THIS_
                                        const UINT32    id,
					INT32           nDelta,
                                        REF(INT32)      nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::BoundedModifyIntByName
     *  Purpose:
     *      Changes the INTEGER value in the registry given its Property name
     *  "pName" and an amount to change it by and keeps the modified value 
     *  within the bounds of the nMin and nMax values. Modifies the value 
     *  of the integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification, if the modified 
     *  value is >= nMin and <= nMax. If either of these limits are violated 
     *  the the resulting value stored in the registry is the value of the 
     *  limit just violated. If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nMin - IN - min value that the modified registry prop can have
     *              if the modified registry value < nMin then
     *                  registry value = nMin
     *  nMax - IN - min value that the modified registry prop can have
     *              if the modified registry value > nMax then
     *                  registry value = nMax
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(BoundedModifyIntByName)   (THIS_
                                        const char*     pName,
					INT32           nDelta,
                                        REF(INT32)      nValue,
					INT32           nMin=INT_MIN,
					INT32           nMax=INT_MAX) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::BoundedModifyIntById
     *  Purpose:
     *      Changes the INTEGER value in the registry given its id "ulID"
     *  and an amount to change it by and keeps the modified value within
     *  the bounds of the nMin and nMax values. Modifies the value of the
     *  integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification, if the modified
     *  value is >= nMin and <= nMax. If either of these limits are violated
     *  the the resulting value stored in the registry is the value of the
     *  limit just violated. If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nMin - IN - min value that the modified registry prop can have
     *              if the modified registry value < nMin then
     *                  registry value = nMin
     *  nMax - IN - min value that the modified registry prop can have
     *              if the modified registry value > nMax then
     *                  registry value = nMax
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(BoundedModifyIntById)     (THIS_
                                        const UINT32    id,
					INT32           nDelta,
                                        REF(INT32)      nValue,
					INT32           nMin=INT_MIN,
					INT32           nMax=INT_MAX) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetAndReturnIntByName
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName". If the Property is found, the previous
     *  value, prior to setting it, will be assigned to nOldValue.
     *  If the Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnIntByName)   (THIS_
                                        const char*     pName,
					INT32           nValue,
                                        REF(INT32)      nOldValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetAndReturnIntById
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's id "ulId". If the id is found, the previous
     *  value, prior to setting it, will be assigned to nOldValue.
     *  If the Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnIntById)     (THIS_
                                        const UINT32    id,
					INT32           nValue,
                                        REF(INT32)      nOldValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetIntRefByName
     *  Purpose:
     *      Retrieve an INTEGER REFERENCE property from the registry given
     *  its Property name "pName".  If the Property is found it will return
     *  HXR_OK and pValue will be assigned the address of the integer
     *  (not the value of the integer, which can be obtained even for
     *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetIntRefByName)         (THIS_
                                        const char*     pName,
                                        REF(INT32*)     pValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetIntRefById
     *  Purpose:
     *      Retrieve an INTEGER REFERENCE property from the registry given
     *  its id "ulId".  If the Property is found it will return
     *  HXR_OK and pValue will be assigned the address of the integer
     *  (not the value of the integer, which can be obtained even for
     *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetIntRefById)           (THIS_
                                        const UINT32    id,
                                        REF(INT32*)     pValue) const PURE;



    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddInt64
     *  Purpose:
     *      Add an INTEGER property with name in "pName" and value in
     *  "iValue" to the registry. The return value is the id to
     *  the newly added Property or ZERO if there was an error.
     *
     *  pName - IN - name of the Property that is going to be added to
     *               the registry
     *  nValue - IN - integer value of the Property that is going to be
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddInt64)       (THIS_
                                        const char*     pName,
                                        const INT64     nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64ByName
     *  Purpose:
     *      Retrieve a 64-bit INTEGER value from the registry given its
     *  Property name "pName". If the Property is found, it will return
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned
     */
    STDMETHOD(GetInt64ByName)          (THIS_
                                        const char*     pName,
                                        REF(INT64)      nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64ById
     *  Purpose:
     *      Retrieve a 64-bit INTEGER value from the registry given its id
     *  "ulId".  If the Property is found, it will return HXR_OK, otherwise
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned
     */
    STDMETHOD(GetInt64ById)            (THIS_
                                        const UINT32    ulId,
                                        REF(INT64)      nValue) const PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetInt64ByName
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName". If the value was set, it will return HXR_OK,
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetInt64ByName)          (THIS_
                                        const char*     pName,
                                        const INT64     nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetInt64ById
     *  Purpose:
     *      Modify a Property's 64-bit INTEGER value in the registry given the
     *  its id "id". If the value was set, it will return HXR_OK, otherwise
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetInt64ById)            (THIS_
                                        const UINT32    id,
                                        const INT64     nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::ModifyInt64ByName
     *  Purpose:
     *      Changes the 64-bit INTEGER value in the registry given its Property
     *  name "pName" and an amount to change it by.  Modifies the value
     *  of the integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification.  If the
     *  Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyInt64ByName)       (THIS_
                                        const char*     pName,
					INT64           nDelta,
                                        REF(INT64)      nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::ModifyInt64ById
     *  Purpose:
     *      Changes the 64-bit INTEGER value in the registry given its id
     *  "ulID and an amount to change it by.  Modifies the value of the
     *  integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification.  If the
     *  Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyInt64ById)         (THIS_
                                        const UINT32    id,
					INT64           nDelta,
                                        REF(INT64)      nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::BoundedModifyInt64ByName
     *  Purpose:
     *      Changes the 64-bit INT val in the registry given its Property name
     *  "pName" and an amount to change it by and keeps the modified value 
     *  within the bounds of the nMin and nMax values. Modifies the value 
     *  of the integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification, if the modified 
     *  value is >= nMin and <= nMax. If either of these limits are violated 
     *  the the resulting value stored in the registry is the value of the 
     *  limit just violated. If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nMin - IN - min value that the modified registry prop can have
     *              if the modified registry value < nMin then
     *                  registry value = nMin
     *  nMax - IN - min value that the modified registry prop can have
     *              if the modified registry value > nMax then
     *                  registry value = nMax
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     *
     *  NOTE:
     *     the default values should b changed from INT_MIN/MAX to their
     *  appropriate 64-bit values
     */
    STDMETHOD(BoundedModifyInt64ByName) (THIS_
                                        const char*     pName,
					INT64           nDelta,
                                        REF(INT64)      nValue,
					INT64           nMin=INT64(INT_MIN),
					INT64           nMax=INT64(INT_MAX)) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::BoundedModifyInt64ById
     *  Purpose:
     *      Changes the 64-bit INT val in the registry given its id "ulID"
     *  and an amount to change it by and keeps the modified value within
     *  the bounds of the nMin and nMax values. Modifies the value of the
     *  integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification, if the modified
     *  value is >= nMin and <= nMax. If either of these limits are violated
     *  the the resulting value stored in the registry is the value of the
     *  limit just violated. If the Property is found, it will return HXR_OK, 
     *  otherwise it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nMin - IN - min value that the modified registry prop can have
     *              if the modified registry value < nMin then
     *                  registry value = nMin
     *  nMax - IN - min value that the modified registry prop can have
     *              if the modified registry value > nMax then
     *                  registry value = nMax
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     *
     *  NOTE:
     *     the default values should b changed from INT_MIN/MAX to their
     *  appropriate 64-bit values
     */
    STDMETHOD(BoundedModifyInt64ById)   (THIS_
                                        const UINT32    id,
					INT64           nDelta,
                                        REF(INT64)      nValue,
					INT64           nMin=INT64(INT_MIN),
					INT64           nMax=INT64(INT_MAX)) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetAndReturnInt64ByName
     *  Purpose:
     *      Modify a Property's 64-bit INTEGER value in the registry given
     *  the Property's name "pName". If the Property is found, the previous
     *  value, prior to setting it, will be assigned to nOldValue.
     *  If the Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnInt64ByName) (THIS_
                                        const char*     pName,
					INT64           nValue,
                                        REF(INT64)      nOldValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetAndReturnInt64ById
     *  Purpose:
     *      Modify a Property's 64-bit INTEGER value in the registry given
     *  the Property's id "ulId". If the id is found, the previous
     *  value, prior to setting it, will be assigned to nOldValue.
     *  If the Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnInt64ById)   (THIS_
                                        const UINT32    id,
					INT64           nValue,
                                        REF(INT64)      nOldValue) PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddInt64Ref
     *  Purpose:
     *      Add a 64-bit INTEGER REFERENCE property with name in "pName"
     *  and value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.
     *
     *  pName - IN - name of the Property that is going to be added to
     *               the registry
     *  pValue - IN - the pointer of the integer value is what gets stored
     *                in the registry as the Integer Reference Property's
     *                value
     */
    STDMETHOD_(UINT32, AddInt64Ref)     (THIS_
                                        const char*     pName,
                                        INT64*          pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64RefByName
     *  Purpose:
     *      Retrieve a 64-bit INTEGER REFERENCE property from the registry
     *  given its Property name "pName".  If the Property is found it will
     *  return HXR_OK and pValue will be assigned the address of the integer
     *  (not the value of the integer, which can be obtained even for
     *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetInt64RefByName)       (THIS_
                                        const char*     pName,
                                        REF(INT64*)     pValue) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64RefById
     *  Purpose:
     *      Retrieve a 64-bit INTEGER REFERENCE property from the registry
     *  given its id "ulId".  If the Property is found it will return
     *  HXR_OK and pValue will be assigned the address of the integer
     *  (not the value of the integer, which can be obtained even for
     *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetInt64RefById)         (THIS_
                                        const UINT32    id,
                                        REF(INT64*)     pValue) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetChildIdListByName
     *  Purpose:
     *      Get a array which enumerates all of the children under a 
     *      property by id. 
     *
     *  pName - IN - name of the Property whose children are to be enumerated.
     *  pValues - OUT - array of unique Property id's.
     *  ulCount - OUT - size of the returned pValues array.
     *
     *  Note: The array must be deleted by the user.
     */
    STDMETHOD(GetChildIdListByName)     (THIS_
				         const char*    pName,
				         REF(UINT32*)   pValues,
				         REF(UINT32)    ulCount) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetChildIdListById
     *  Purpose:
     *      Get a array which enumerates all of the children under a 
     *      property by id. 
     *
     *  ulId - IN - unique id of the Property whose children are to be enumerated.
     *  pValues - OUT - array of unique Property id's.
     *  ulCount - OUT - size of the returned pValues array.
     *
     *  Note: The pValues array must be deleted by the user.
     */
    STDMETHOD(GetChildIdListById)       (THIS_
				         const UINT32   ulId,
				         REF(UINT32*)   pValues,
				         REF(UINT32)    ulCount) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropStatusById
     *  Purpose:
     *      Gets status of a property by id.
     *
     *  ulId - IN - id of property to get child ids for.
     *
     *  Returns:
     *    HXR_OK if property exists.
     *    HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
     *    HXR_FAIL if property does not exist.
     */

    STDMETHOD(GetPropStatusById) (THIS_
				  const UINT32 ulId) const PURE;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropStatusByName
     *  Purpose:
     *      Gets status of a property by name.
     *
     *  szPropName - IN - name of property to get child ids for.
     *
     *  Returns:
     *    HXR_OK if property exists.
     *    HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
     *    HXR_FAIL if property does not exist.
     */

    STDMETHOD(GetPropStatusByName) (THIS_
				    const char* pName) const PURE;

};
// $EndPrivate.



// $Private:
/*
 *
 *  Interface:
 *
 *	IHXDeletedPropResponse 
 *
 *  Purpose:
 *
 *      Provides an alternative way to be notified about the deletion
 *      of objects from the registry.
 *
 *  IID_IHXDeletePropResponse
 *
 *      {00000609-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDeletedPropResponse, 0x00000609, 0x901, 0x11d1,
                        0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef INTERFACE
#define INTERFACE IHXDeletedPropResponse

DECLARE_INTERFACE_(IHXDeletedPropResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXDeletedPropResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedComposite
     *  Purpose:
     *      Provide notification that a COMPOSITE Property has been deleted
     *      from the registry.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     */
    STDMETHOD(DeletedComposite)(THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedInt
     *  Purpose:
     *      Provide notification that a INTEGER Property has been deleted.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the Property being deleted (null-terminated)
     *  nValue - IN - integer value of the Property which has been deleted
     */
    STDMETHOD(DeletedInt)      (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                const INT32     nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedIntRef
     *  Purpose:
     *      Provide notification that an INTEGER reference Property has
     *      been deleted from the registry.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     *  nValue - IN - integer value of the Property which has been deleted
     *  pValue - IN - the pointer of the integer reference Property
     *                which has been deleted
     */
    STDMETHOD(DeletedIntRef)   (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                const INT32     nValue,
                                const INT32*    pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedString
     *  Purpose:
     *      Provide notification that a String Property has been deleted
     *      from the registry.
     *
     *  ulId - IN - unique id of the deleted Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     *  pValue - IN - value of the deleted Property 
     */
    STDMETHOD(DeletedString)   (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                IHXBuffer*     pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedBuffer
     *  Purpose:
     *      Provide notification that a Buffer Property has been deleted
     *      from the registry.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     *  pValue - IN - buffer value of the deleted Property 
     */
    STDMETHOD(DeletedBuffer)   (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                IHXBuffer*     pValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedInt64
     *  Purpose:
     *      Provide notification that a 64-bit integer Property has been
     *      deleted from the registry.
     *
     *  Note: This is not used for IHXRegistry, but for IHXRegistry2.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     *  nValue - IN - 64-bit integer value of the deleted Property 
     */
    STDMETHOD(DeletedInt64)    (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                const INT64     nValue) PURE;

    /************************************************************************
     *  Method:
     *      IHXDeletedPropResponse::DeletedInt64Ref
     *  Purpose:
     *      Provide notification that a 64-bit integer reference Property
     *      has been deleted from the registry.
     *
     *  Note: This is not used for IHXRegistry, but for IHXRegistry2.
     *
     *  ulId - IN - unique id of the Property which has been deleted
     *  ulParentID - IN - unique id of the parent Property of the deleted item
     *  bIsParentNotify - IN - TRUE if this is a parent notification.
     *  pName - IN - name of the deleted Property (null-terminated)
     *  nValue - IN - 64-bit integer value of the deleted Property 
     *  pValue - IN - the pointer of the 64-bit integer reference Property
     *                which has been deleted
     */
    STDMETHOD(DeletedInt64Ref) (THIS_
                                const UINT32    ulId,
                                const UINT32    ulParentID,
				const HXBOOL      bIsParentNotify,
                                IHXBuffer*     pName, 
                                const INT64     nValue,
                                const INT64*    pValue) PURE;

};
// $EndPrivate.

#endif /* _HXMON_H_ */
