/*
  Copyright (c) 2005 Gábor Lehel <illissius@gmail.com>

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
    AtomicString makes sure only one copy of any equivalent string is stored in memory at any one time.
    As a side benefit, comparing AtomicStrings is reduced to a pointer comparison.
*/

#ifndef AMAROK_ATOMICSTRING_H
#define AMAROK_ATOMICSTRING_H

#include "config.h"
#include <ksharedptr.h>

class QString;

#ifdef __GNUC__
    #include <ext/hash_set>
    struct SuperFastHash;
    struct equal;
    typedef __gnu_cxx::hash_set<QString*, SuperFastHash, equal> set_type;
#else
    #include <set>
    struct less;
    typedef std::set<QString*, less> set_type;
#endif

class AtomicString
{
public:
    AtomicString();
    AtomicString( const AtomicString &other );
    AtomicString( const QString &string );

    virtual ~AtomicString();

    const QString &string() const;
    QString string();
    inline operator const QString&() const { return string(); }
    inline operator QString() { return string(); }
    inline const QString &operator->() const { return string(); }

    AtomicString &operator=( const AtomicString &other );
    bool operator==( const AtomicString &other ) const;

private:
    class Impl;
    friend class Impl;

    static set_type s_store;

    KSharedPtr<Impl> m_string;
};

#endif
