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

#include "PodcastReader.h"

#include "debug.h"

#include <kio/job.h>
#include <kurl.h>

#include <QDebug>
#include <QMap>

using namespace Meta;

PodcastReader::PodcastReader( PodcastCollection * collection )
        : QXmlStreamReader()
        , m_collection( collection )
        , m_current( 0 )
{}

PodcastReader::~PodcastReader()
{
    DEBUG_BLOCK
}

bool PodcastReader::read ( QIODevice *device )
{
    DEBUG_BLOCK
    setDevice( device );
    return read();
}

bool
PodcastReader::read(const KUrl &url)
{
    DEBUG_BLOCK

    m_url = url;

    KIO::TransferJob *getJob = KIO::storedGet( m_url, KIO::Reload, KIO::HideProgressInfo );

//     connect( getJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
//              SLOT( slotAddData( KIO::Job *, const QByteArray & ) ) );

    connect( getJob, SIGNAL(  result( KJob * ) ),
             SLOT( downloadResult( KJob * ) ) );

    connect( getJob, SIGNAL( redirection( KIO::Job *, const KUrl & ) ),
             SLOT( slotRedirection( KIO::Job *, const KUrl & ) ) );
    connect( getJob, SIGNAL( permanentRedirection( KIO::Job *,
             const KUrl &, const KUrl &) ),
             SLOT( slotPermanentRedirection( KIO::Job *, const KUrl &,
             const KUrl &) ) );

//     read();

    return !getJob->isErrorPage();
}

bool
PodcastReader::update( PodcastChannelPtr channel )
{
    m_channel = channel;
    m_current = static_cast<PodcastMetaCommon *>(channel.data());

    return read( m_channel->url() );
}

void
PodcastReader::slotAddData( KIO::Job *, const QByteArray & data)
{
    DEBUG_BLOCK

    QXmlStreamReader::addData( data );
    //parse some more data
    read();
}

void
PodcastReader::downloadResult( KJob * job )
{
    DEBUG_BLOCK

    QXmlStreamReader::addData( static_cast<KIO::StoredTransferJob *>(job)->data() );
    //parse some more data
    read();
}

bool
PodcastReader::read()
{
    DEBUG_BLOCK
    bool result = true;

    while ( !atEnd() )
    {
        if( !error() )
        {
            readNext();
            debug() << "reading " << tokenString();
        }
        else if ( error() == PrematureEndOfDocumentError )
        {
            debug() << "recovering from PrematureEndOfDocumentError";
        }
        else
            debug() << "some other error occured: " << errorString();

        if( !m_current )
        {
            debug() << "no m_current yet";

            //Pre Channel
            if ( isStartElement() )
            {
                debug() << "Initial StartElement: " << QXmlStreamReader::name().toString();
                debug() << "version: " << attributes().value ( "version" ).toString();
                if ( QXmlStreamReader::name() == "rss" && attributes().value ( "version" ) == "2.0" )
                {
                    while( readNext() == QXmlStreamReader::Characters )
                    {
                        debug() << "reading Characters";
                    }

                    if (isEndElement())
                    {
                        debug() << "endElement";
                        break;
                    }
                    if (isStartElement())
                    {
                        debug() << "nested StartElement: " << QXmlStreamReader::name().toString();
                        if ( QXmlStreamReader::name() == "channel" )
                        {
                            debug() << "new channel";
                            m_channel = new Meta::PodcastChannel();
                            m_channel->setUrl( m_url );
                            debug() << "m_channel.data(): " << m_channel.data();
                            m_current = static_cast<Meta::PodcastMetaCommon *>( m_channel.data() );
                        }
//                         else
//                             readUnknownElement();
                    }
                }
                else
                {
                    raiseError ( QObject::tr ( "The file is not an RSS version 2.0 file." ) );
                    result = false;
                    break;
                }
            }
            else if( tokenType() != QXmlStreamReader::StartDocument )
            {
                debug() << "some weird thing happend. " << QXmlStreamReader::name().toString()
                        << " : " << tokenString();
            }
        }
        else
        {
            if( isStartElement() )
            {
                debug() << "startElement: " << QXmlStreamReader::name().toString();
                if (QXmlStreamReader::name() == "item")
                {
                    debug() << "new episode";
                    m_current = new Meta::PodcastEpisode( m_channel );
                }
                else if( QXmlStreamReader::name() == "title" )
                    m_current->setTitle( readTitle() );
                else if( QXmlStreamReader::name() == "description" )
                    m_current->setDescription( readDescription() );
                else if( QXmlStreamReader::name() == "guid" )
                    static_cast<PodcastEpisode *>(m_current)->setGuid( readGuid() );
                else if( QXmlStreamReader::name() == "enclosure" )
                    static_cast<PodcastEpisode *>(m_current)->setUrl( readUrl() );
            }
            else if( isEndElement() )
            {
                debug() << "endElement: " << QXmlStreamReader::name().toString();;
                if (QXmlStreamReader::name() == "item")
                {
                    commitEpisode();
//                     commitChannel(); //DEBUG: should not commit here.
                }
                else if( QXmlStreamReader::name() == "channel" )
                {
                    commitChannel();
                    emit finished( this, true );
                    break;
                }
            }
        }
    }

    if ( error() )
    {
        if ( error() == QXmlStreamReader::PrematureEndOfDocumentError)
        {
            qDebug() << "waiting for data at line " << lineNumber();
        }
        else
        {
            qDebug() << "XML ERROR: " << error() << " at line: " << lineNumber()
                    << ": " << columnNumber ()
                    << "\n\t" << errorString();
            qDebug() << "\tname = " << QXmlStreamReader::name().toString()
                    << " tokenType = " << tokenString();

            if( m_channel )
                commitChannel();
            emit finished( this, false );
        }
    }
    return result;
}

QString
PodcastReader::readTitle()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "title" );

    return readElementText();
}

QString
PodcastReader::readDescription()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "description" );

    return readElementText();
}

QString
PodcastReader::readLink()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "link" );
    return readElementText();
}

KUrl
PodcastReader::readUrl()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "enclosure" );
    //TODO: need to get the url argument here
    QString url = attributes().value( "", "url").toString();
    debug() << readElementText();
    return KUrl( url );
}

QString
PodcastReader::readGuid()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "guid" );
    return readElementText();
}

QString
PodcastReader::readPubDate()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "pubDate" );
    return readElementText();
}

void
PodcastReader::readUnknownElement()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() );

    debug() << "unknown element: " << QXmlStreamReader::name().toString();

    while ( !atEnd() )
    {
        readNext();

        if ( isEndElement() )
            break;

        if ( isStartElement() )
            readUnknownElement();
    }
}

void
PodcastReader::slotRedirection( KIO::Job * job, const KUrl & url )
{
    DEBUG_BLOCK
    Q_UNUSED( job );
    debug() << "redirected to: " << url.url();

}

void
PodcastReader::slotPermanentRedirection( KIO::Job * job, const KUrl & fromUrl,
        const KUrl & toUrl)
{
    DEBUG_BLOCK
    Q_UNUSED( job ); Q_UNUSED( fromUrl );
    debug() << "premanently redirected to: " << toUrl.url();
    //TODO: change url in database
}

void
PodcastReader::commitChannel()
{
    Q_ASSERT( m_channel );

    if( m_collection->channels().contains( m_channel ) )
        return;

    debug() << "commit new Podcast Channel (as Album) " << m_channel->title();
    m_collection->acquireReadLock();
    m_collection->addChannel( PodcastChannelPtr( m_channel ) );
    m_collection->releaseLock();

//     emit finished( this, true );
}

void
PodcastReader::commitEpisode()
{
    Q_ASSERT( m_current );
    PodcastEpisodePtr item = PodcastEpisodePtr( static_cast<PodcastEpisode *>(m_current) );
    item->setAlbum( m_channel->name() );

    PodcastEpisodePtr episodeMatch = podcastEpisodeCheck( item );
    if( episodeMatch == item )
    {
        debug() << "commit episode " << m_current->title();
        m_collection->acquireReadLock();
        m_collection->addEpisode( item );
        m_collection->releaseLock();

        Q_ASSERT( m_channel );
        //make a copy of the pointer and add that to the channel
        m_channel->addEpisode( PodcastEpisodePtr( item ) );
    }

    m_current = static_cast<PodcastMetaCommon *>( m_channel.data() );
}

Meta::PodcastEpisodePtr
PodcastReader::podcastEpisodeCheck(Meta::PodcastEpisodePtr episode)
{
    DEBUG_BLOCK
    Meta::PodcastEpisodePtr episodeMatch = episode;
    Meta::PodcastEpisodeList episodes = m_channel->episodes();

    debug() << "episode title: " << episode->title();
    debug() << "episode url: " << episode->url();
    debug() << "episode guid: " << episode->guid();

    foreach( PodcastEpisodePtr match, episodes )
    {
        debug() << "match title: " << match->title();
        debug() << "match url: " << match->url();
        debug() << "match guid: " << match->guid();

        int score = 0;
        if( !episode->title().isEmpty() && episode->title() == match->title() )
            score += 1;
        if( !episode->url().isEmpty() && episode->url() == match->url() )
            score += 3;
        if( !episode->guid().isEmpty() && episode->guid() == match->guid() )
            score += 3;

        debug() << "score: " << score;
        if( score >= 3 )
        {
            episodeMatch = match;
            break;
        }
    }

    return episodeMatch;
}

#include "PodcastReader.moc"
