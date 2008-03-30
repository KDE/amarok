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

#include "PodcastCollection.h"

#include "Amarok.h"
//#include "AmarokStatusBar.h"
#include "CollectionManager.h"
#include "ContextStatusBar.h"
#include "debug.h"
#include "PodcastReader.h"
#include "support/MemoryQueryMaker.h"
#include "SqlStorage.h"
#include "TheInstances.h"

#include <KLocale>
#include <KIO/Job>
#include <KUrl>

#include <QFile>
#include <QDir>

using namespace Meta;

static const int PODCAST_DB_VERSION = 1;

PodcastCollection::PodcastCollection()
{
    DEBUG_BLOCK

    m_channelProvider = new PodcastChannelProvider( this );
    m_sqlStorage = CollectionManager::instance()->sqlStorage();

    QStringList values;

    values = m_sqlStorage->query( "SELECT version FROM admin WHERE key = 'AMAROK_PODCAST';");
    if( values.isEmpty() )
    {
        debug() << "creating Podcast Tables";
        createTables();
    }
}


PodcastCollection::~PodcastCollection()
{}

QueryMaker*
PodcastCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
PodcastCollection::collectionId() const
{
    return "Podcasts";
}

bool
PodcastCollection::possiblyContainsTrack(const KUrl & url) const
{
    Q_UNUSED( url );
    return false;
}

Meta::TrackPtr
PodcastCollection::trackForUrl(const KUrl & url)
{
    Q_UNUSED( url );
    return TrackPtr();
}

CollectionLocation *
PodcastCollection::location() const
{
    return 0;
}

void
PodcastCollection::slotUpdateAll()
{
    foreach( Meta::PodcastChannelPtr channel, m_channels )
    {
        slotUpdate( channel );
    }
}

void
PodcastCollection::slotUpdate( Meta::PodcastChannelPtr channel )
{
    bool result = false;
    PodcastReader * podcastReader = new PodcastReader( this );

    connect( podcastReader, SIGNAL( finished( PodcastReader *, bool ) ),
             SLOT( slotReadResult( PodcastReader *, bool ) ) );

    result = podcastReader->update( channel );
}

void
PodcastCollection::slotReadResult( PodcastReader *podcastReader, bool result )
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

    m_channelProvider->slotUpdated();
    emit( updated() );
}


void
PodcastCollection::addPodcast(const QString & url)
{
    DEBUG_BLOCK

    if( url.isNull() || url.isEmpty() )
    {
        debug() << " attempt to add an empty url";
        return;
    }

    KUrl kurl = KUrl( url );

    bool result = false;
    PodcastReader * podcastReader = new PodcastReader( this );

    connect( podcastReader, SIGNAL( finished( PodcastReader *, bool ) ),
             SLOT( slotReadResult( PodcastReader *, bool ) ) );

    result = podcastReader->read( kurl );
}

void
PodcastCollection::addChannel( Meta::PodcastChannelPtr channel )
{
    m_channels << channel;
    debug() << "channel.count() = " << channel.count();
    addAlbum( channel->name(), AlbumPtr::dynamicCast( channel ) );
}

void
PodcastCollection::addEpisode( Meta::PodcastEpisodePtr episode )
{
    addTrack( episode->name(), TrackPtr::dynamicCast( episode ) );
}

void
PodcastCollection::slotDownloadEpisode( Meta::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK

    KIO::StoredTransferJob *storedTransferJob = KIO::storedGet( episode->remoteUrl(), KIO::Reload, KIO::HideProgressInfo );

    m_jobMap[storedTransferJob] = episode;
    m_fileNameMap[storedTransferJob] = episode->remoteUrl().fileName();

    debug() << "starting download for " << episode->title() << " url: " << episode->remoteUrl().prettyUrl();
    The::contextStatusBar()->newProgressOperation( storedTransferJob )
            .setDescription( episode->title().isEmpty()
            ? i18n( "Downloading Podcast Media" )
    : i18n( "Downloading Podcast \"%1\"", episode->title() ) )
            .setAbortSlot( this, SLOT( abortDownload()) );

    connect( storedTransferJob, SIGNAL(  finished( KJob * ) ), SLOT( downloadResult( KJob * ) ) );
    connect( storedTransferJob, SIGNAL( redirection( KIO::Job *, const KUrl& ) ), SLOT( redirected( KIO::Job *,const KUrl& ) ) );
}

void
PodcastCollection::downloadResult( KJob * job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        Amarok::ContextStatusBar::instance()->longMessage( job->errorText() );
        debug() << "Unable to retrieve podcast media. KIO Error: " << job->errorText() << endl;
    }
    else
    {
        Meta::PodcastEpisodePtr episode = m_jobMap[job];

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
            episode->setPlayableUrl( localUrl );
        }
        else
        {
            Amarok::ContextStatusBar::instance()->longMessage( i18n("Unable to save podcast episode file to %1",
                                                                    localUrl.prettyUrl()) );
        }
        localFile->close();
    }
    //remove it from the jobmap
    m_jobMap.remove( job );
    m_fileNameMap.remove( job );
}

void
PodcastCollection::redirected( KIO::Job *job, const KUrl & redirectedUrl )
{
    debug() << "redirecting to " << redirectedUrl << ". filename: " << redirectedUrl.fileName();
    m_fileNameMap[job] = redirectedUrl.fileName();
}

PodcastChannelProvider::PodcastChannelProvider( PodcastCollection *parent) : PlaylistProvider(),
        m_parent( parent )
{
}

void
PodcastChannelProvider::slotUpdated()
{
    DEBUG_BLOCK
    emit updated();
}

Meta::PlaylistList
PodcastChannelProvider::playlists()
{
    Meta::PlaylistList playlistList;

    QListIterator<Meta::PodcastChannelPtr> i( m_parent->channels() );
    while( i.hasNext() )
    {
        playlistList << PlaylistPtr::staticCast( i.next() );
    }
    return playlistList;
}

//SQL storage stuff
void
PodcastCollection::createTables() const
{
    DEBUG_BLOCK

    m_sqlStorage->query( QString( "CREATE TABLE podcastchannels "
                    "(id " + m_sqlStorage->idType() +
                    ",url " + m_sqlStorage->exactTextColumnType() + " UNIQUE"
                    ",title " + m_sqlStorage->textColumnType() +
                    ",weblink " + m_sqlStorage->exactTextColumnType() +
                    ",image " + m_sqlStorage->exactTextColumnType() +
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

#include "PodcastCollection.moc"
