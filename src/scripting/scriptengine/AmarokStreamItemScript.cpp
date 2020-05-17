/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "AmarokStreamItemScript.h"

#include "core/support/Debug.h"

#include <QJSEngine>

//using namespace AmarokScript;

StreamItem::StreamItem( QJSEngine *engine )
    : QObject( engine )
    , m_year( 0 )
{
    QJSValue scriptObj = engine->newQObject( this );
    //engine->setDefaultPrototype( qMetaTypeId<StreamItem*>(), QJSValue() );
    scriptObj.setPrototype( QJSValue() );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("StreamItem"), scriptObj );
}

QString
StreamItem::itemName() const
{
    return m_name;
}

QString
StreamItem::infoHtml() const
{
    return m_infoHtml;
}

QString
StreamItem::playableUrl() const
{
    return m_playableUrl;
}

QString
StreamItem::callbackData() const
{
    return m_callbackData;
}

int
StreamItem::level() const
{
    return m_level;
}


QString
StreamItem::album() const
{
    return m_album;
}

QString
StreamItem::artist() const
{
    return m_artist;
}

QString
StreamItem::genre() const
{
    return m_genre;
}

QString
StreamItem::composer() const
{
    return m_composer;
}

int
StreamItem::year() const
{
    return m_year;
}

QString
StreamItem::coverUrl()
{
    return m_coverUrl;
}


void
StreamItem::setItemName( const QString &name )
{
    m_name = name;
}

void
StreamItem::setInfoHtml( const QString &infoHtml )
{
    m_infoHtml = infoHtml;
}

void
StreamItem::setPlayableUrl( const QString &playableUrl )
{
    m_playableUrl = playableUrl;
}

void
StreamItem::setCallbackData( const QString &callbackData )
{
    m_callbackData = callbackData;
}

void
StreamItem::setLevel( int level )
{
    m_level = level;
}

void
StreamItem::setAlbum( const QString &album )
{
    m_album = album;
}

void
StreamItem::setArtist( const QString &artist )
{
    m_artist = artist;
}

void
StreamItem::setGenre( const QString &genre )
{
    m_genre = genre;
}

void
StreamItem::setComposer( const QString &composer )
{
    m_composer = composer;
}

void
StreamItem::setYear( int year )
{
    m_year = year;
}

void
StreamItem::setCoverUrl( const QString &url )
{
    m_coverUrl = url;
}
