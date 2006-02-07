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

/**
    AtomicString is a thin wrapper over QString that makes sure only one copy of any equivalent string is stored in memory at any one time.
    As a side benefit, comparing AtomicStrings is reduced to a pointer comparison.
    Constructing an AtomicString is slower than constructing a QString as a hash has to be generated to ensure uniqueness.
    According to benchmarks, Paul Hsieh's SuperFastHash (which is currently used) can hash 5 million 256 byte strings in 1.34s on a 1.62GHz Athlon XP.
    For other use, the overhead compared to a QString should be minimal.
**/

#ifndef AMAROK_ATOMICSTRING_H
#define AMAROK_ATOMICSTRING_H

#include "config.h"
#include <ksharedptr.h>
#ifdef __GNUC__
    #include <ext/hash_set>
#else
    #include <set>
#endif
#include "amarok_export.h"

class QString;

class LIBAMAROK_EXPORT AtomicString
{
public:
    /** Constructs a null AtomicString, which is equivalent to QString::null. */
    AtomicString();

    /** Copies an AtomicString. This is as fast as copying a KSharedPtr. */
    AtomicString( const AtomicString &other );

    /** Constructs an AtomicString from a QString.
        This is slower than constructing a QString as a hash function must be run on it. */
    AtomicString( const QString &string );

    virtual ~AtomicString();

    /** Copies an AtomicString. This is as fast as copying a KSharedPtr.
        Note that you can pass a QString here and an AtomicString will be constructed from it. */
    AtomicString &operator=( const AtomicString &other );

    /** Tests for equivalence with another AtomicString. This is just a pointer comparison.
        Note that you can pass a QString here and an AtomicString will be constructed from it. */
    bool operator==( const AtomicString &other ) const;

    /** Returns the string. */
    const QString string() const;

    /** Returns the string. */
    QString string();

    /** Implicitly casts to a const QString&. */
    inline operator const QString() const { return string(); }

    /** Implicitly casts to a QString.
        Note that this isn't a reference, so you don't end up modifying shared data by accident. */
    inline operator QString() { return string(); }

    /** Provided for convenience, so you can do atomicstring->QStringfunction(),
        instead of atomicstring.string().QStringfunction(). */
    const QString *operator->() const;

    /** Returns a deep copy of the string. Useful for threading. */
    QString deepCopy() const;

    /** For convenience, in AtomicString's case they are equivalent. */
    bool isNull() const;
    inline bool isEmpty() const { return isNull(); }

    /** Returns the internal pointer to the string.
        The pointer is guaranteed to be equivalent for equivalent strings, and different for different ones.
        This is useful for certain kinds of hacks, but shouldn't normally be used. */
    const QString *ptr() const;

    /** For debugging purposes -- lists all strings and how many places they're used to stderr. */
    static void listContents();

private:
    class Data;
    friend class Data;

    #ifdef __GNUC__
        struct SuperFastHash;
        struct equal;
        typedef __gnu_cxx::hash_set<QString*, SuperFastHash, equal> set_type;
    #else
        struct less;
        typedef std::set<QString*, less> set_type;
    #endif
    static set_type s_store;

    KSharedPtr<Data> m_string;
};

#endif
