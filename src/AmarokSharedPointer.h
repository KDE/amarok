/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef AMAROKSHAREDPOINTER_H
#define AMAROKSHAREDPOINTER_H

#include <QObject>
#include <QSharedData>

template<class T>
class AmarokSharedPointer
{
public:
    inline AmarokSharedPointer() : d(Q_NULLPTR) {}
    inline explicit AmarokSharedPointer(T *t) : d(t) { if (d) d->ref.ref(); }
    inline AmarokSharedPointer(const AmarokSharedPointer& other) : d(other.d) { if (d) d->ref.ref(); }
    inline ~AmarokSharedPointer() { if (d && !d->ref.deref()) delete d; }

    AmarokSharedPointer& operator=(const AmarokSharedPointer& other)
    {
        if (d != other.d)
        {
            if (d && !d->ref.deref())
                delete d;
            d = other.d;
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    AmarokSharedPointer& operator=(T *t)
    {
        if (d != t)
        {
            if (d && !d->ref.deref())
                delete d;
            d = t;
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    inline bool operator==(const AmarokSharedPointer& other) const { return d == other.d; }
    inline bool operator!=(const AmarokSharedPointer& other) const { return d != other.d; }
    inline bool operator<(const AmarokSharedPointer& other) const { return std::less<const T*>()(d, other.d); }
    inline const T& operator*() const { Q_ASSERT(d); return *d; }
    inline T& operator*() { Q_ASSERT(d); return *d; }
    inline const T* operator->() const { Q_ASSERT(d); return d; }
    inline T* operator->() { Q_ASSERT(d); return d; }
    inline operator bool() const { return ( d != Q_NULLPTR ); }

    inline bool isNull() const { return d == Q_NULLPTR; }
    inline int count() const { return d ? d->ref.load() : 0; }
    inline T* data() const { return d; }
    inline void clear() { if (d && !d->ref.deref()) delete d; d = Q_NULLPTR; }

    template <class U>
    static AmarokSharedPointer<T> staticCast(const AmarokSharedPointer<U>& o)
    {
        return AmarokSharedPointer<T>(static_cast<T *>(o.data()));
    }
    template <class U>
    static AmarokSharedPointer<T> dynamicCast(const AmarokSharedPointer<U>& o)
    {
        return AmarokSharedPointer<T>(dynamic_cast<T *>(o.data()));
    }
    template <class U>
    static AmarokSharedPointer<T> qobjectCast(const AmarokSharedPointer<U>& o)
    {
        return AmarokSharedPointer<T>(qobject_cast<T *>(o.data()));
    }

private:
    T *d;
};

template<class T>
inline uint qHash( const AmarokSharedPointer<T> &p, uint seed ) { return qHash( p.data(), seed ); }

#endif // AMAROKSHAREDPOINTER_H
