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

#include <QDebug>
#include <QExplicitlySharedDataPointer>


// Simple wrapper to make QExplicitlySharedDataPointer more convenient
template<class T>
class AmarokSharedPointer : public QExplicitlySharedDataPointer<T>
{
public:
    AmarokSharedPointer() = default;
    ~AmarokSharedPointer() = default;
    explicit AmarokSharedPointer(T *data) noexcept: QExplicitlySharedDataPointer<T>(data){};
    AmarokSharedPointer(const AmarokSharedPointer &) = default;
    AmarokSharedPointer(AmarokSharedPointer &&) = default;

    AmarokSharedPointer &operator=(const AmarokSharedPointer &) = default;
    AmarokSharedPointer &operator=(AmarokSharedPointer &&) = default;
    AmarokSharedPointer &operator=(T *t) noexcept { this->QExplicitlySharedDataPointer<T>::operator=(t); return *this;}

    explicit inline operator bool() const noexcept { return this->QExplicitlySharedDataPointer<T>::operator bool();}
    inline bool isNull() const noexcept { return this->QExplicitlySharedDataPointer<T>::operator!(); }
    inline int count() const noexcept { return this->constData() ? this->constData()->ref.loadRelaxed() : 0 ;}
    template <class U>
    static AmarokSharedPointer<T> staticCast(const AmarokSharedPointer<U>& o)
    {
        return AmarokSharedPointer<T>(static_cast<T*>(o.data()));
    }
    template <class U>
    static AmarokSharedPointer<T> dynamicCast(const AmarokSharedPointer<U>& o)
    {
        return AmarokSharedPointer<T>(dynamic_cast<T*>(o.data()));
    }
};

template<typename T>
QDebug operator<<(QDebug d, const AmarokSharedPointer<T> &ptr){
    d << (uintptr_t) ptr.data();
    return d;
}


#endif // AMAROKSHAREDPOINTER_H
