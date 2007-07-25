/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef PODCASTCOLLECTION_H
#define PODCASTCOLLECTION_H

#include <collection.h>
#include "support/memorycollection.h"
#include "PodcastMeta.h"

class KUrl;
class PodcastReader;

/**
	@author Bart Cerneels <bart.cerneels@gmail.com>
*/
class PodcastCollection : public Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        static PodcastCollection * instance();

        virtual QueryMaker * queryMaker();
        virtual void startFullScan() { }

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual CollectionLocation* location() const;

        void addPodcast( const QString &url );

        void addChannel( Meta::PodcastChannelPtr channel );
        void addEpisode( Meta::PodcastEpisodePtr episode );

        Meta::PodcastChannelList channels() { return m_channels; };

    protected:
        PodcastCollection();
        ~PodcastCollection();

    signals:
        void remove();

    public slots:
        void slotUpdateAll();
        void slotUpdate( QString podcastChannelName );
        void slotReadResult( PodcastReader *podcastReader, bool result );

    private:
        static PodcastCollection* s_instance;

        QList<KUrl> urls;

        Meta::PodcastChannelList m_channels;

};

#endif
