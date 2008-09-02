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

#include "PodcastReader.h"

#include "Debug.h"

#include <kio/job.h>
#include <kurl.h>

#include <QDate>
#include <QDebug>
#include <QMap>

using namespace Meta;

PodcastReader::PodcastReader( PodcastProvider * podcastProvider )
        : QXmlStreamReader()
        , m_podcastProvider( podcastProvider )
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

    KIO::TransferJob *getJob = KIO::get( m_url, KIO::Reload, KIO::HideProgressInfo );

    connect( getJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             SLOT( slotAddData( KIO::Job *, const QByteArray & ) ) );

    connect( getJob, SIGNAL(  result( KJob * ) ),
             SLOT( downloadResult( KJob * ) ) );

    connect( getJob, SIGNAL( redirection( KIO::Job *, const KUrl & ) ),
             SLOT( slotRedirection( KIO::Job *, const KUrl & ) ) );
    connect( getJob, SIGNAL( permanentRedirection( KIO::Job *,
             const KUrl &, const KUrl &) ),
             SLOT( slotPermanentRedirection( KIO::Job *, const KUrl &,
             const KUrl &) ) );

    return !getJob->isErrorPage();
}

bool
PodcastReader::update( PodcastChannelPtr channel )
{
    DEBUG_BLOCK
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
    Q_UNUSED( job );
    DEBUG_BLOCK

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
            debug() << "recovering from PrematureEndOfDocumentError for "
                    << QXmlStreamReader::name().toString() << " at "
                    << QXmlStreamReader::lineNumber();
            readNext();
            debug() << "reading " << tokenString();
        }
        else
            debug() << "some other error occurred: " << errorString();

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
                            m_channel->setSubscribeDate( QDate::currentDate() );
                            m_current = static_cast<Meta::PodcastMetaCommon *>( m_channel.data() );
                        }
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
                if( QXmlStreamReader::name() == "item" )
                {
                    debug() << "new episode";
                    m_current = new Meta::PodcastEpisode( m_channel );
                }
                else if( QXmlStreamReader::name() == "enclosure" )
                {
                    m_urlString = QXmlStreamReader::attributes().value( QString(), QString("url") ).toString();
                }
                m_currentTag = QXmlStreamReader::name().toString();
            }
            else if( isEndElement() )
            {
                if (QXmlStreamReader::name() == "item")
                {
                    commitEpisode();
                }
                else if( QXmlStreamReader::name() == "channel" )
                {
                    commitChannel();
                    emit finished( this, true );
                    break;
                }
                else if( QXmlStreamReader::name() == "title")
                {
                    m_current->setTitle( m_titleString );
                    m_titleString.clear();
                }
                else if( QXmlStreamReader::name() == "description" )
                {
                    m_current->setDescription( m_descriptionString );
                    m_descriptionString.clear();
                }
                else if( QXmlStreamReader::name() == "guid" )
                {
                    static_cast<PodcastEpisode *>(m_current)->setGuid( m_guidString );
                    m_guidString.clear();
                }
                else if( QXmlStreamReader::name() == "enclosure" )
                {
                    debug() << "enclosure: url = " << m_urlString;
                    static_cast<PodcastEpisode *>(m_current)->setUidUrl( KUrl( m_urlString ) );
                    m_urlString.clear();
                }
                else if( QXmlStreamReader::name() == "link" )
                {
                    m_channel->setWebLink( KUrl( m_linkString ) );
                    m_linkString.clear();
                }
            }
            else if( isCharacters() && !isWhitespace() )
            {
                if( m_currentTag == "title" )
                    m_titleString += text().toString();
                else if( m_currentTag == "link" )
                    m_linkString += text().toString();
                else if( m_currentTag == "description" )
                    m_descriptionString += text().toString();
                else if( m_currentTag == "pubDate" )
                    m_pubDateString += text().toString();
                else if( m_currentTag == "guid" )
                    m_guidString += text().toString();
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

    if( m_podcastProvider->channels().contains( m_channel ) )
        return;

    debug() << "commit new Podcast Channel " << m_channel->title();
//     m_podcastProvider->acquireReadLock();
    m_channel = m_podcastProvider->addChannel( PodcastChannelPtr( m_channel ) );
//     m_podcastProvider->releaseLock();

//     emit finished( this, true );
}

void
PodcastReader::commitEpisode()
{
    DEBUG_BLOCK
    Q_ASSERT( m_current );
    PodcastEpisodePtr item = PodcastEpisodePtr( static_cast<PodcastEpisode *>(m_current) );

    PodcastEpisodePtr episodeMatch = podcastEpisodeCheck( item );
    if( episodeMatch == item )
    {
        debug() << "commit episode " << item->title();

        Q_ASSERT( m_channel );
        //make a copy of the pointer and add that to the channel
        m_channel->addEpisode( PodcastEpisodePtr( item ) );
    }

    m_current = static_cast<PodcastMetaCommon *>( m_channel.data() );
}

Meta::PodcastEpisodePtr
PodcastReader::podcastEpisodeCheck(Meta::PodcastEpisodePtr episode)
{
//     DEBUG_BLOCK
    Meta::PodcastEpisodePtr episodeMatch = episode;
    Meta::PodcastEpisodeList episodes = m_channel->episodes();

//     debug() << "episode title: " << episode->title();
//     debug() << "episode url: " << episode->url();
//     debug() << "episode guid: " << episode->guid();

    foreach( PodcastEpisodePtr match, episodes )
    {
//         debug() << "match title: " << match->title();
//         debug() << "match url: " << match->url();
//         debug() << "match guid: " << match->guid();

        int score = 0;
        if( !episode->title().isEmpty() && episode->title() == match->title() )
            score += 1;
        if( !episode->uidUrl().isEmpty() && episode->uidUrl() == match->uidUrl() )
            score += 3;
        if( !episode->guid().isEmpty() && episode->guid() == match->guid() )
            score += 3;

//         debug() << "score: " << score;
        if( score >= 3 )
        {
            episodeMatch = match;
            break;
        }
    }

    return episodeMatch;
}

#include "PodcastReader.moc"
