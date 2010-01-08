/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
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

#include "Mp3tunesMeta.h"

#include "Amarok.h"

#include <KIcon>
#include <KLocale>

#include <QAction>
using namespace Meta;

Mp3TunesTrack::Mp3TunesTrack( const QString& title )
    : ServiceTrack( title )
    , m_filetype()
{
}

QString Mp3TunesTrack::type() const
{
    return "mp3";
}

void  Mp3TunesTrack::setType( const QString &type )
{
    m_filetype = type;
}

QString Mp3TunesTrack::sourceName() { return "MP3tunes.com"; }
QString Mp3TunesTrack::sourceDescription() { return i18n( "Online music locker where you can safely store and access your music: http://mp3tunes.com" ); }
QPixmap Mp3TunesTrack::emblem()  { return  KStandardDirs::locate( "data", "amarok/images/emblem-mp3tunes.png" );  }

//// Mp3TunesAlbum ////

Mp3TunesAlbum::Mp3TunesAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
{
}

Mp3TunesAlbum::Mp3TunesAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
{
}

Mp3TunesAlbum::~ Mp3TunesAlbum()
{
}

void Mp3TunesAlbum::setCoverUrl( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString Mp3TunesAlbum::coverUrl( ) const
{
    return m_coverURL;
}

QList< QAction * > Meta::Mp3TunesAlbum::customActions()
{
    DEBUG_BLOCK
    QList< QAction * > actions;
    //QAction * action = new QAction( KIcon("get-hot-new-stuff-amarok" ), i18n( "&Download" ), 0 );

    //TODO connect some slot to the action, also, give the damn action a parent please
    //actions.append( action );
    return actions;
}














