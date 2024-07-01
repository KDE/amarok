/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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
 * You should have received a copy of the GNU General Public License aint with          *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TranscodingProperty.h"
#include "core/support/Debug.h"

namespace Transcoding
{

Property
Property::Tradeoff( const QByteArray &name,
                    const QString &prettyName,
                    const QString &description,
                    const QString &leftText,
                    const QString &rightText,
                    int min,
                    int max,
                    int defaultValue )
{
    if( max < min )
        qSwap( min, max );
    return Property( name, prettyName, description, TRADEOFF,
                     defaultValue, min, max, QStringList(),
                     QStringList() << leftText << rightText );
}

Property
Property::Tradeoff(const QByteArray &name,
                    const QString &prettyName,
                    const QString &description,
                    const QString &leftText,
                    const QString &rightText,
                    const QStringList &valueLabels,
                    int defaultValue )
{
    return Property( name, prettyName, description, TRADEOFF,
                     defaultValue, 0, valueLabels.isEmpty() ? 0 : valueLabels.size() - 1, valueLabels,
                     QStringList() << leftText << rightText );
}

QVariant::Type
Property::variantType() const
{
    switch( m_type )
    {
        case TRADEOFF:
            return QVariant::Int;
    }
    return QVariant::Invalid;
}

Property::Property( const QByteArray &name,
                    const QString &prettyName,
                    const QString &description,
                    Type type,
                    const QVariant &defaultValue,
                    int min,
                    int max,
                    const QStringList &valueLabels,
                    const QStringList &endLabels )
    : m_name( name )
    , m_prettyName( prettyName )
    , m_description( description )
    , m_type( type )
    , m_defaultValue( defaultValue )
    , m_min( min )
    , m_max( max )
    , m_valueLabels( valueLabels )
    , m_endLabels( endLabels )
{}

} //namespace Transcoding
