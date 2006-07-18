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

#include <stdint.h>
#include <qdeepcopy.h>
#include <qstring.h>

#include "atomicstring.h"

#ifdef __GNUC__

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

#else

    struct AtomicString::less
    {
        bool operator()( const QString *a, const QString *b ) const
        {
            return *a < *b;
        }
    };

#endif

class AtomicString::Data: public QString
{
public:
    uint refcount;
    Data(): refcount( 0 ) { }
    Data( const QString &s ): QString( s ), refcount( 0 ) { }
};

AtomicString::AtomicString(): m_string( 0 ) { }

AtomicString::AtomicString( const AtomicString &other ): m_string( other.m_string )
{
    ref( m_string );
}

AtomicString::AtomicString( const QString &string ): m_string( 0 )
{
    if( string.isEmpty() )
        return;

    Data *s = new Data( string );
    m_string = static_cast<Data*>( *( s_store.insert( s ).first ) );
    ref( m_string );
    if( !s->refcount )
        delete s;
}

AtomicString::~AtomicString()
{
    deref( m_string );
}

QString AtomicString::string() const
{
    if( m_string )
        return *m_string;
    return QString::null;
}

const QString *AtomicString::operator->() const
{
    return ptr();
}

QString AtomicString::deepCopy() const
{
    if( m_string )
        return QDeepCopy<QString>( *m_string );
    return QString::null;
}

bool AtomicString::isEmpty() const
{
    return !m_string;
}

const QString *AtomicString::ptr() const
{
    if( m_string )
        return m_string;
    return &QString::null;
}

uint AtomicString::refcount() const
{
    if( m_string )
        return m_string->refcount;
    return 0;
}

AtomicString &AtomicString::operator=( const AtomicString &other )
{
    if( m_string == other.m_string )
        return *this;
    deref( m_string );
    m_string = other.m_string;
    ref( m_string );
    return *this;
}

bool AtomicString::operator==( const AtomicString &other ) const
{
    return m_string == other.m_string;
}

void AtomicString::deref( Data *s )
{
    if( !s )
        return;
    if( !( --s->refcount ) )
    {
        s_store.erase( s );
        delete s;
    }
}

void AtomicString::ref( Data *s )
{
    if( s )
        s->refcount++;
}

AtomicString::set_type AtomicString::s_store;
