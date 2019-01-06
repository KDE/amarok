/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef SERVICEMETABASE_H
#define SERVICEMETABASE_H

#include "amarok_export.h"
#include "amarokurls/BookmarkMetaActions.h"
#include "core/support/Debug.h"
#include "InfoParserBase.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "ServiceCapabilities.h"

#include <QAction>
#include <QPointer>
#include <QStringList>

namespace Amarok
{
    class TrackProvider;
}

class AMAROK_EXPORT ServiceMetaFactory
{
    public:
        explicit ServiceMetaFactory( const QString &dbPrefix );
        virtual ~ServiceMetaFactory() {}

        QString tablePrefix();

        virtual int getTrackSqlRowCount();
        virtual QString getTrackSqlRows();
        virtual Meta::TrackPtr createTrack( const QStringList &rows );

        virtual int getAlbumSqlRowCount();
        virtual QString getAlbumSqlRows();
        virtual Meta::AlbumPtr createAlbum( const QStringList &rows );

        virtual int getArtistSqlRowCount();
        virtual QString getArtistSqlRows();
        virtual Meta::ArtistPtr createArtist( const QStringList &rows );

        virtual int getGenreSqlRowCount();
        virtual QString getGenreSqlRows();
        virtual Meta::GenrePtr createGenre( const QStringList &rows );

    private:
        QString m_dbTablePrefix;
};

class AMAROK_EXPORT ServiceDisplayInfoProvider
{
    public:
        ServiceDisplayInfoProvider() {}
        virtual ~ServiceDisplayInfoProvider() {}

        virtual void processInfoOf( InfoParserBase * infoParser ) = 0;
};

class AMAROK_EXPORT ActionsProvider
{
    public:
        ActionsProvider() {}
        virtual ~ActionsProvider() {}

        virtual QList< QAction *> actions() { DEBUG_BLOCK return QList< QAction *>(); }
};

class AMAROK_EXPORT SourceInfoProvider
{
    public:
        SourceInfoProvider() {}
        virtual ~SourceInfoProvider() {}

        virtual QString sourceName() { return QString(); }
        virtual QString sourceDescription() { return QString(); }
        virtual QPixmap emblem()  { return QPixmap(); }
        virtual QString scalableEmblem()  { return QString(); }
        virtual bool hasSourceInfo() const { return true; }
};

class AMAROK_EXPORT BookmarkThisProvider : public QObject
{
    Q_OBJECT
    public:

        BookmarkThisProvider() : QObject() {}
        virtual ~BookmarkThisProvider() {}

        virtual bool isBookmarkable() const { return false; }
        virtual QString browserName() const { return QStringLiteral("internet"); }
        virtual QString collectionName() const { return QString(); }
        virtual bool simpleFiltering() const { return true; }
        virtual QAction * bookmarkAction() { return 0; }

    protected:
        QPointer<QAction> m_bookmarkAction;
};


namespace Meta
{
class ServiceTrack;
class ServiceAlbum;
class ServiceArtist;
class ServiceGenre;
class ServiceComposer;
class ServiceYear;

typedef AmarokSharedPointer<ServiceTrack> ServiceTrackPtr;
typedef AmarokSharedPointer<ServiceArtist> ServiceArtistPtr;
typedef AmarokSharedPointer<ServiceAlbum> ServiceAlbumPtr;
typedef AmarokSharedPointer<ServiceGenre> ServiceGenrePtr;
typedef AmarokSharedPointer<ServiceComposer> ServiceComposerPtr;
typedef AmarokSharedPointer<ServiceYear> ServiceYearPtr;

typedef QList<ServiceTrackPtr > ServiceTrackList;
typedef QList<ServiceArtistPtr > ServiceArtistList;
typedef QList<ServiceAlbumPtr > ServiceAlbumList;
typedef QList<ServiceComposerPtr> ServiceComposerList;
typedef QList<ServiceGenrePtr > ServiceGenreList;
typedef QList<ServiceYearPtr > ServiceYearList;

class AMAROK_EXPORT ServiceTrack : public Meta::Track,
                                   public ServiceDisplayInfoProvider,
                                   public ActionsProvider,
                                   public SourceInfoProvider,
                                   public BookmarkThisProvider
{
    public:
        //Give this a displayable name as some services has terrible names for their streams
        //ServiceTrack( const QString & name );
        explicit ServiceTrack( const QString & name );

        //create track based on an sql query result
        explicit ServiceTrack( const QStringList & resultRow );
        virtual ~ServiceTrack();

        QString name() const override;
        virtual QUrl downloadableUrl() const;
        QUrl playableUrl() const override;
        QString uidUrl() const override;
        QString prettyUrl() const override;
        QString notPlayableReason() const override;

        AlbumPtr album() const override;
        virtual void setAlbum( const QString &newAlbum );

        ArtistPtr artist() const override;
        virtual void setArtist( const QString &newArtist );

        GenrePtr genre() const override;
        virtual void setGenre( const QString &newGenre );

        ComposerPtr composer() const override;
        virtual void setComposer( const QString &newComposer );

        YearPtr year() const override;
        virtual void setYear( int newYear );

        virtual void setTitle( const QString &newTitle );

        qreal bpm() const override;

        QString comment() const override;
        virtual void setComment( const QString &newComment );

        virtual QString description() const;
        virtual void setDescription( const QString &newDescription );

        qint64 length() const override;

        int filesize() const override;
        int sampleRate() const override;
        int bitrate() const override;

        int trackNumber() const override;
        virtual void setTrackNumber( int newTrackNumber );

        int discNumber() const override;
        virtual void setDiscNumber( int newDiscNumber );

        QString type() const override;

        void processInfoOf( InfoParserBase * infoParser ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                   ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                   ( type == Capabilities::Capability::BookmarkThis ) ||
                   ( type == Capabilities::Capability::FindInSource && isBookmarkable() );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            else if ( type == Capabilities::Capability::FindInSource && isBookmarkable() )
                return new ServiceFindInSourceCapability( this );
            return 0;
        }

        StatisticsPtr statistics() override;

        //ServiceTrack specific methods

        virtual void setAlbumPtr( Meta::AlbumPtr album );
        void setArtist( Meta::ArtistPtr artist );
        void setComposer( Meta::ComposerPtr composer );
        void setGenre( Meta::GenrePtr genre );
        void setYear( Meta::YearPtr year );
        void setStatisticsProvider( Meta::StatisticsPtr provider );

        void setLength( qint64 length );

        void setId( int id );
        int  id( ) const;
        void setAlbumId( int albumId );
        int  albumId() const;
        void setArtistId( int id );
        int  artistId() const;
        virtual void setUidUrl( const QString &url );
        void setDownloadableUrl( const QString &url );

    private:
        Meta::StatisticsPtr m_statsStore;
        ArtistPtr   m_artist;
        AlbumPtr    m_album;
        GenrePtr    m_genre;
        ComposerPtr m_composer;
        YearPtr     m_year;

        int     m_id;
        int     m_trackNumber;
        qint64     m_length;
        QString m_description;
        QString m_displayUrl;
        QString m_playableUrl;
        QString m_downloadableUrl;
        int     m_albumId;
        QString m_albumName;
        int     m_artistId;
        QString m_artistName;
        QString m_name;

};

class AMAROK_EXPORT ServiceArtist : public Meta::Artist,
                                    public ServiceDisplayInfoProvider,
                                    public ActionsProvider,
                                    public SourceInfoProvider,
                                    public BookmarkThisProvider
{
    public:
        explicit ServiceArtist( const QStringList & resultRow );
        explicit ServiceArtist( const QString & name );
        virtual ~ServiceArtist();

        QString name() const override;

        TrackList tracks() override;

        void processInfoOf( InfoParserBase * infoParser ) override;


        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                    ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                    ( type == Capabilities::Capability::BookmarkThis );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            return 0;
        }

        QAction * bookmarkAction() override {

            if ( isBookmarkable() ) {
                if ( m_bookmarkAction.isNull())
                    m_bookmarkAction = QPointer<QAction>( new BookmarkArtistAction( this, ArtistPtr( this ) ) );
                return m_bookmarkAction.data();
            }
            else
                return 0;
        }

        //ServiceArtist specific methods

        void addTrack( TrackPtr track );

        void setDescription( const QString &description );
        QString description( ) const;
        void setId( int id );
        int id( ) const;
        void setTitle( const QString &title );

        void setSourceName( const QString source ) { m_sourceName = source; }
        QString sourceName() override { return m_sourceName; }

    private:
        int       m_id;
        QString   m_name;
        QString   m_description;
        TrackList m_tracks;
        QString   m_sourceName;
};

class AMAROK_EXPORT ServiceAlbum : public Meta::Album,
                                   public ServiceDisplayInfoProvider,
                                   public ActionsProvider,
                                   public SourceInfoProvider,
                                   public BookmarkThisProvider
{
    public:
        explicit ServiceAlbum( const QStringList & resultRow );
        explicit ServiceAlbum( const QString & name  );
        virtual ~ServiceAlbum();

        QString name() const override;

        bool isCompilation() const override;
        bool canUpdateCompilation() const override;
        void setCompilation( bool compilation ) override;

        bool hasAlbumArtist() const override;
        ArtistPtr albumArtist() const override;
        TrackList tracks() override;

        void processInfoOf( InfoParserBase * infoParser ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                    ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                    ( type == Capabilities::Capability::BookmarkThis );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            return 0;
        }

        QAction * bookmarkAction() override {

            if ( isBookmarkable() ) {
                if ( m_bookmarkAction.isNull())
                    m_bookmarkAction = QPointer<QAction>( new BookmarkAlbumAction( this, AlbumPtr( this ) ) );
                return m_bookmarkAction.data();
            }
            else
                return 0;
        }

        //ServiceAlbum specific methods
        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );

        void setDescription( const QString &description );
        QString description( ) const;

        void setId( int id );
        int  id() const;

        void setArtistId( int artistId );
        int  artistId( ) const;

        void setArtistName( const QString &name );
        QString artistName() const;

        void setTitle( const QString &title );

        void setSourceName( const QString source ) { m_sourceName = source; }
        QString sourceName() override { return m_sourceName; }

    private:
        int       m_id;
        QString   m_name;
        TrackList m_tracks;
        bool      m_isCompilation;
        ArtistPtr m_albumArtist;
        QString   m_description;
        int       m_artistId;
        QString   m_artistName;
        QString   m_sourceName;
};

class AMAROK_EXPORT ServiceGenre : public Meta::Genre,
                                   public ServiceDisplayInfoProvider,
                                   public ActionsProvider,
                                   public SourceInfoProvider,
                                   public BookmarkThisProvider
{
    public:
        explicit ServiceGenre( const QString &name );
        explicit ServiceGenre( const QStringList &row );
        virtual ~ServiceGenre();

        QString name() const override;

        TrackList tracks() override;

        void processInfoOf( InfoParserBase * infoParser ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                    ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                    ( type == Capabilities::Capability::BookmarkThis );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            return 0;
        }


        //ServiceGenre specific methods
        void setId( int id );
        int id() const;

        void addTrack( TrackPtr track );

        int  albumId();
        void setAlbumId( int albumId );

        void setSourceName( const QString source ) { m_sourceName = source; }
        QString sourceName() override { return m_sourceName; }

    private:
        int       m_id;
        int       m_albumId;
        QString   m_name;
        TrackList m_tracks;
        QString   m_sourceName;
};

class AMAROK_EXPORT ServiceComposer : public Meta::Composer,
                                      public ServiceDisplayInfoProvider,
                                      public ActionsProvider,
                                      public SourceInfoProvider,
                                      public BookmarkThisProvider
{
    public:
        explicit ServiceComposer( const QString &name );
        virtual ~ServiceComposer();

        QString name() const override;

        TrackList tracks() override;

        void processInfoOf( InfoParserBase * infoParser ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                    ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                    ( type == Capabilities::Capability::BookmarkThis );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            return 0;
        }

        //ServiceComposer specific methods
        void addTrack( ServiceTrackPtr track );

    private:
        QString   m_name;
        TrackList m_tracks;
};

class AMAROK_EXPORT ServiceYear : public Meta::Year,
                                  public ServiceDisplayInfoProvider,
                                  public ActionsProvider,
                                  public SourceInfoProvider,
                                  public BookmarkThisProvider
{
    public:
        explicit ServiceYear( const QString &name );
        virtual ~ServiceYear();

        QString name() const override;

        TrackList tracks() override;

        void processInfoOf( InfoParserBase * infoParser ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::Actions ) ||
                    ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() ) ||
                    ( type == Capabilities::Capability::BookmarkThis );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::Actions )
                return new ServiceActionsCapability( this );
            else if ( type == Capabilities::Capability::SourceInfo && hasSourceInfo() )
                return new ServiceSourceInfoCapability( this );
            else if ( type == Capabilities::Capability::BookmarkThis )
                return new ServiceBookmarkThisCapability( this );
            return 0;
        }

        //ServiceYear specific methods
        void addTrack( ServiceTrackPtr track );

    private:
        QString   m_name;
        TrackList m_tracks;
};

}

#endif

