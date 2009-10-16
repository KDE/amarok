/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_STREAM_P_H
#define AMAROK_STREAM_P_H

#include "Debug.h"
#include "EngineController.h"
#include "EngineObserver.h"
#include "Meta.h"
#include "MetaConstants.h"

#include <QList>
#include <QObject>
#include <QPixmap>

using namespace MetaStream;

class MetaStream::Track::Private : public QObject, public EngineObserver
{
    Q_OBJECT
    public:
        Private( Track *t )
            : EngineObserver( The::engineController() )
            , track( t )
        {}

        void notify() const
        {
            DEBUG_BLOCK

            foreach( Meta::Observer *observer, observers )
            {
                debug() << "Notifying observer: " << observer;
                observer->metadataChanged( Meta::TrackPtr( const_cast<MetaStream::Track*>( track ) ) );
            }
        }

        void engineNewMetaData( const QHash<qint64, QString> &metaData, bool trackChanged )
        {
            Q_UNUSED( trackChanged )

            if( metaData.value( Meta::valUrl ) == url.url() )
            {
                DEBUG_BLOCK
                debug() << "Applying new Metadata.";

                if( metaData.contains( Meta::valArtist ) )
                    artist = metaData.value( Meta::valArtist );
                if( metaData.contains( Meta::valTitle ) )
                    title = metaData.value( Meta::valTitle );
                if( metaData.contains( Meta::valAlbum ) )
                    album = metaData.value( Meta::valAlbum );

                // Special demangling of artist/title for Shoutcast streams, which usually have "Artist - Title" in the title tag:
                if( artist.isEmpty() && title.contains( " - " ) ) {
                    const QStringList artist_title = title.split( " - " );
                    if( artist_title.size() >= 2 ) {
                        artist = artist_title[0];
                        title  = title.remove( 0, artist.length() + 3 );
                    }
                }

                notify();
            }
        }

        void engineStateChanged( Phonon::State state, Phonon::State oldState )
        {
            Q_UNUSED( oldState )

            if ( state ==  Phonon::PlayingState ) {
                Meta::TrackPtr track = The::engineController()->currentTrack();

                if ( track && track->playableUrl().url() == url.url() )
                { 
                    if( track->artist() )
                        artist = track->artist()->name();
                    title = track->name();
                    if( track->album() )
                        album = track->album()->name();

                        // Special demangling of artist/title for Shoutcast streams, which usually have "Artist - Title" in the title tag:
                    if( !track->artist() && title.contains( " - " ) ) {
                        const QStringList artist_title = title.split( " - " );
                        if( artist_title.size() >= 2 ) {
                            artist = artist_title[0];
                            title  = title.remove( 0, artist.length() + 3 );
                        }
                    }

                    notify();
                }
            }
        }

    public:
        QSet<Meta::Observer*> observers;
        KUrl url;
        QString title;
        QString artist;
        QString album;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

    private:
        Track *track;
};


// internal helper classes

class StreamArtist : public Meta::Artist
{
    public:
        StreamArtist( MetaStream::Track::Private *dptr )
            : Meta::Artist()
            , d( dptr )
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
            if( d )
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

        MetaStream::Track::Private * const d;
};

class StreamAlbum : public Meta::Album
{
public:
    StreamAlbum( MetaStream::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
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
        if( d )
            return d->album;
        return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    void setImage( const QPixmap &pixmap )
    {
        m_cover = pixmap;
    }

    QPixmap image( int size )
    {
        if ( m_cover.isNull() )
            return Meta::Album::image( size );

        //only cache during session
        if ( m_coverSizeMap.contains( size ) )
            return m_coverSizeMap.value( size );

        QPixmap scaled = m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

        m_coverSizeMap.insert( size, scaled );
        return scaled;
    }
 
    bool operator==( const Meta::Album &other ) const {
        return name() == other.name();
    }

    MetaStream::Track::Private * const d;
    QMap <int, QPixmap> m_coverSizeMap;
    QPixmap m_cover;
};


#endif

