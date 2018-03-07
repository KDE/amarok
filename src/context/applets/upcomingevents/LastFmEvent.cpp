/****************************************************************************************
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
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

#include "LastFmEvent.h"


LastFmEvent::LastFmEvent()
{
    static bool metaTypeRegistered = false;
    if (!metaTypeRegistered)
    {
        qRegisterMetaType<LastFmEvent>("LastFmEvent");
        metaTypeRegistered = true;
    }
}

LastFmEvent::LastFmEvent( const LastFmEvent& event)
    : QSharedData( event )
    , m_attendance( event.m_attendance )
    , m_cancelled( event.m_cancelled )
    , m_date( event.m_date )
    , m_url( event.m_url )
    , m_imageUrls( event.m_imageUrls )
    , m_description( event.m_description )
    , m_name( event.m_name )
    , m_headliner( event.m_headliner )
    , m_participants( event.m_participants )
    , m_tags( event.m_tags )
    , m_venue( event.m_venue )
{}

LastFmEvent::~LastFmEvent() {}

QStringList LastFmEvent::artists() const
{
    return QStringList() << m_headliner << m_participants;
}

KDateTime LastFmEvent::date() const
{
    return m_date;
}

QString LastFmEvent::name() const
{
    return m_name;
}

QUrl LastFmEvent::url() const
{
    return m_url;
}

void LastFmEvent::setDate( const KDateTime &date )
{
    m_date = date;
}

void LastFmEvent::setName( const QString &name )
{
    m_name = name;
}

void LastFmEvent::setUrl( const QUrl &url )
{
    m_url = url;
}

QString
LastFmEvent::imageSizeToString( ImageSize size )
{
    switch( size )
    {
    default:
    case Small:      return QString("small");
    case Medium:     return QString("medium");
    case Large:      return QString("large");
    case ExtraLarge: return QString("extralarge");
    case Mega:       return QString("maga");
    }
}

LastFmEvent::ImageSize
LastFmEvent::stringToImageSize( const QString &string )
{
    if( string == "small" )
        return Small;
    if( string == "medium" )
        return Medium;
    if( string == "large" )
        return Large;
    if( string == "extralarge" )
        return ExtraLarge;
    if( string == "mega" )
        return Mega;
    return Small;
}

LastFmLocation::LastFmLocation()
{}

LastFmLocation::~LastFmLocation()
{}

LastFmLocation::LastFmLocation( const LastFmLocation &cpy )
    : QSharedData( cpy )
    , city( cpy.city )
    , country( cpy.country )
    , street( cpy.street )
    , postalCode( cpy.postalCode )
    , latitude( cpy.latitude )
    , longitude( cpy.longitude )
{}

LastFmVenue::LastFmVenue()
{}

LastFmVenue::~LastFmVenue()
{}

LastFmVenue::LastFmVenue( const LastFmVenue &cpy )
    : QSharedData( cpy )
    , id( cpy.id )
    , name( cpy.name )
    , url( cpy.url )
    , website( cpy.website )
    , phoneNumber( cpy.phoneNumber )
    , imageUrls( cpy.imageUrls )
    , location( cpy.location )
{}
