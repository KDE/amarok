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
#include "debug.h"

#include "atomicstring.h"

#ifdef __GNUC__

    // Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
    // or anything like that.
    const unsigned PHI = 0x9e3779b9U;
    // Copyright (c) Paul Hsieh
    // http://www.azillionmonkeys.com/qed/hash.html
    struct AtomicString::SuperFastHash
    {
        bool operator()( const QString *string ) const
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

AtomicString::set_type AtomicString::s_store;

class AtomicString::Data: public QString, public KShared
{
public:
    bool stored;
    Data(): stored( false ) { }
    Data( const QString &s ): QString( s ), stored( false ) { }
    virtual ~Data()
    {
        if( stored )
            AtomicString::s_store.erase( this );
    }
};

AtomicString::AtomicString()
{
}

AtomicString::AtomicString( const AtomicString &other )
{
    m_string = other.m_string;
}

AtomicString::AtomicString( const QString &string )
{
    if( string.isEmpty() )
        return;

    KSharedPtr<Data> s = new Data( string );
    const std::pair<set_type::iterator, bool> r = s_store.insert( s.data() );
    m_string = static_cast<Data*>( *r.first );
    m_string->stored = true;
}

AtomicString::~AtomicString()
{
}

const QString AtomicString::string() const
{
    if( m_string )
        return *m_string;
    return QString::null;
}

QString AtomicString::string()
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

bool AtomicString::isNull() const
{
    return !m_string;
}

const QString *AtomicString::ptr() const
{
    if( m_string )
        return m_string;
    return &QString::null;
}

AtomicString &AtomicString::operator=( const AtomicString &other )
{
    m_string = other.m_string;
    return *this;
}

bool AtomicString::operator==( const AtomicString &other ) const
{
    return m_string == other.m_string;
}

void AtomicString::listContents() //static
{
    for( set_type::iterator it = s_store.begin(), end = s_store.end(); it != end; ++it )
        debug() << static_cast<Data*>((*it))->_KShared_count() << " " << (**it) << endl;
}
