/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
//Added by qt3to4:
#include <pthread.h>

#include <qatomic.h>

#include "atomicstring.h"

#if __GNUC__ >= 3

    // Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
    // or anything like that.
    const unsigned PHI = 0x9e3779b9U;
    // Copyright (c) Paul Hsieh
    // http://www.azillionmonkeys.com/qed/hash.html
    struct AtomicString::SuperFastHash
    {
        uint32_t operator()( const QString *string ) const
        {
            unsigned l = string->length();
            const QChar *s = string->unicode();
            uint32_t hash = PHI;
            uint32_t tmp;

            int rem = l & 1;
            l >>= 1;

            // Main loop
            for (; l > 0; l--) {
                hash += s[0].unicode();
                tmp = (s[1].unicode() << 11) ^ hash;
                hash = (hash << 16) ^ tmp;
                s += 2;
                hash += hash >> 11;
            }

            // Handle end case
            if (rem) {
                hash += s[0].unicode();
                hash ^= hash << 11;
                hash += hash >> 17;
            }

            // Force "avalanching" of final 127 bits
            hash ^= hash << 3;
            hash += hash >> 5;
            hash ^= hash << 2;
            hash += hash >> 15;
            hash ^= hash << 10;

            // this avoids ever returning a hash code of 0, since that is used to
            // signal "hash not computed yet", using a value that is likely to be
            // effectively the same as 0 when the low bits are masked
            if (hash == 0)
                hash = 0x80000000;

            return hash;
        }
    };

    struct AtomicString::equal
    {
        bool operator()( const QString *a, const QString *b ) const
        {
            return *a == *b;
        }
    };

#endif

class AtomicString::Data: public QString
{
public:
    QBasicAtomic refcount;
    Data() { refcount.init( 0 ); }
    Data( const QString &s ): QString( s ){ refcount.init( 0 ); }
};



AtomicString::AtomicString(): m_string( 0 ) { }

AtomicString::AtomicString( const AtomicString &other )
{
    m_string = other.m_string;
    // No need to lock here: ref() is atomic, and it could not be deleted in between because
    // at least one other ref exists (other's)
    if( m_string ) m_string->refcount.ref();
}

AtomicString::AtomicString( const QString &string ): m_string( 0 )
{
    if( string.isEmpty() )
        return;
    
    Data *s = new Data( string );
    s_storeMutex.lock();
    m_string = static_cast<Data *>( *( s_store.insert( s ).first ) );
    // The atomic increment and test here is important to protect from the unlocked deref()
    if( m_string->refcount.ref() == 1 && m_string != s ) {
        // Well, it *is* about to be deleted! (see deref() ). In this case it's a bad idea
        // to use it, we'll have a hard time synchronizing with the deleter. So we mark it
        // removed (ref=-1) and replace it with our copy. This is safe because nobody but
        // us and the deleter has a pointer to *m_string, and we're about to forget ours
        m_string->refcount = -1;
        s_store.erase( m_string );
        m_string = s;
        s_store.insert( m_string );
        m_string->refcount.ref();
    }
    s_storeMutex.unlock();
    
    if ( m_string != s ) delete( s );
}

AtomicString::~AtomicString()
{
    deref( m_string );
}

AtomicString &AtomicString::operator=( const AtomicString &other )
{
    if( m_string == other.m_string )
        return *this;
    deref( m_string );
    m_string = other.m_string;
    // no need for store lock: other is holding a ref
    if( m_string ) m_string->refcount.ref();
    return *this;
}

// call without holding the lock. if needed, it will acquire the lock itself
inline void AtomicString::deref( Data *s )
{
    if ( !s ) return;
    Data *old_s = s;
    if ( s->refcount.deref() == 0 ) {
        s_storeMutex.lock();
        // check again, in case an AtomicString(const QString &) was just happening
        if( s->refcount == 0 ) s_store.erase( s );
        s_storeMutex.unlock();
        delete s;
    }
}

AtomicString::set_type AtomicString::s_store;
QMutex AtomicString::s_storeMutex;
QString AtomicString::s_emptyString;
