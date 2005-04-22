
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

#ifndef _HXCCF_H_
#define _HXCCF_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */

typedef _INTERFACE	IHXCommonClassFactory		IHXCommonClassFactory;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCommonClassFactory
 * 
 *  Purpose:
 * 
 *	RMA interface that manages the creation of common RMA classes.
 * 
 *  IID_IHXCommonClassFactory:
 * 
 *	{00000000-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCommonClassFactory, 0x00000000, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCommonClassFactory

DECLARE_INTERFACE_(IHXCommonClassFactory, IUnknown)
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
     *	IHXCommonClassFactory methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCommonClassFactory::CreateInstance
     *	Purpose:
     *	    Creates instances of common objects supported by the system,
     *	    like IHXBuffer, IHXPacket, IHXValues, etc.
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE: Aggregation is never used. Therefore and outer unknown is
     *	    not passed to this function, and you do not need to code for this
     *	    situation.
     */
    STDMETHOD(CreateInstance)		(THIS_
					REFCLSID    /*IN*/  rclsid,
					void**	    /*OUT*/ ppUnknown) PURE;

    /************************************************************************
     *  Method:
     *	    IHXController::CreateInstanceAggregatable
     *  Purpose:
     *	    Creates instances of common objects that can be aggregated
     *	    supported by the system, like IHXSiteWindowed
     *
     *	    This method is similar to Window's CoCreateInstance() in its 
     *	    purpose, except that it only creates objects of a well known
     *	    types.
     *
     *	    NOTE 1: Unlike CreateInstance, this method will create internal
     *		    objects that support Aggregation.
     *
     *	    NOTE 2: The output interface is always the non-delegating 
     *		    IUnknown.
     */
    STDMETHOD(CreateInstanceAggregatable)
				    (THIS_
				    REFCLSID	    /*IN*/  rclsid,
				    REF(IUnknown*)  /*OUT*/ ppUnknown,
				    IUnknown*	    /*IN*/  pUnkOuter) PURE;
};


#endif /*_HXCCF_H_*/
