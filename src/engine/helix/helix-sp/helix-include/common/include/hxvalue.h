
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

#ifndef _HXVALUE_H_
#define _HXVALUE_H_

#include "hxcom.h"

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IUnknown			    IUnknown;
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXKeyValueList		    IHXKeyValueList;
typedef _INTERFACE  IHXKeyValueListIter            IHXKeyValueListIter;
typedef _INTERFACE  IHXKeyValueListIterOneKey      IHXKeyValueListIterOneKey;
typedef _INTERFACE  IHXValues			    IHXValues;
typedef _INTERFACE  IHXOptions			    IHXOptions;

/* Note : GUIDS 3101 - 3107 are deprecated. */

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXKeyValueList
 *
 *  Purpose:
 *
 *	Stores a list of strings, where strings are keyed by not necessarily
 *      unique keys.
 *	
 *
 *  IHXKeyValueList:
 *
 *	{0x00003108-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXKeyValueList, 0x00003108, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);
#define CLSID_IHXKeyValueList IID_IHXKeyValueList

#undef  INTERFACE
#define INTERFACE   IHXKeyValueList

DECLARE_INTERFACE_(IHXKeyValueList, IUnknown)
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
     * Regular methods
     */

     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::AddKeyValue
     *	Purpose:
     *      Add a new key/value tuple to our list of strings.  You can have
     *      multiple strings for the same key.
     */
    STDMETHOD(AddKeyValue)	(THIS_
				const char* pKey,
				IHXBuffer* pStr) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::GetIter
     *	Purpose:
     *      Return an iterator that allows you to iterate through all the 
     *      key/value tuples in our list of strings.
     */
    STDMETHOD(GetIter)		(THIS_
				REF(IHXKeyValueListIter*) pIter) PURE;


     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::GetIterOneKey
     *	Purpose:
     *      Return an iterator that allows you to iterate through all the 
     *      strings for a particular key.
     */
    STDMETHOD(GetIterOneKey)	(THIS_
				const char* pKey,
				REF(IHXKeyValueListIterOneKey*) pIter) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::AppendAllListItems
     *	Purpose:
     *      Append all the key/string tuples from another list to this list.
     *      (You can have duplicate keys.)
     */
    STDMETHOD(AppendAllListItems)   (THIS_
				    IHXKeyValueList* pList) PURE;
     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::KeyExists
     *	Purpose:
     *      See whether any strings exist for a particular key.
     */
    STDMETHOD_(HXBOOL,KeyExists)  (THIS_
				const char* pKey) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::CreateObject
     *	Purpose:
     *      Create an empty object that is the same class as the current object.
     */
    STDMETHOD(CreateObject)	(THIS_
				REF(IHXKeyValueList*) pNewList) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueList::ImportValues.
     *	Purpose:
     *      Import all the strings from an IHXValues object into this object.
     *      If this object also supports IHXValues, it should also import the 
     *      ULONGs and Buffers.  You can have duplicate keys, and old data is 
     *      left untouched.
     */
    STDMETHOD(ImportValues)	(THIS_
				IHXValues* pValues) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXKeyValueListIter
 *
 *  Purpose:
 *
 *	Iterate over all the items in a CKeyValueList.
 *      Call IHXKeyValueList::GetIter to create an iterator.
 *	
 *
 *  IHXKeyValueListIter:
 *
 *	{0x00003109-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXKeyValueListIter,   0x00003109, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXKeyValueListIter IID_IHXKeyValueListIter

#undef  INTERFACE
#define INTERFACE   IHXKeyValueListIter

DECLARE_INTERFACE_(IHXKeyValueListIter, IUnknown)
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
     * Regular methods
     */

     /************************************************************************
     *	Method:
     *	    IHXKeyValueListIter::GetNextPair
     *	Purpose:
     *      Each call to this method returns one key/value tuple from your
     *      list of strings.  Strings are returned in same order that they
     *      were inserted.
     */
    STDMETHOD(GetNextPair)	(THIS_
				REF(const char*) pKey,
				REF(IHXBuffer*) pStr) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueListIter::ReplaceCurr
     *	Purpose:
     *      Replaces the value in the key/value tuple that was returned 
     *      in the last call to GetNextPair with a new string.
     */
    STDMETHOD(ReplaceCurr)	(THIS_
				IHXBuffer* pStr) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXKeyValueListIterOneKey
 *
 *  Purpose:
 *
 *	Iterate over all the items in a CKeyValueList that match a particular key.
 *      Call IHXKeyValueList::GetIterOneKey to create an iterator.
 *	
 *
 *  IHXKeyValueListIterOneKey:
 *
 *	{0x00003110-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXKeyValueListIterOneKey,   0x00003110, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXKeyValueListIterOneKey IID_IHXKeyValueListIterOneKey

#undef  INTERFACE
#define INTERFACE   IHXKeyValueListIterOneKey

DECLARE_INTERFACE_(IHXKeyValueListIterOneKey, IUnknown)
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
     * Regular methods
     */

     /************************************************************************
     *	Method:
     *	    IHXKeyValueListIterOneKey::GetNextString
     *	Purpose:
     *      Each call to this method returns one string that matches the 
     *      key for this iterator.  Strings are returned in same order that they
     *      were inserted.
     *      
     */
    STDMETHOD(GetNextString)	(THIS_
				REF(IHXBuffer*) pStr) PURE;

     /************************************************************************
     *	Method:
     *	    IHXKeyValueListIterOneKey::ReplaceCurr
     *	Purpose:
     *      Replaces the value in the key/value tuple that was referenced
     *      in the last call to GetNextString with a new string.
     *      
     */
    STDMETHOD(ReplaceCurr)	(THIS_
				IHXBuffer* pStr) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXOptions
 *
 *  Purpose:
 *
 *	This is a generic options interface, implemented by any object to
 *	allow its options to be read and set by another component of the
 *	system.
 *	
 *
 *  IHXOptions:
 *
 *	{0x00003111-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXOptions,   0x00003111, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXOptions IID_IHXOptions

#undef  INTERFACE
#define INTERFACE   IHXOptions

DECLARE_INTERFACE_(IHXOptions, IUnknown)
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
     * Regular methods
     */

     /************************************************************************
     *	Method:
     *	    IHXOptions::GetOptions
     *	Purpose:
     *      This method returns a list of the options supported by this
     *	    particular object, along with the value currently set for each
     *	    option. Enumerate the members of the returned IHXValues object
     *	    to discover what options a component supports and the type of
     *	    each of those options. The value for each name-value pair is
     *	    the current setting for that option.
     *      
     */
    STDMETHOD(GetOptions)	(THIS_
				REF(IHXValues*) pOptions) PURE;

     /************************************************************************
     *	Method:
     *	    IHXOptions::SetOptionULONG32
     *	Purpose:
     *      Sets the value of a ULONG32 option. The return value indicates
     *	    whether or not the SetOptionULONG32 call succeeded.
     *      
     */
    STDMETHOD(SetOptionULONG32)	(THIS_
				const char* pName,
				ULONG32 ulValue) PURE;

     /************************************************************************
     *	Method:
     *	    IHXOptions::SetOptionCString
     *	Purpose:
     *      Sets the value of a CString option. The return value indicates
     *	    whether or not the SetOptionCString call succeeded.
     *      
     */
    STDMETHOD(SetOptionCString)	(THIS_
				const char* pName,
				IHXBuffer* pValue) PURE;

     /************************************************************************
     *	Method:
     *	    IHXOptions::SetOptionBuffer
     *	Purpose:
     *      Sets the value of a Buffer option. The return value indicates
     *	    whether or not the SetOptionBuffer call succeeded.
     *      
     */
    STDMETHOD(SetOptionBuffer)	(THIS_
				const char* pName,
				IHXBuffer* pValue) PURE;
};


#endif /* !_HXVALUE_H_ */
