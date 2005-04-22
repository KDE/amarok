
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

#ifndef _CHXMAPSTRINGTOOB_H_
#define _CHXMAPSTRINGTOOB_H_

// Notes...
//
// Since we aren't using templates, we get to copy the same basic code all
// over the place.  So, if you change something in this class, chances are
// that the other CHXMap*To* classes may need the change as well.
// XXXSAB: Need to better abstract out the common code...
//
// This implementation has a few dynamically resized vectors - their
// "chunk sizes" (number of elements added to size when a new element
// addition requires a reallocation) can be adjusted via the following
// accessors.
//
//    m_items - This is the vector of actual key/value pairs (along with a
//        boolean "free" flag) where the data for the map is stored.  It's
//        chunk size is controlled via the optional argument to the map
//        ctor.  And the default value for that is controlled by the
//        static SetDefaultChunkSize() method.
//
//    m_buckets - This is the vector of hash buckets.  Each hash bucket is
//        a vector of int indices into the m_items vector.  The number of
//        buckets doesn't change over time and is controlled via the
//        InitHashTable() method (which has the effect of resetting the
//        map) and it defaults to z_defaultNumBuckets (101 at the moment).
//        The chunk size of the individual hash buckets is set by the
//        SetBucketChunkSize() method and the default for that is set by
//        the SetDefaultBucketChunkSize() method.
//

#include "hxtypes.h"
#include "carray.h"
#include "hxstring.h"

#include "hxmaputils.h"
#include "chxmapbuckets.h"

#include "hlxclib/string.h"     // strcasecmp()

class CHXMapStringToOb
{
public:
    typedef const char* key_type;
    typedef const char* key_arg_type;
    typedef const char* key_ref_type;

    inline static CHXString& key_nil() { return (CHXString&)HXEmptyString; }

    typedef void* value_type;
    typedef void* value_arg_type;
    typedef void*& value_ref_type;
    typedef void* value_const_ref_type;
    inline static value_ref_type val_nil() { static const value_type p = 0; return (value_ref_type)p; }

    struct Item
    {
        Item (key_arg_type key_ = key_nil(),
              value_arg_type val_ = val_nil(),
              bool bFree_ = true) :
            key(key_), val(val_), bFree(bFree_)
        {}

        CHXString  key;
        value_type val;
        bool  bFree;
    };
    DECLARE_ITEMVEC(ItemVec_t,Item,Item(),0,0);

    class Iterator
    {
    public:
        typedef key_type iter_key_type;
        friend class CHXMapStringToOb;

        // NOTE: (item == -1) is used to mean "set to end of pItems".
        Iterator(ItemVec_t* pItems = NULL,
                 int item = -1);

        // NOTE: Values of 'next' copied into iterator...since this
        //       iterator is caching key/value and doesn't return a
        //       value_type&, it can't be used to modify the values in the
        //       map.
        Iterator& operator++();
        Iterator  operator++(int); // XXXSAB: tested?

        HXBOOL operator==(const Iterator&) const;
        HXBOOL operator!=(const Iterator&) const;
        value_type operator*(); // returns the 'value'
        iter_key_type get_key  ();   // returns the 'key'

    private:
        void GotoValid();

        ItemVec_t*      m_pItems;
        int             m_item;

        // cached key/value
        CHXString       m_key;
        value_type      m_val;
    };

private:

#if defined(HELIX_CONFIG_NOSTATICS)
    static const ULONG32 z_defaultNumBuckets;
    static const ULONG32 z_defaultChunkSize;
    static const ULONG32 z_defaultBucketChunkSize;
#else
    static ULONG32 z_defaultNumBuckets;
    static ULONG32 z_defaultChunkSize;
    static ULONG32 z_defaultBucketChunkSize;
#endif
    

public:
    // Construction

    // NOTE: Chunk size is the number of key/value pairs to grow by each
    //       time one of the hash buckets needs to be grown.
    CHXMapStringToOb(int chunkSize = z_defaultChunkSize);
    ~CHXMapStringToOb();

    // Attributes
    inline int GetCount() const;
    inline HXBOOL IsEmpty() const;

    HXBOOL Lookup(key_arg_type key, value_arg_type& value) const;
    POSITION Lookup(key_arg_type key) const;

    // XXXSAB: I added GetKeyAt() and GetAt() since there was previously
    //         no easy way to get those data without advancing the
    //         POSITION.
    key_ref_type GetKeyAt(POSITION pos) const;
    value_const_ref_type GetAt(POSITION pos) const;
    value_ref_type GetAt(POSITION pos);

    // Lookup & add if not there
    value_ref_type operator[](key_arg_type key);

    // add a new (key, value) pair
    POSITION SetAt(key_arg_type key, value_arg_type value);

    // remove existing (key, ?) pair
    POSITION Remove(key_arg_type key);

    HXBOOL RemoveKey(key_arg_type key);

    void RemoveAll();

    // Iteration
    POSITION GetStartPosition() const;
    void GetNextAssoc (POSITION& pos, key_arg_type& key, value_arg_type& value) const;

    // CHXString Helper...
    void GetNextAssoc (POSITION& pos, CHXString& key, value_arg_type& value) const
    {
        const char* sz = NULL;
        GetNextAssoc (pos, sz, value);
        if (sz) key = sz;
    }

    Iterator Begin();
    Iterator End();
    Iterator Erase(Iterator it);
    // XXXSAB: Added Find() command to parallel STL style method
    Iterator Find(key_arg_type key);

    // Returns the number of hash buckets
    inline ULONG32 GetHashTableSize() const;

    // This will reset the internal storage so that any the map will be
    // empty when this returns.
    // NOTE: This function always allocates some data - the bAlloc flag is
    //       for compatibility with an old interface and is ignored.
    HX_RESULT InitHashTable(ULONG32 numBuckets = z_defaultNumBuckets,
                       HXBOOL bAlloc = TRUE);

    // defaults to on. Set this before you add anything! And make sure
    // that if you set a hash function that it's behavior matches the case
    // sensitivity flag.
    void SetCaseSensitive(HXBOOL bCaseSens) { m_bCaseSens = bCaseSens ? true : false; }

    typedef ULONG32 (*HashFunc_t) (key_arg_type key);
    static ULONG32 DefaultHashFunc (key_arg_type key)
    {
        return HlxMap::StrHashFunc(key, true);
    }
    static ULONG32 DefaultNoCaseHashFunc (key_arg_type key)
    {
        return HlxMap::StrHashFunc(key, false);
    }
    inline HashFunc_t SetHashFunc (HashFunc_t hf = DefaultHashFunc); // XXXSAB: tested???

    // Overrideables: special non-virtual (XXXSAB: Huh?)
    inline ULONG32 HashKey(key_arg_type key) const;

    inline static void SetDefaultNumBuckets (ULONG32 numBuckets);
    inline static void SetDefaultChunkSize (ULONG32 chunkSize);
    inline static void SetDefaultBucketChunkSize (ULONG32 chunkSize);
    inline void SetBucketChunkSize (ULONG32 chunkSize);

    // In _DEBUG mode, this does a bunch of DPRINTF's...
    void Dump() const;

private:
    inline HXBOOL Lookup(key_arg_type key, int& retItem) const;
    HXBOOL LookupInBucket(ULONG32 bucket, key_arg_type key, int& retItem) const;
    Item* LookupItem(ULONG32 bucket, key_arg_type key);
    inline const Item* LookupItem(ULONG32 bucket, key_arg_type key) const
    {
        return ((CHXMapStringToOb*)this)->LookupItem(bucket, key);
    }

    // Internal function - key already verified not to exist
    HXBOOL AddToBucket(ULONG32 bucket, key_arg_type key, value_arg_type value, int& retItem);

    inline POSITION Item2Pos(int item) const;
    inline int Pos2Item(POSITION pos) const;

private:

    HashFunc_t          m_hf;

    ItemVec_t           m_items;
    HlxMap::IntVec_t    m_free;

    CHlxMapBuckets      m_buckets;
    ULONG32             m_numBuckets;
    ULONG32             m_chunkSize;
    ULONG32             m_bucketChunkSize;

    // Members specific to the type of key and/or value goes below here.
    void ConstructTypeSpecifics();
    inline HXBOOL IsKeyMatch (key_arg_type k1, key_arg_type k2) const
    {
        HXBOOL bRet;

        if (m_bCaseSens) bRet = (strcmp(k1, k2) == 0);
        else bRet = (strcasecmp(k1, k2) == 0);

#ifdef XXXSAB
        printf ("IsKeyMatch(\"%s\", \"%s\") -> %s\n",
                k1, k2,
                bRet ? "true" : "false");
#endif /* XXXSAB */

        return bRet;
    }

    bool                m_bCaseSens;
};

int CHXMapStringToOb::GetCount() const
{
    return m_items.size() - m_free.size();
}

HXBOOL CHXMapStringToOb::IsEmpty() const
{
    return GetCount() == 0;
}

ULONG32 CHXMapStringToOb::GetHashTableSize() const
{
    return m_numBuckets;
}

CHXMapStringToOb::HashFunc_t CHXMapStringToOb::SetHashFunc (
    CHXMapStringToOb::HashFunc_t hf)
{
    HashFunc_t old = m_hf;
    m_hf = hf;
    return old;
}

ULONG32 CHXMapStringToOb::HashKey (key_arg_type key) const
{
    if (m_hf) return m_hf(key);
    return m_bCaseSens ? DefaultHashFunc(key) : DefaultNoCaseHashFunc(key);
}

void CHXMapStringToOb::SetDefaultNumBuckets (ULONG32 numBuckets)
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    z_defaultNumBuckets = numBuckets;
#endif
}

void CHXMapStringToOb::SetDefaultChunkSize (ULONG32 chunkSize)
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    z_defaultChunkSize = chunkSize;
#endif
}

void CHXMapStringToOb::SetDefaultBucketChunkSize (ULONG32 chunkSize)
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    z_defaultBucketChunkSize = chunkSize;
#endif
}

void CHXMapStringToOb::SetBucketChunkSize (ULONG32 chunkSize)
{
    m_bucketChunkSize = chunkSize;
}

#endif // _CHXMAPSTRINGTOOB_H_
