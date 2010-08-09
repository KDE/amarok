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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace Transcoding
{

Property
Property::Numeric( const QByteArray name,
                   const QString &prettyName,
                   const QString &description,
                   int min,
                   int max,
                   int defaultValue )
{
    return Property( name, prettyName, description, NUMERIC, defaultValue, min, max, QStringList(), QStringList(), QString() );
}

Property
Property::String( const QByteArray name,
                  const QString &prettyName,
                  const QString &description,
                  const QString &defaultText )
{
    return Property( name, prettyName, description, TEXT, 0, 0, 0, QStringList(), QStringList(), defaultText );
}

Property
Property::List( const QByteArray name,
                const QString &prettyName,
                const QString &description,
                const QStringList &valuesList,
                const QStringList &prettyValuesList,
                int defaultIndex )
{
    return Property( name, prettyName, description, LIST, defaultIndex, 0, 0, valuesList, prettyValuesList, QString() );
}

Property::Property( const QByteArray name,
                    const QString &prettyName,
                    const QString &description,
                    Type type,
                    int defaultNumber,
                    int min,
                    int max,
                    const QStringList &values,
                    const QStringList &prettyValues,
                    const QString &defaultText )
    : m_name( name )
    , m_prettyName( prettyName )
    , m_description( description )
    , m_type( type )
    , m_defaultNumber( defaultNumber )
    , m_min( min )
    , m_max( max )
    , m_values( values )
    , m_prettyValues( prettyValues )
    , m_defaultString( defaultText )
{}

} //namespace Transcoding
