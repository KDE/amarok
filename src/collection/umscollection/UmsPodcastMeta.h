/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef UMSPODCASTMETA_H
#define UMSPODCASTMETA_H

#include "podcasts/PodcastMeta.h"

#include "meta/file/File.h"

class UmsPodcastEpisode;
class UmsPodcastChannel;

typedef KSharedPtr<UmsPodcastEpisode> UmsPodcastEpisodePtr;
typedef KSharedPtr<UmsPodcastChannel> UmsPodcastChannelPtr;

typedef QList<UmsPodcastEpisodePtr> UmsPodcastEpisodeList;
typedef QList<UmsPodcastChannelPtr> UmsPodcastChannelList;

class UmsPodcastEpisode : public Meta::PodcastEpisode
{
    public:
        UmsPodcastEpisode();
        ~UmsPodcastEpisode();

        void setLocalUrl( KUrl localUrl );
        void setLocalFile( MetaFile::TrackPtr localFile );

        //Track Methods
        virtual QString name() const;
        virtual QString prettyName() const;
        virtual void setTitle( const QString &title );
        virtual bool isEditable() const;

        virtual Meta::AlbumPtr album() const;
        virtual Meta::ArtistPtr artist() const;
        virtual Meta::ComposerPtr composer() const;
        virtual Meta::GenrePtr genre() const;
        virtual Meta::YearPtr year() const;

    private:
        MetaFile::TrackPtr m_localFile;
};

class UmsPodcastChannel : public Meta::PodcastChannel
{

};

Q_DECLARE_METATYPE( UmsPodcastEpisodePtr )
Q_DECLARE_METATYPE( UmsPodcastEpisodeList )
Q_DECLARE_METATYPE( UmsPodcastChannelPtr )
Q_DECLARE_METATYPE( UmsPodcastChannelList )

#endif // UMSPODCASTMETA_H
