/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_COMVARIANT_H
#define AMAROK_COMVARIANT_H

#include <objbase.h>

#include <algorithm>

#include <QString>

// basic smart variant wrapper
class ComVariant
{
public:
    ComVariant()
    {
        VariantInit( &m_variant );
    }

    ComVariant(const ComVariant &other)
    {
        VariantCopy( &m_variant, &other.m_variant );
    }

    ~ComVariant()
    {
        VariantClear( &m_variant );
    }

    // implement assignemnt in terms of copy construct
    ComVariant &operator=(ComVariant other)
    {
        std::swap(m_variant, other.m_variant);
        return *this;
    }

    // taking the address is usually used for assignment, so we clear any existing variant
    VARIANT *operator&()
    {
        VariantClear( &m_variant );
        return &m_variant;
    }

    // convert variant to a utf16 string and then to a QString
    // returns an empty string on failure
    QString AsString()
    {
        ComVariant result;
        if( SUCCEEDED( VariantChangeType( &result, &m_variant, 0, VT_BSTR ) ) )
        {
            return QString::fromUtf16( reinterpret_cast<ushort *>( m_variant.bstrVal ) );
        }
        return "";
    }

private:
    mutable VARIANT m_variant;
};

#endif // AMAROK_COMVARIANT_H
