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

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"
#include "covermanager/CoverCache.h"

#include <QObject>

using namespace MetaStream;

class MetaStream::Track::Private : public QObject
{
    Q_OBJECT

    public:
        Private( Track *t )
            : trackNumber( 0 )
            , length( 0 )
            , track( t )
        {
            EngineController *engine = The::engineController();
            if( !engine )
                return; // engine might not be available during tests, silence the warning

            // force a direct connection or slot might not be called because of thread
            // affinity. (see BUG 300334)
            connect( engine, &EngineController::currentMetadataChanged,
                     this, &Private::currentMetadataChanged,
                     Qt::DirectConnection );
        }

    public Q_SLOTS:
        void currentMetadataChanged( const QVariantMap &metaData )
        {
            const QUrl metaDataUrl = metaData.value( Meta::Field::URL ).toUrl();
            if( metaDataUrl == url )
            {
                // keep synchronized to EngineController::slotMetaDataChanged()
                if( metaData.contains( Meta::Field::ARTIST ) )
                    artist = metaData.value( Meta::Field::ARTIST ).toString();
                if( metaData.contains( Meta::Field::TITLE ) )
                    title = metaData.value( Meta::Field::TITLE ).toString();
                if( metaData.contains( Meta::Field::ALBUM ) )
                    album = metaData.value( Meta::Field::ALBUM ).toString();
                if( metaData.contains( Meta::Field::GENRE ) )
                    genre = metaData.value( Meta::Field::GENRE ).toString();
                if( metaData.contains( Meta::Field::TRACKNUMBER ) )
                    trackNumber = metaData.value( Meta::Field::TRACKNUMBER ).toInt();
                if( metaData.contains( Meta::Field::COMMENT ) )
                    comment = metaData.value( Meta::Field::COMMENT ).toString();
                if( metaData.contains( Meta::Field::LENGTH ) )
                    length = metaData.value( Meta::Field::LENGTH ).value<qint64>();

                //TODO: move special handling to subclass or using some configurable XSPF
                // Special demangling of artist/title for Shoutcast streams, which usually
                // have "Artist - Title" in the title tag:
                if( artist.isEmpty() && title.contains( QLatin1String(" - ") ) )
                {
                    const QStringList artist_title = title.split( QStringLiteral(" - ") );
                    if( artist_title.size() >= 2 )
                    {
                        artist = artist_title[0];
                        title  = title.remove( 0, artist.length() + 3 );
                    }
                }

                track->notifyObservers();
            }
        }

    public:
        QUrl url;
        QString title;
        QString artist;
        QString album;
        QString genre;
        int trackNumber;
        QString comment;
        qint64 length;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

    private:
        Track *track;
};


// internal helper classes

class StreamArtist : public Meta::DefaultArtist
{
    public:
        explicit StreamArtist( MetaStream::Track::Private *dptr )
            : DefaultArtist()
            , d( dptr )
            {}

        QString name() const override
        {
            if( d && !d->artist.isEmpty() )
                return d->artist;
            return DefaultArtist::name();
        }

        MetaStream::Track::Private * const d;
};

class StreamAlbum : public Meta::DefaultAlbum
{
public:
    explicit StreamAlbum( MetaStream::Track::Private *dptr )
        : DefaultAlbum()
        , d( dptr )
    {}

    ~StreamAlbum()
    {
        CoverCache::invalidateAlbum( this );
    }

    bool hasAlbumArtist() const override
    {
        return false;
    }

    QString name() const override
    {
        if( d && !d->album.isEmpty() )
            return d->album;
        return DefaultAlbum::name();
    }

    bool hasImage( int size ) const override
    {
        if( m_cover.isNull() )
            return Meta::Album::hasImage( size );
        else
            return true;
    }

    QImage image( int size ) const override
    {
        if( m_cover.isNull() )
            return Meta::Album::image( size );
        else
            return m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    }

    void setImage( const QImage &image ) override
    {
        m_cover = image;
        CoverCache::invalidateAlbum( this );
    }

    MetaStream::Track::Private * const d;
    QImage m_cover;
};

class StreamGenre : public Meta::DefaultGenre
{
public:
    explicit StreamGenre( MetaStream::Track::Private *dptr )
        : DefaultGenre()
        , d( dptr )
    {}

    QString name() const override
    {
        if( d && !d->genre.isEmpty() )
            return d->genre;
        return DefaultGenre::name();
    }

    MetaStream::Track::Private * const d;
};

#endif
