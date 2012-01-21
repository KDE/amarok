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
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TranscodingConfiguration.h"

#include "TranscodingProperty.h"
#include "TranscodingFormat.h"
#include "TranscodingController.h"
#include "core/support/Components.h"

#include <KLocalizedString>

using namespace Transcoding;

QMap<Encoder, QString> Configuration::s_encoderNames;

Configuration::Configuration( Encoder encoder )
    : m_encoder( encoder )
{
}

void
Configuration::addProperty( QByteArray name, QVariant value )
{
    m_values.insert( name, value );
}

QVariant
Configuration::property( QByteArray name ) const
{
    return m_values.value( name );
}

Configuration
Configuration::fromConfigGroup( const KConfigGroup &serialized )
{
    Configuration invalid( INVALID );
    Controller *controller = Amarok::Components::transcodingController();

    QString encoderName = serialized.readEntry( "Encoder", QString() );
    Encoder encoder = encoderNames().key( encoderName, INVALID );
    Configuration ret( encoder );
    if( !ret.isValid() )
        return invalid;

    Format *format = controller->format( ret.encoder() );
    foreach( Property property, format->propertyList() )
    {
        QString key = QString( "Parameter ").append( property.name() );
        QVariant value = serialized.readEntry( key, QString() /* does not work with QVariant() */ );
        if( !value.isValid() )
            return invalid;
        if( !value.convert( property.variantType() ) )
            return invalid;
        switch( property.type() )
        {
            case Property::TRADEOFF:
                if( value.toInt() < property.min() )
                    return invalid;
                if( value.toInt() > property.max() )
                    return invalid;
                break;
        }
        ret.m_values.insert( property.name(), value );
    }
    return ret;
}

void
Configuration::saveToConfigGroup( KConfigGroup &group ) const
{
    group.deleteGroup(); // remove all keys
    Q_ASSERT( encoderNames().contains( m_encoder ) );
    QString encoderName = encoderNames().value( m_encoder );
    group.writeEntry( QLatin1String( "Encoder" ), encoderName );
    QMapIterator<QByteArray, QVariant> it( m_values );
    while( it.hasNext() )
    {
        it.next();
        group.writeEntry( QString( "Parameter " ).append( it.key() ), it.value() );
    }
}

QString
Configuration::prettyName() const
{
    if( !isValid() )
        return i18n( "Invalid encoder" );
    if( isJustCopy() )
        return i18n( "Just copy or move" );

    Format *format = Amarok::Components::transcodingController()->format( m_encoder );
    if( format->propertyList().isEmpty() )
        return format->prettyName();

    // we take only the first property into account, assume it's the most significant
    const Property &property = format->propertyList().first();
    QByteArray name = property.name();
    Q_ASSERT( m_values.contains( name ) );
    Q_ASSERT( m_values.value( name ).type() == property.variantType() );
    QString propertyText;
    switch( property.type() )
    {
        case Property::TRADEOFF:
        {
            int currValue = m_values.value( name ).toInt();
            int min = property.min();
            int max = property.max();
            Q_ASSERT( min <= currValue && currValue <= max );
            if( property.valueLabels().size() == ( property.max() - property.min() + 1 ) )
                propertyText = property.valueLabels().at( currValue - min );
            else
                propertyText = i18nc( "%1 example: 'Compression level' %2 example: '5'",
                                      "%1 %2", property.prettyName(), currValue );
            break;
        }
    }
    return i18nc( "%1 example: 'MP3' %2 example: 'VBR 175kb/s'", "%1, %2",
                  format->prettyName(), propertyText );
}

const QMap<Encoder, QString>&
Configuration::encoderNames()
{
    if( !s_encoderNames.isEmpty() )
        return s_encoderNames;

    s_encoderNames.insert( INVALID, QLatin1String( "INVALID" ) );
    s_encoderNames.insert( JUST_COPY, QLatin1String( "JUST_COPY" ) );
    s_encoderNames.insert( AAC, QLatin1String( "AAC" ) );
    s_encoderNames.insert( ALAC, QLatin1String( "ALAC" ) );
    s_encoderNames.insert( FLAC, QLatin1String( "FLAC" ) );
    s_encoderNames.insert( MP3, QLatin1String( "MP3" ) );
    s_encoderNames.insert( VORBIS, QLatin1String( "VORBIS" ) );
    s_encoderNames.insert( WMA2, QLatin1String( "WMA2" ) );
    return s_encoderNames;
}
