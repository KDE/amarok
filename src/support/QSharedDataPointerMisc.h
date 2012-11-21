/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef QSHAREDDATAPOINTERMISC
#define QSHAREDDATAPOINTERMISC

#include <QExplicitlySharedDataPointer>
#include <QSharedDataPointer>

/**
 * @brief QSharedDataPointerMisc.h
 *
 * Qt doesn't provide operator<, operator== and qHash() for
 * Q{,Explicitly}SharedDataPointer<T>.
 *
 * Define those so that you can use Q{,Explicitly}SharedDataPointers in QMaps,
 * QHashes etc.
 *
 * QMap<Key,T> needs operator< for key, however
 * QExplicitlySharedDataPointer<T> silently coerces to bool, which gives really
 * hard to find bugs, because it makes QMap with it useless.
 */

/**
 * Define trivial comparison operator for QSharedDataPointer<T> so that it doesn't
 * coerce to bool.
 */
template<class T>
Q_INLINE_TEMPLATE bool operator<( const QSharedDataPointer<T> &l, const QSharedDataPointer<T> &r )
{
    return l.data() < r.data();
}

/**
 * Define trivial comparison operator for QExplicitlySharedDataPointer<T> so
 * that it doesn't coerce to bool.
 */
template<class T>
Q_INLINE_TEMPLATE bool operator<( const QExplicitlySharedDataPointer<T> &l, const QExplicitlySharedDataPointer<T> &r )
{
    return l.data() < r.data();
}

/**
 * Define trivial equivalence operator for QSharedDataPointer<T> so that it can
 * be used in Qt containers.
 */
template<class T>
Q_INLINE_TEMPLATE bool operator==( const QSharedDataPointer<T> &l, const QSharedDataPointer<T> &r )
{
    return l.data() == r.data();
}

/**
 * Define trivial equivalence operator for QExplicitlySharedDataPointer<T> so
 * that it can be used in Qt containers.
 */
template<class T>
Q_INLINE_TEMPLATE bool operator==( const QExplicitlySharedDataPointer<T> &l, const QExplicitlySharedDataPointer<T> &r )
{
    return l.data() == r.data();
}

/**
 * Define trivial qHash() for QSharedDataPointer<T> so that it can
 * be used in Qt containers.
 */
template<class T>
Q_INLINE_TEMPLATE uint qHash( const QSharedDataPointer<T> &ptr )
{
    return qHash( ptr.data() );
}

/**
 * Define trivial qHash() for QExplicitlySharedDataPointer<T> so that it can be
 * used in Qt containers.
 */
template<class T>
Q_INLINE_TEMPLATE uint qHash( const QExplicitlySharedDataPointer<T> &ptr )
{
    return qHash( ptr.data() );
}

#endif // QSHAREDDATAPOINTERMISC
