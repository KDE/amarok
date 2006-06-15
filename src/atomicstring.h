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
 * A thin wrapper over QString which ensures only a single copy of string data
 * is stored for equivalent strings. As a side benefit, testing for equality
 * is reduced to a pointer comparison. Construction is slower than a QString,
 * as it must be checked for equality with existing strings. (A hash set is
 * used for this purpose. According to benchmarks, Paul Hsieh's SuperFastHash
 * (which is currently used -- see http://www.azillionmonkeys.com/qed/hash.html)
 * can hash 5 million 256 byte strings in 1.34s on a 1.62GHz Athlon XP.) For
 * other use, the overhead compared to a plain QString should be minimal.
 *
 * @author Gábor Lehel <illissius@gmail.com>
 */

#ifndef AMAROK_ATOMICSTRING_H
#define AMAROK_ATOMICSTRING_H

#include "config.h"
#ifdef __GNUC__
    #include <ext/hash_set>
#else
    #include <set>
#endif
#include "amarok_export.h"

#include <qstring.h>

class LIBAMAROK_EXPORT AtomicString
{
public:
    /**
     * Constructs an empty string.
     * @see isEmpty
     */
    AtomicString();

    /**
     * Constructs a copy of \p string. This operation takes constant time.
     * @param string the string to copy
     * @see operator=
     */
    AtomicString( const AtomicString &other );

    /**
     * Constructs a copy of \p string.
     * @param string the string to copy
     */
    AtomicString( const QString &string );

    /**
     * Destroys the string.
     * Note: this isn't virtual, to halve sizeof().
     */
    ~AtomicString();

    /**
     * Makes this string a copy of \p string. This operation takes constant time.
     * @param string the string to copy
     */
    AtomicString &operator=( const AtomicString &other );

    /**
     * This operation takes constant time.
     * @return whether this string and \p string are equivalent
     */
    bool operator==( const AtomicString &other ) const;

    /**
     * @return the string
     */
    QString string() const;

    /**
     * Implicitly casts to a QString.
     * @return the string
     */
    inline operator QString() const { return string(); }

    /**
     * For convenience, so you can do atomicstring->QStringfunction(),
     * instead of atomicstring.string().QStringfunction().
     */
    const QString *operator->() const;

    /**
     * Useful for threading.
     * @return a deep copy of the string
     */
    QString deepCopy() const;

    /**
     * For convenience. Equivalent to isNull().
     * @return whether the string is empty
     * @see isNull
     */
    bool isEmpty() const;

    /**
     * For convenience. Equivalent to isEmpty().
     * @return whether the string is empty
     * @see isEmpty
     */
    inline bool isNull() const { return isEmpty(); }

    /**
     * Returns the internal pointer to the string.
     * Guaranteed to be equivalent for equivalent strings, and different for
     * different ones. This can be useful for certain kinds of hacks, but
     * shouldn't normally be used.
     * @return the internal pointer to the string
     */
    const QString *ptr() const;

    /**
     * For debugging purposes.
     * @return the number of nonempty AtomicStrings equivalent to this one
     */
    uint refcount() const;

private:
    #ifdef __GNUC__
        struct SuperFastHash;
        struct equal;
        typedef __gnu_cxx::hash_set<QString*, SuperFastHash, equal> set_type;
    #else
        struct less;
        typedef std::set<QString*, less> set_type;
    #endif

    class Data;
    friend class Data;

    void ref( Data* );
    void deref( Data* );

    static set_type s_store;

    Data *m_string;
};

#endif
