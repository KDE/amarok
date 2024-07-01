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

#include <KConfigGroup>
#include <KLocalizedString>

using namespace Transcoding;

QMap<Encoder, QString> Configuration::s_encoderNames;

Configuration::Configuration( Encoder encoder, TrackSelection trackSelection )
    : m_encoder( encoder )
    , m_trackSelection( trackSelection )
{
}

void
Configuration::addProperty( const QByteArray &name, const QVariant &value )
{
    m_values.insert( name, value );
}

QVariant
Configuration::property( const QByteArray &name ) const
{
    return m_values.value( name );
}

Configuration
Configuration::fromConfigGroup( const KConfigGroup &serialized )
{
    QString encoderName = serialized.readEntry( "Encoder", QString() );
    Encoder encoder = encoderNames().key( encoderName, INVALID );
    TrackSelection trackSelection = TrackSelection( serialized.readEntry( "TrackSelection", int( TranscodeAll ) ) );
    Configuration ret( encoder, trackSelection );
    if( !ret.isValid() )
        return ret; // return ret, so that its trackSelection value may be used

    Controller *controller = Amarok::Components::transcodingController();
    // reset controller to 0 if it doesn't contain encoder to prevent bogus format() call
    if( controller && !controller->allEncoders().contains( ret.encoder() ) )
        controller = nullptr;
    Format *format = controller ? controller->format( ret.encoder() ) : nullptr;

    PropertyList emptyList;
    for( const Property &property : ( format ? format->propertyList() : emptyList ) )
    {
        Configuration invalid( INVALID );
        QString key = QStringLiteral( "Parameter ").append( QLatin1String( property.name() ) );
        QVariant value = serialized.readEntry( key, QString() /* does not work with QVariant() */ );

        if( !value.isValid() )
            return invalid;
        if( !value.canConvert( property.variantType() ) )
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
    group.writeEntry( QStringLiteral( "Encoder" ), encoderName );
    group.writeEntry( QStringLiteral( "TrackSelection" ), int( m_trackSelection ) );
    QMapIterator<QByteArray, QVariant> it( m_values );
    while( it.hasNext() )
    {
        it.next();
        group.writeEntry( QStringLiteral( "Parameter " ).append( QLatin1String( it.key() ) ), it.value() );
    }
}

QString
Configuration::prettyName() const
{
    if( !isValid() )
        return i18n( "Invalid" );
    if( isJustCopy() )
        return i18n( "Just Copy" );

    Format *format = Amarok::Components::transcodingController()->format( m_encoder );
    if( format->propertyList().isEmpty() )
        return formatPrettyPrefix();

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
            const int currValue = m_values.value( name ).toInt();
            const int min = property.min();
            const int max = property.max();
            Q_ASSERT( min <= currValue && currValue <= max );
            if( property.valueLabels().size() == ( max - min + 1 ) )
                propertyText = property.valueLabels().at( currValue - min );
            else
                propertyText = i18nc( "%1 example: 'Compression level' %2 example: '5'",
                                      "%1 %2", property.prettyName(), currValue );
            break;
        }
    }

    return i18nc( "Displayed next to the \"Transcode:\" label. "
                  "%1 example: 'All Tracks to MP3' %2 example: 'VBR 175kb/s'",
                  "%1, %2", formatPrettyPrefix(), propertyText );

}

const QMap<Encoder, QString>&
Configuration::encoderNames()
{
    if( !s_encoderNames.isEmpty() )
        return s_encoderNames;

    s_encoderNames.insert( INVALID, QStringLiteral( "INVALID" ) );
    s_encoderNames.insert( JUST_COPY, QStringLiteral( "JUST_COPY" ) );
    s_encoderNames.insert( AAC, QStringLiteral( "AAC" ) );
    s_encoderNames.insert( ALAC, QStringLiteral( "ALAC" ) );
    s_encoderNames.insert( FLAC, QStringLiteral( "FLAC" ) );
    s_encoderNames.insert( MP3, QStringLiteral( "MP3" ) );
    s_encoderNames.insert( OPUS, QStringLiteral( "OPUS" ) );
    s_encoderNames.insert( VORBIS, QStringLiteral( "VORBIS" ) );
    s_encoderNames.insert( WMA2, QStringLiteral( "WMA2" ) );
    return s_encoderNames;
}

bool
Configuration::isJustCopy( const Meta::TrackPtr &srcTrack,
                           const QStringList &playableFileTypes ) const
{
    if( m_encoder == INVALID || m_encoder == JUST_COPY )
        return true;

    if( !srcTrack )
        return false;

    switch( m_trackSelection )
    {
        case TranscodeUnlessSameType:
        {
            QString srcExt = srcTrack->type();
            QString destExt = Amarok::Components::transcodingController()->format( m_encoder )->fileExtension();
            if( destExt.compare( srcExt, Qt::CaseInsensitive )  == 0 ) //if source and destination file formats are the same
                return true;
            else
                return false;
        }
        case TranscodeOnlyIfNeeded:
        {
            QString srcExt = srcTrack->type();
            //check if the file is already in a format supported by the target collection
            if( playableFileTypes.isEmpty() || playableFileTypes.contains( srcExt ) )
                return true; // if isEmpty(), assume all formats compatible
            else
                return false;
        }
        case TranscodeAll:
            return false;
    }
    return false; // shouldn't really get here
}

QString
Configuration::formatPrettyPrefix() const
{
    Format *format = Amarok::Components::transcodingController()->format( m_encoder );

    switch( m_trackSelection )
    {
        case TranscodeAll:
            return i18nc( "Displayed next to the \"Transcode:\" label. "
                          "%1 example: 'MP3'",
                          "All Tracks to %1", format->prettyName() );
        case TranscodeUnlessSameType:
            return i18nc( "Displayed next to the \"Transcode:\" label. "
                          "%1 example: 'MP3'",
                          "Non-%1 Tracks to %1", format->prettyName() );
        case TranscodeOnlyIfNeeded:
            return i18nc( "Displayed next to the \"Transcode:\" label. "
                          "%1 example: 'MP3'",
                          "When Needed to %1", format->prettyName() );
    }
    return format->prettyName();
}

void
Configuration::setTrackSelection( TrackSelection trackSelection )
{
    m_trackSelection = trackSelection;
}

bool
Configuration::operator!=( const Configuration &other ) const
{
    return m_encoder != other.m_encoder ||
            m_trackSelection != other.m_trackSelection;
}
