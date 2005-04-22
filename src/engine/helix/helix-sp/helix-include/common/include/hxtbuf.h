
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

#ifndef _HXTBUF_H_
#define _HXTBUF_H_

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXTimeStampedBuffer
 *
 *  Purpose:
 *
 *	Basic opaque data storage buffer. Used in interfaces where 
 *	object ownership is best managed through COM style reference 
 *	counting.
 *
 *  IID_IHXTimeStampedBuffer:
 *
 *	{00000700-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXTimeStampedBuffer, 0x00000700, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#define CLSID_IHXTimeStampedBuffer IID_IHXTimeStampedBuffer

#undef  INTERFACE
#define INTERFACE   IHXTimeStampedBuffer

DECLARE_INTERFACE_(IHXTimeStampedBuffer, IUnknown)
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
     *	IHXTimeStampedBuffer methods
     */
    STDMETHOD_(UINT32,GetTimeStamp)(THIS) PURE;

    STDMETHOD(SetTimeStamp)(THIS_
			    UINT32	ulTimeStamp) PURE;
};

#endif /* _HXTBUF_H_ */
