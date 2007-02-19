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

#include <config.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <QString>
//Added by qt3to4:
#include <Q3PtrList>
#include <pthread.h>

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
    mutable uint refcount;
    Data(): refcount( 0 ) { }
    Data( const QString &s ): QString( s ), refcount( 0 ) { }
};

AtomicString::AtomicString(): m_string( 0 ) { }

AtomicString::AtomicString( const AtomicString &other )
{
    s_storeMutex.lock();
    m_string = other.m_string;
    ref( m_string );
    s_storeMutex.unlock();
}

AtomicString::AtomicString( const QString &string ): m_string( 0 )
{
    if( string.isEmpty() )
        return;

    Data *s = new Data( string );  // note: s is a shallow copy
    s_storeMutex.lock();
    m_string = static_cast<Data*>( *( s_store.insert( s ).first ) );
    ref( m_string );
    uint rc = s->refcount;
    s_storeMutex.unlock();
    if ( !rc ) delete( s );	// already present
}

AtomicString::~AtomicString()
{
    s_storeMutex.lock();
    deref( m_string );
    s_storeMutex.unlock();
}

QString AtomicString::string() const
{
    if ( !m_string ) return QString();
    // References to the stored string are only allowed to circulate in the main thread
    return *m_string;
}

bool AtomicString::isEmpty() const
{
    return !m_string;
}

const QString *AtomicString::ptr() const
{
    if( !m_string ) {
        Data *s = new Data( QString() );  // note: s is a shallow copy
        s_storeMutex.lock();
        m_string = static_cast<Data*>( *( s_store.insert( s ).first ) );
        ref( m_string );
        uint rc = s->refcount;
        s_storeMutex.unlock();
        if ( !rc ) delete( s );	// already present
    }

    return m_string;
}

uint AtomicString::refcount() const
{
    if ( m_string ) {
	s_storeMutex.lock();
	uint rc = m_string->refcount;
	s_storeMutex.unlock();
	return rc;
    }
    return 0;
}

AtomicString &AtomicString::operator=( const AtomicString &other )
{
    if( m_string == other.m_string )
        return *this;
    s_storeMutex.lock();
    deref( m_string );
    m_string = other.m_string;
    ref( m_string );
    s_storeMutex.unlock();
    return *this;
}

bool AtomicString::operator==( const AtomicString &other ) const
{
    return m_string == other.m_string;
}

bool AtomicString::operator!=( const AtomicString &other ) const
{
    return m_string != other.m_string;
}

// needs to be called holding the lock
inline void AtomicString::deref( Data *s )
{
    checkLazyDeletes();         // a good time to do this
    if( !s )
        return;
    if( !( --s->refcount ) )
    {
        s_store.erase( s );
        // only the main thread is allowed to delete stored strings
        if ( isMainThread() )
            delete s;
        else
            s_lazyDeletes.append(s);
    }
}

// needs to be called holding the lock
inline void AtomicString::ref( Data *s ) const
{
    //checkLazyDeletes();         // a good time to do this
    if( s )
        s->refcount++;
}

// It is not necessary to hold the store mutex here.
bool AtomicString::isMainThread()
{
    // For isMainThread(), we could use QThread::currentThread(), except the
    // docs say it's unreliable. And in general QThreads don't like to be called from
    // app destructors. Good old pthreads will serve us well. As for Windows, these
    // two calls surely have equivalents; better yet we'll have QT4 and thread safe
    // QStrings by then.
    // Note that the the static local init is thread safe.
    static pthread_t main_thread = pthread_self();
    return pthread_equal(pthread_self(), main_thread);
}

// call holding the store mutex
inline void AtomicString::checkLazyDeletes()
{
    // only the main thread is allowed to delete
    if ( isMainThread() )
    {
        s_lazyDeletes.setAutoDelete(true);
        s_lazyDeletes.clear();
    }
}

AtomicString::set_type AtomicString::s_store;
Q3PtrList<QString> AtomicString::s_lazyDeletes;
QMutex AtomicString::s_storeMutex;
