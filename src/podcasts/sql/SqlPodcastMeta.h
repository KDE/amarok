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
#include "meta/file/File.h"

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
        SqlPodcastEpisode( const QStringList &queryResult, SqlPodcastChannelPtr sqlChannel );

        /** Copy from another PodcastEpisode
        */
        SqlPodcastEpisode( PodcastEpisodePtr episode );

        ~SqlPodcastEpisode();

        //PodcastEpisode methods
        PodcastChannelPtr channel() const { return PodcastChannelPtr::dynamicCast( m_channel ); }
        virtual bool isNew() const { return m_isNew; }
        virtual void setLocalUrl( const KUrl &url );

        //Track Methods
        virtual qint64 length() const;
        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );
        virtual bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual ComposerPtr composer() const;
        virtual GenrePtr genre() const;
        virtual YearPtr year() const;

        virtual void beginMetaDataUpdate();
        virtual void endMetaDataUpdate();
        virtual void abortMetaDataUpdate();

        //SqlPodcastEpisode specific methods
        int dbId() const { return m_dbId; };

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
        static TrackList sqlEpisodesToTracks( SqlPodcastEpisodeList episodes );
        static PodcastEpisodeList sqlEpisodesToPodcastEpisodes( SqlPodcastEpisodeList episodes );

        SqlPodcastChannel( const QStringList &queryResult );

        /** Copy a PodcastChannel
        */
        SqlPodcastChannel( PodcastChannelPtr channel );

        ~SqlPodcastChannel();
        // Meta::Playlist methods
        TrackList tracks() { return sqlEpisodesToTracks( m_episodes ); }

        //Meta::PodcastChannel methods
        Meta::PodcastEpisodeList episodes();

        PodcastEpisodePtr addEpisode( PodcastEpisodePtr episode );
        //SqlPodcastChannel specific methods
        int dbId() const { return m_dbId; }
        void addEpisode( SqlPodcastEpisodePtr episode ) { m_episodes << episode; }

        void updateInDb();
        void deleteFromDb();

        const SqlPodcastEpisodeList sqlEpisodes() { return m_episodes; }

        void loadEpisodes();

    private:

        int m_dbId; //database ID

        SqlPodcastEpisodeList m_episodes;
};

}

Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodePtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodeList )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelPtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelList )

#endif
