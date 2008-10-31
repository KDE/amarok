/* This file is part of the KDE project
   Copyright (C) 2008 Bart Cerneels <bart.cerneels@kde.org>

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

#ifndef SQLPODCASTMETA_H
#define SQLPODCASTMETA_H

#include "PodcastMeta.h"


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

        //Track Methods
        QString type() const { return i18n("SQL Podcast"); };

        //SqlPodcastEpisode specific methods
        int dbId() const { return m_dbId; };

        void updateInDb();

    private:

        bool m_batchUpdate;

        int m_dbId; //database ID
        SqlPodcastChannelPtr m_sqlChannel; //the parent of this episode
};

class SqlPodcastChannel : public PodcastChannel
{
    public:
        SqlPodcastChannel( const QStringList &queryResult );

        /** Copy a PodcastChannel
        */
        SqlPodcastChannel( PodcastChannelPtr channel );

        ~SqlPodcastChannel();

        //SqlPodcastChannel specific methods
        int dbId() const { return m_dbId; };
        virtual void addEpisode( SqlPodcastEpisodePtr episode ) { m_sqlEpisodes << episode; };
        void addEpisode( PodcastEpisodePtr episode ) { debug() << "adding episode " << episode->title() << " to sqlchannel " << title();  m_episodes << episode; addEpisode( SqlPodcastEpisodePtr( new SqlPodcastEpisode( episode ) ) ); };

        void updateInDb();
        const SqlPodcastEpisodeList sqlEpisodes() { return m_sqlEpisodes; };

    private:
        void loadEpisodes();

        int m_dbId; //database ID

        SqlPodcastEpisodeList m_sqlEpisodes;
};

}

Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodePtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastEpisodeList )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelPtr )
Q_DECLARE_METATYPE( Meta::SqlPodcastChannelList )

#endif
