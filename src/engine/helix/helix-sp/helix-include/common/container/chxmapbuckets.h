
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

#ifndef _CHLXMAPBUCKETS_H_
#define _CHLXMAPBUCKETS_H_

#include "hxmaputils.h"

class CHlxMapBuckets
{
public:
    typedef HlxMap::IntVec_t ITEM;

    CHlxMapBuckets() : m_items(0), m_size(0)
    {
    }

    CHlxMapBuckets(UINT16 num) : m_items(0), m_size(0)
    {
        // Nasty new in constructor makes it hard to detect OOM errors,
        // but I don't think this constructor is actually used by anybody.
        m_items = new ITEM[num];
        m_size = num;
    }

    ~CHlxMapBuckets() { HX_VECTOR_DELETE(m_items); }

    inline ITEM& operator[] (int idx)
    {
        return m_items[idx];
    }

    inline const ITEM& operator[] (int idx) const
    {
        return m_items[idx];
    }

    inline bool empty () const { return !m_items; }

    inline UINT16 size() const { return m_size; }

    inline HX_RESULT Init (UINT16 num)
    {
        HX_VECTOR_DELETE(m_items);
        m_items = new ITEM[num];
        if( !m_items )
        {
            return HXR_OUTOFMEMORY;
        }
        m_size = num;
        return HXR_OK;
    }

private:
    ITEM*       m_items;
    UINT16      m_size;
};

#endif // _CHLXMAPBUCKETS_H_
