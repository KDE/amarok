/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

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

#include "SqlPodcastProvider.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "ContextStatusBar.h"
#include "debug.h"
#include "PodcastReader.h"
#include "SqlStorage.h"
#include "TheInstances.h"

#include <KLocale>
#include <KIO/Job>
#include <KUrl>

#include <QFile>
#include <QDir>

using namespace Meta;

static const int PODCAST_DB_VERSION = 1;

SqlPodcastProvider * SqlPodcastProvider::s_instance = 0;

SqlPodcastProvider *
SqlPodcastProvider::instance()
{
    if ( s_instance == 0 )
        s_instance = new SqlPodcastProvider();

    return s_instance;
}

SqlPodcastProvider::SqlPodcastProvider()
{
    DEBUG_BLOCK

    m_sqlStorage = CollectionManager::instance()->sqlStorage();

    QStringList values;

    values = m_sqlStorage->query( "SELECT version FROM admin WHERE key = 'AMAROK_PODCAST';");
    if( values.isEmpty() )
    {
        debug() << "creating Podcast Tables";
        createTables();
    }
}

SqlPodcastProvider::~SqlPodcastProvider()
{}

bool
SqlPodcastProvider::possiblyContainsTrack( const KUrl & url ) const
{
    Q_UNUSED( url );
    return false;
}

Meta::TrackPtr
SqlPodcastProvider::trackForUrl(const KUrl & url)
{
    Q_UNUSED( url );
    return TrackPtr();
}

Meta::PlaylistList
SqlPodcastProvider::playlists()
{
    Meta::PlaylistList playlistList;

    QListIterator<Meta::SqlPodcastChannelPtr> i( m_channels );
    while( i.hasNext() )
    {
        playlistList << PlaylistPtr::staticCast( i.next() );
    }
    return playlistList;
}

void
SqlPodcastProvider::addPodcast(const KUrl & url)
{
    DEBUG_BLOCK

    KUrl kurl = KUrl( url );

    bool result = false;
    PodcastReader * podcastReader = new PodcastReader( this );

    connect( podcastReader, SIGNAL( finished( PodcastReader *, bool ) ),
             SLOT( slotReadResult( PodcastReader *, bool ) ) );

    result = podcastReader->read( kurl );
}

void
SqlPodcastProvider::addChannel( Meta::PodcastChannelPtr channel )
{
    m_channels << SqlPodcastChannelPtr( new Meta::SqlPodcastChannel( channel ) );
}

void
SqlPodcastProvider::addEpisode( Meta::PodcastEpisodePtr episode )
{
    new SqlPodcastEpisode( episode );
}

Meta::PodcastChannelList
SqlPodcastProvider::channels()
{
    PodcastChannelList list;
    QListIterator<SqlPodcastChannelPtr> i(m_channels);
    while( i.hasNext() )
    {
        list << PodcastChannelPtr::dynamicCast( i.next() );
    }
    return list;
}

Meta::SqlPodcastChannelPtr
SqlPodcastProvider::podcastChannelForId( int podcastChannelId )
{
    QListIterator<Meta::SqlPodcastChannelPtr> i( m_channels );
    while( i.hasNext() )
    {
        if( i.next()->id() == podcastChannelId )
            return i.previous();
    }
    return Meta::SqlPodcastChannelPtr();
}

void
SqlPodcastProvider::updateAll()
{
    foreach( Meta::SqlPodcastChannelPtr channel, m_channels )
    {
        update( channel );
    }
}

void
SqlPodcastProvider::update( Meta::PodcastChannelPtr channel )
{
    bool result = false;
    PodcastReader * podcastReader = new PodcastReader( this );

    connect( podcastReader, SIGNAL( finished( PodcastReader *, bool ) ),
             SLOT( slotReadResult( PodcastReader *, bool ) ) );

    result = podcastReader->update( channel );
}

void
SqlPodcastProvider::downloadEpisode( Meta::PodcastEpisodePtr podcastEpisode )
{
    SqlPodcastEpisodePtr episode = SqlPodcastEpisodePtr( new SqlPodcastEpisode( podcastEpisode ) );
    DEBUG_BLOCK

    KIO::StoredTransferJob *storedTransferJob = KIO::storedGet( episode->url(), KIO::Reload, KIO::HideProgressInfo );

    m_jobMap[storedTransferJob] = episode;
    m_fileNameMap[storedTransferJob] = KUrl( episode->url() ).fileName();

    debug() << "starting download for " << episode->title() << " url: " << episode->prettyUrl();
    The::statusBar()->newProgressOperation( storedTransferJob )
            .setDescription( episode->title().isEmpty()
                ? i18n("Downloading Podcast Media")
                : i18n("Downloading Podcast \"%1\"", episode->title()) )
            .setAbortSlot( this, SLOT( abortDownload()) );

    connect( storedTransferJob, SIGNAL(  finished( KJob * ) ), SLOT( downloadResult( KJob * ) ) );
    connect( storedTransferJob, SIGNAL( redirection( KIO::Job *, const KUrl& ) ), SLOT( redirected( KIO::Job *,const KUrl& ) ) );
}

void
SqlPodcastProvider::slotReadResult( PodcastReader *podcastReader, bool result )
{
    DEBUG_BLOCK
    if ( !result )
    {
        debug() << "Parse error in podcast "
            << podcastReader->url() << " line: "
            << podcastReader->lineNumber() << " column "
            << podcastReader->columnNumber() << " : "
            << podcastReader->errorString();
    }
    else
    {
        debug() << "Finished updating: " << podcastReader->url();
    }

    podcastReader->deleteLater();

    emit( updated() );
}

void
SqlPodcastProvider::update( Meta::SqlPodcastChannelPtr channel )
{
    update( PodcastChannelPtr::dynamicCast( channel ) );
}

void
SqlPodcastProvider::downloadEpisode( Meta::SqlPodcastEpisodePtr podcastEpisode )
{
    downloadEpisode( PodcastEpisodePtr::dynamicCast( podcastEpisode ) );
}

void
SqlPodcastProvider::slotUpdated()
{
    DEBUG_BLOCK
    emit updated();
}

void
SqlPodcastProvider::downloadResult( KJob * job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        The::statusBar()->longMessage( job->errorText() );
        debug() << "Unable to retrieve podcast media. KIO Error: " << job->errorText() << endl;
    }
    else
    {
        Meta::SqlPodcastEpisodePtr episode = m_jobMap[job];

        QDir dir( Amarok::saveLocation("podcasts") );
        //save in directory with channels title
        if ( !dir.exists( episode->channel()->title() ) )
        {
            dir.mkdir( episode->channel()->title() );
        }
        dir.cd( episode->channel()->title() );
        KUrl localUrl = KUrl::fromPath( dir.absolutePath() );
        localUrl.addPath( m_fileNameMap[job] );

        QFile *localFile = new QFile( localUrl.path() );
        if( localFile->open( IO_WriteOnly ) &&
            localFile->write( static_cast<KIO::StoredTransferJob *>(job)->data() ) != -1 )
        {
            episode->setLocalUrl( localUrl );
        }
        else
        {
            The::statusBar()->longMessage( i18n("Unable to save podcast episode file to %1",
                            localUrl.prettyUrl()) );
        }
        localFile->close();
    }
    //remove it from the jobmap
    m_jobMap.remove( job );
    m_fileNameMap.remove( job );
}

void
SqlPodcastProvider::redirected( KIO::Job *job, const KUrl & redirectedUrl )
{
    debug() << "redirecting to " << redirectedUrl << ". filename: " << redirectedUrl.fileName();
    m_fileNameMap[job] = redirectedUrl.fileName();
}

void
SqlPodcastProvider::createTables() const
{
    DEBUG_BLOCK

    m_sqlStorage->query( QString( "CREATE TABLE podcastchannels "
                    "(id " + m_sqlStorage->idType() +
                    ",url " + m_sqlStorage->exactTextColumnType() + " UNIQUE"
                    ",title " + m_sqlStorage->textColumnType() +
                    ",weblink " + m_sqlStorage->exactTextColumnType() +
                    ",image " + m_sqlStorage->exactTextColumnType() +
                    //TODO: change to description
                    ",comment " + m_sqlStorage->longTextColumnType() +
                    ",copyright "  + m_sqlStorage->textColumnType() +
                    ",directory "  + m_sqlStorage->textColumnType() +
                    ",labels" + m_sqlStorage->textColumnType() +
                    ",autoscan BOOL, fetchtype INTEGER"
                    ",autotransfer BOOL, haspurge BOOL, purgecount INTEGER );" ) );

    m_sqlStorage->query( QString( "CREATE TABLE podcastepisodes ("
                    "id " + m_sqlStorage->idType() +
                    ",url " + m_sqlStorage->exactTextColumnType() + " UNIQUE"
                    ",parent INTEGER"
                    ",localurl " + m_sqlStorage->exactTextColumnType() +
                    ",guid " + m_sqlStorage->exactTextColumnType() +
                    ",title " + m_sqlStorage->textColumnType() +
                    ",subtitle " + m_sqlStorage->textColumnType() +
                    ",sequencenumber INTEGER" +
                    ",description " + m_sqlStorage->longTextColumnType() +
                    //TODO: change to mimetype
                    ",filetype "  + m_sqlStorage->textColumnType() +
                    ",createdate "  + m_sqlStorage->textColumnType() +
                    ",length INTEGER"
                    ",size INTEGER"
                    ",isNew BOOL );" ));

    m_sqlStorage->query( "CREATE INDEX url_podchannel ON podcastchannels( url );" );
    m_sqlStorage->query( "CREATE INDEX url_podepisode ON podcastepisodes( url );" );
    m_sqlStorage->query( "CREATE INDEX localurl_podepisode ON podcastepisodes( localurl );" );

    m_sqlStorage->query( "INSERT INTO admin(key,version) "
                          "VALUES('AMAROK_PODCAST'," + QString::number( PODCAST_DB_VERSION ) + ");" );
}

#include "SqlPodcastProvider.moc"
