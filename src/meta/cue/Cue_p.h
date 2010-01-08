/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef AMAROK_CUE_P_H
#define AMAROK_CUE_P_H

#include "Debug.h"
#include "EngineController.h"
#include "EngineObserver.h"
#include "Meta.h"
#include "MetaConstants.h"

#include <QList>
#include <QObject>

using namespace MetaCue;

class MetaCue::Track::Private : public QObject//, public EngineObserver
{
    Q_OBJECT
public:
    Private ( Track *t )
    //      : EngineObserver( The::engineController() )
            : title()
            , artist()
            , album()
            , tracknumber( 0 )
            , length( 0 )
            , track ( t )
    {}

    QSet<Meta::Observer*> observers;
    KUrl url;
    QString title;
    QString artist;
    QString album;
    int tracknumber;
    qint64 length;

    Meta::ArtistPtr artistPtr;
    Meta::AlbumPtr albumPtr;
    Meta::GenrePtr genrePtr;
    Meta::ComposerPtr composerPtr;
    Meta::YearPtr yearPtr;

//    private:
    Track *track;
};


// internal helper classes

class CueArtist : public Meta::Artist
{
public:
    CueArtist ( MetaCue::Track::Private *dptr )
            : Meta::Artist()
            , d ( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        if ( d )
            return d->artist;
        return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Artist &other ) const {
        return name() == other.name();
    }

    MetaCue::Track::Private * const d;
};

class CueAlbum : public Meta::Album
{
public:
    CueAlbum ( MetaCue::Track::Private *dptr )
            : Meta::Album()
            , d ( dptr )
    {}

    bool isCompilation() const
    {
        return false;
    }

    bool hasAlbumArtist() const
    {
        return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if ( d )
            return d->album;
        return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image ( int size )
    {
        return Meta::Album::image ( size );
    }

    bool operator==( const Meta::Album &other ) const {
        return name() == other.name();
    }

    MetaCue::Track::Private * const d;
};


#endif
