/****************************************************************************************
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#ifndef SYNCEDPODCAST_H
#define SYNCEDPODCAST_H

#include "core/podcasts/PodcastMeta.h"
#include "playlistmanager/SyncedPlaylist.h"

class SyncedPodcast : public SyncedPlaylist, public Podcasts::PodcastChannel
{
    public:
        explicit SyncedPodcast( Podcasts::PodcastChannelPtr podcast );
        virtual ~SyncedPodcast() {}

        //Playlist virtual methods
        QString name() const override { return title(); }

        //PodcastMetaCommon methods
        QString title() const override { return m_master->title(); }
        QString description() const override { return m_master->description(); }
        QStringList keywords() const override { return m_master->keywords(); }
        QString subtitle() const override { return m_master->subtitle(); }
        QString summary() const override { return m_master->summary(); }
        QString author() const override { return m_master->author(); }

        //Podcasts::PodcastChannel methods
        QUrl url() const override { return m_master->url(); }
        QUrl webLink() const override { return m_master->webLink(); }
        bool hasImage() const override { return m_master->hasImage(); }
        QUrl imageUrl() const override { return m_master->imageUrl(); }
        QImage image() const override { return m_master->image(); }
        QString copyright() const override { return m_master->copyright(); }
        QStringList labels() const override { return m_master->labels(); }
        QDate subscribeDate() const override { return m_master->subscribeDate(); }

        void setUrl( const QUrl &url ) override { m_master->setUrl( url ); }
        void setWebLink( const QUrl &link ) override { m_master->setWebLink( link ); }
        void setImage( const QImage &image ) override { m_master->setImage( image ); }
        void setImageUrl( const QUrl &imageUrl ) override { m_master->setImageUrl( imageUrl ); }
        void setCopyright( const QString &copyright ) override { m_master->setCopyright( copyright ); }
        void setLabels( const QStringList &labels ) override { m_master->setLabels( labels ); }
        void addLabel( const QString &label ) override { m_master->addLabel( label ); }
        void setSubscribeDate( const QDate &date ) override { m_master->setSubscribeDate( date ); }

        Podcasts::PodcastEpisodePtr addEpisode( const Podcasts::PodcastEpisodePtr &episode ) override
        {
            return m_master->addEpisode( episode );
        }
        Podcasts::PodcastEpisodeList episodes() const override { return m_master->episodes(); }

        bool load( QTextStream &stream ) { return m_master->load( stream ); }

        //Settings
        virtual QUrl saveLocation() const { return m_master->saveLocation(); }
        virtual bool autoScan() const { return m_master->autoScan(); }
        virtual bool hasPurge() const { return m_master->hasPurge(); }
        virtual int purgeCount() const { return m_master->purgeCount(); }

        void setSaveLocation( const QUrl &url ) { m_master->setSaveLocation( url ); }
        void setAutoScan( bool autoScan ) { m_master->setAutoScan( autoScan ); }
        void setFetchType( Podcasts::PodcastChannel::FetchType fetchType )
        {
            m_master->setFetchType( fetchType );
        }
        void setPurge( bool purge ) { m_master->setPurge( purge ); }
        void setPurgeCount( int purgeCount ) { m_master->setPurgeCount( purgeCount ); }

    private:
        Podcasts::PodcastChannelPtr m_master;
};

typedef AmarokSharedPointer<SyncedPodcast> SyncedPodcastPtr;
typedef QList<SyncedPodcastPtr> SyncedPodcastList;

Q_DECLARE_METATYPE( SyncedPodcastPtr )
Q_DECLARE_METATYPE( SyncedPodcastList )

#endif // SYNCEDPODCAST_H
