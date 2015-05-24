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
        virtual QString name() const { return title(); }

        //PodcastMetaCommon methods
        virtual QString title() const { return m_master->title(); }
        virtual QString description() const { return m_master->description(); }
        virtual QStringList keywords() const { return m_master->keywords(); }
        virtual QString subtitle() const { return m_master->subtitle(); }
        virtual QString summary() const { return m_master->summary(); }
        virtual QString author() const { return m_master->author(); }

        //Podcasts::PodcastChannel methods
        virtual QUrl url() const { return m_master->url(); }
        virtual QUrl webLink() const { return m_master->webLink(); }
        virtual bool hasImage() const { return m_master->hasImage(); }
        virtual QUrl imageUrl() const { return m_master->imageUrl(); }
        virtual QImage image() const { return m_master->image(); }
        virtual QString copyright() const { return m_master->copyright(); }
        virtual QStringList labels() const { return m_master->labels(); }
        virtual QDate subscribeDate() const { return m_master->subscribeDate(); }

        virtual void setUrl( const QUrl &url ) { m_master->setUrl( url ); }
        virtual void setWebLink( const QUrl &link ) { m_master->setWebLink( link ); }
        virtual void setImage( const QImage &image ) { m_master->setImage( image ); }
        virtual void setImageUrl( const QUrl &imageUrl ) { m_master->setImageUrl( imageUrl ); }
        virtual void setCopyright( const QString &copyright ) { m_master->setCopyright( copyright ); }
        virtual void setLabels( const QStringList &labels ) { m_master->setLabels( labels ); }
        virtual void addLabel( const QString &label ) { m_master->addLabel( label ); }
        virtual void setSubscribeDate( const QDate &date ) { m_master->setSubscribeDate( date ); }

        virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode )
        {
            return m_master->addEpisode( episode );
        }
        virtual Podcasts::PodcastEpisodeList episodes() const { return m_master->episodes(); }

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

typedef KSharedPtr<SyncedPodcast> SyncedPodcastPtr;
typedef QList<SyncedPodcastPtr> SyncedPodcastList;

Q_DECLARE_METATYPE( SyncedPodcastPtr )
Q_DECLARE_METATYPE( SyncedPodcastList )

#endif // SYNCEDPODCAST_H
