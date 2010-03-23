/****************************************************************************************
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef SQLPODCASTMETA_H
#define SQLPODCASTMETA_H

#include "PodcastMeta.h"
#include "core/meta/impl/file/File.h"

class SqlPodcastProvider;

namespace Meta
{

class SqlPodcastEpisode;
class SqlPodcastChannel;

typedef KSharedPtr<SqlPodcastEpisode> SqlPodcastEpisodePtr;
typedef KSharedPtr<SqlPodcastChannel> SqlPodcastChannelPtr;

typedef QList<SqlPodcastEpisodePtr> SqlPodcastEpisodeList;
typedef QList<SqlPodcastChannelPtr> SqlPodcastChannelList;

class SqlPodcastEpisode : public PodcastEpisode
{
    public:
        static TrackList toTrackList( SqlPodcastEpisodeList episodes );
        static PodcastEpisodeList toPodcastEpisodeList( SqlPodcastEpisodeList episodes );

        SqlPodcastEpisode( const QStringList &queryResult, SqlPodcastChannelPtr sqlChannel );

        /** Copy from another PodcastEpisode
        */
        SqlPodcastEpisode( PodcastEpisodePtr episode );

        ~SqlPodcastEpisode();

        //PodcastEpisode methods
        PodcastChannelPtr channel() const { return PodcastChannelPtr::dynamicCast( m_channel ); }
        virtual bool isNew() const { return m_isNew; }
        virtual void setNew( bool isNew );
        virtual void setLocalUrl( const KUrl &url );

        //Track Methods
        virtual QString name() const;
        virtual QString prettyName() const;
        virtual void setTitle( const QString &title );
        virtual qint64 length() const;
        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );
        virtual bool isEditable() const;
        virtual void finishedPlaying( double playedFraction );

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual ComposerPtr composer() const;
        virtual GenrePtr genre() const;
        virtual YearPtr year() const;

        //SqlPodcastEpisode specific methods
        bool writeTagsToFile();
        int dbId() const { return m_dbId; }

        void updateInDb();
        void deleteFromDb();

    private:
        bool m_batchUpdate;

        int m_dbId; //database ID
        SqlPodcastChannelPtr m_channel; //the parent of this episode

        MetaFile::TrackPtr m_localFile;
};

class SqlPodcastChannel : public PodcastChannel
{
    public:
        static PlaylistPtr toPlaylistPtr( SqlPodcastChannelPtr sqlChannel );
        static SqlPodcastChannelPtr fromPlaylistPtr( PlaylistPtr playlist );

        SqlPodcastChannel( SqlPodcastProvider *provider, const QStringList &queryResult );

        /** Copy a PodcastChannel
        */
        SqlPodcastChannel( SqlPodcastProvider *provider, PodcastChannelPtr channel );

        ~SqlPodcastChannel();
        // Meta::Playlist methods
        virtual TrackList tracks() { return Meta::SqlPodcastEpisode::toTrackList( m_episodes ); }
        virtual PlaylistProvider *provider() const;

        //Meta::PodcastChannel methods
        virtual void setTitle( const QString &title );
        virtual Meta::PodcastEpisodeList episodes();
        virtual bool hasImage() const { return !m_image.isNull(); }
        virtual void setImage( const QPixmap &image );
        virtual QPixmap image() const { return m_image; }
        virtual KUrl imageUrl() const { return m_imageUrl; }
        virtual void setImageUrl( const KUrl &imageUrl );

        PodcastEpisodePtr addEpisode( PodcastEpisodePtr episode );

        //SqlPodcastChannel specific methods
        int dbId() const { return m_dbId; }
        void addEpisode( SqlPodcastEpisodePtr episode ) { m_episodes << episode; }

        bool writeTags() const { return m_writeTags; }
        void setWriteTags( bool writeTags ) { m_writeTags = writeTags; }
        void updateInDb();
        void deleteFromDb();

        const SqlPodcastEpisodeList sqlEpisodes() { return m_episodes; }

        void loadEpisodes();

    private:
        bool m_writeTags;
        int m_dbId; //database ID

        SqlPodcastEpisodeList m_episodes;
        SqlPodcastProvider *m_provider;
};

}

Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodePtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodeList )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelPtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelList )

#endif
