
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

#ifndef _HLXMAPUTILS_H_
#define _HLXMAPUTILS_H_

#include "hxtypes.h"
#include "hxassert.h"

typedef void* POSITION;         // XXXSAB: where does this belong?

#if defined(HELIX_CONFIG_LOW_HEAP_HASH_TABLE)
// The default values to the hash class result in terrible heap consumption.
// Using absolute minimal values here. Not much of a hash table now though!
#define CHUNK_INIT 1
#else
#define CHUNK_INIT 16
#endif

#define DECLARE_ITEMVEC(CLASS,ITEM,NIL,CHUNK,INIT) \
    class CLASS \
    { \
    public: \
        CLASS(); \
        CLASS(int num);\
        CLASS(int num, const ITEM& item); \
        CLASS(const CLASS& from); \
        CLASS& operator= (const CLASS& from); \
        ~CLASS(); \
 \
        inline ITEM& operator[] (int idx) \
        { \
            return m_items[idx]; \
            /* return (idx >= 0 && idx < m_used ? m_items[idx] : nil()); */ \
        } \
 \
        inline const ITEM& operator[] (int idx) const \
        { \
            return m_items[idx]; \
            /* return (idx >= 0 && idx < m_used ? m_items[idx] : nil()); */ \
        } \
 \
        inline bool empty () const { return m_used <= 0; } \
        inline int size () const { return m_used; } \
        inline void resize (int s) \
        { \
            resize(s, INIT); \
        } \
 \
        void resize (int s, const ITEM& item); \
 \
        inline int capacity () const { return m_alloc; } \
\
        void reserve (int s); \
\
        inline void SetChunkSize (int chunk) { m_chunkSize = chunk; } \
        void GrowBy (int by); \
 \
        CLASS& push_back (const ITEM& item); \
 \
        inline CLASS& pop_back () \
        { \
            HX_ASSERT (m_used > 0); \
            if (m_used > 0) --m_used; \
            return *this; \
        } \
 \
        inline ITEM& back() \
        { \
            HX_ASSERT (m_items); HX_ASSERT (m_used > 0); \
            return m_items[m_used-1]; \
        } \
 \
        void zap (int idx, int numToZap = 1); \
 \
    private: \
        ITEM*       m_items; \
        int         m_alloc; \
        int         m_used; \
        UINT16      m_chunkSize; \
    }

#define DECLARE_ITEMVEC_IMP(PARENT, CLASS,ITEM,NIL,CHUNK,INIT) \
        PARENT::CLASS::CLASS() : \
            m_items(0), m_alloc(0), m_used(0), m_chunkSize(CHUNK) \
        { \
        } \
 \
        PARENT::CLASS::CLASS(int num) : \
            m_items(0), m_alloc(0), m_used(0), m_chunkSize(CHUNK) \
        { \
            if (num > 0) \
            { \
                m_items = new ITEM[num]; \
                m_used = m_alloc = num; \
                for (int i = 0; i < num; ++i) m_items[i] = INIT; \
            } \
        } \
 \
        PARENT::CLASS::CLASS(int num, const ITEM& item) : \
            m_items(0), m_alloc(0), m_used(0), m_chunkSize(CHUNK) \
        { \
            if (num > 0) \
            { \
                m_items = new ITEM[num]; \
                m_used = m_alloc = num; \
                for (int i = 0; i < num; ++i) m_items[i] = item; \
            } \
        } \
 \
        PARENT::CLASS::CLASS(const PARENT::CLASS& from) : \
            m_items(0), m_alloc(0), m_used(0), m_chunkSize(CHUNK) \
        { \
            m_used = from.m_used; \
            m_alloc = from.m_alloc; \
            m_items = new ITEM[m_alloc]; \
            for (int i = 0; i < m_used; ++i) m_items[i] = from.m_items[i]; \
        } \
 \
        PARENT::CLASS& PARENT::CLASS::operator= (const PARENT::CLASS& from) \
        { \
            if (m_items != from.m_items) \
            { \
                HX_VECTOR_DELETE(m_items); \
                m_used = from.m_used; \
                m_alloc = from.m_alloc; \
                m_items = new ITEM[m_alloc]; \
                for (int i = 0; i < m_used; ++i) m_items[i] = from.m_items[i]; \
            } \
            return *this; \
        } \
 \
        PARENT::CLASS::~CLASS() { HX_VECTOR_DELETE(m_items); } \
 \
 \
        void PARENT::CLASS::resize (int s, const ITEM& item) \
        { \
            reserve(s); \
            for (int i = m_used; i < s; ++i) m_items[i] = item; \
            m_used = s; \
        } \
 \
        void PARENT::CLASS::reserve (int s) \
        { \
            if (s > m_alloc) \
            { \
                ITEM* newItems = new ITEM[s]; \
                if( newItems ){ \
                for (int i = 0; i < m_used; ++i) newItems[i] = m_items[i]; \
                HX_VECTOR_DELETE (m_items); \
                m_items = newItems; \
                m_alloc = s; \
                } \
            } \
        } \
 \
        void PARENT::CLASS::GrowBy (int by) \
        { \
            /* If no chunkSize specified, \
               use the larger of 16 and the currently allocated amount. \
            */ \
 \
            int chunk = m_chunkSize > 0 ? m_chunkSize : MAX (m_alloc, CHUNK_INIT); \
            int newAlloc = m_alloc + ((by + chunk - 1) / chunk) * chunk; \
            reserve (newAlloc); \
        } \
 \
        PARENT::CLASS& PARENT::CLASS::push_back (const ITEM& item) \
        { \
            if (m_used == m_alloc) GrowBy (1); \
            HX_ASSERT (m_used < m_alloc); \
            m_items[m_used++] = item; \
            return *this; \
        } \
 \
        void PARENT::CLASS::zap (int idx, int numToZap) \
        { \
            HX_ASSERT (idx >= 0 && idx < m_used); \
 \
            if ((idx + numToZap) >= m_used) \
            { \
                m_used = idx; \
            } \
            else \
            { \
                int src = idx + numToZap; \
                int dest = idx; \
                for (; src < m_used; ++src, ++dest) \
                    m_items[dest] = m_items[src]; \
                m_used -= numToZap; \
            } \
        } 

struct HlxMap
{
    DECLARE_ITEMVEC(IntVec_t, int, 0, 0, 0);

    static ULONG32 StrHashFunc (const char* key, bool bCaseSens);
};

#endif // _HLXMAPUTILS_H_
