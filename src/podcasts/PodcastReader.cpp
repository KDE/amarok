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

#include "PodcastMetaBase.h"
#include "debug.h"

#include <kio/job.h>
#include <kurl.h>

#include <QDebug>
#include <QMap>

PodcastReader::PodcastReader( PodcastCollection * collection )
        : QXmlStreamReader()
        , m_collection( collection )
        , m_current( 0 )
{}

PodcastReader::~PodcastReader()
{
}

bool PodcastReader::read ( QIODevice *device )
{
    DEBUG_BLOCK
    setDevice( device );
    return read();
}

bool
PodcastReader::read(const QString & url)
{
    DEBUG_BLOCK

    m_url = url;

    KIO::TransferJob *getJob = KIO::get( KUrl( url ), true, false );

    connect( getJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             SLOT( slotAddData( KIO::Job *, const QByteArray & ) ) );
    connect( getJob, SIGNAL( redirection( KIO::Job *, const KUrl & ) ),
             SLOT( slotRedirection( KIO::Job *, const KUrl & ) ) );
    connect( getJob, SIGNAL( permanentRedirection( KIO::Job *,
             const KUrl &, const KUrl &) ),
             SLOT( slotPermanentRedirection( KIO::Job *, const KUrl &,
             const KUrl &) ) );

//     read();

    return !getJob->isErrorPage();
}

void
PodcastReader::slotAddData( KIO::Job *, const QByteArray & data)
{
    DEBUG_BLOCK

    QXmlStreamReader::addData( data );
    //parse some more data
    read();
}

bool PodcastReader::read()
{
    DEBUG_BLOCK
    bool result = true;

    while ( !atEnd() )
    {
        if( !m_current )
        {
            //Pre Channel
            readNext();

            if ( isStartElement() )
            {
                debug() << "Initial StartElement: " << QXmlStreamReader::name().toString() << endl;
                debug() << "version: " << attributes().value ( "version" ).toString() << endl;
                if ( QXmlStreamReader::name() == "rss" && attributes().value ( "version" ) == "2.0" )
                {
                    while( readNext() == QXmlStreamReader::Characters )
                    {
                        debug() << "reading Characters" << endl;
                    }

                    if (isEndElement())
                    {
                        debug() << "endElement" << endl;
                        Q_ASSERT( false );
                        break;
                    }
                    if (isStartElement())
                    {
                        debug() << "nested StartElement: " << QXmlStreamReader::name().toString() << endl;
                        if ( QXmlStreamReader::name() == "channel" )
                        {
                            debug() << "new channel" << endl;
                            m_current = m_channel = new PodcastAlbum();
                            m_artist = PodcastArtistPtr( new PodcastArtist( "testcast" ) );
                        }
//                         else
//                             readUnknownElement();
                    }
                }
                else
                {
                    raiseError ( QObject::tr ( "The file is not an RSS version 2.0 file." ) );
                    result = false;
                }
            }
            continue;
        }

        Q_ASSERT( m_current );

        if( error() == QXmlStreamReader::NoError )
            readNext();

        if (isEndElement())
        {
            debug() << "endElement: " << QXmlStreamReader::name().toString() << endl;;
            if (QXmlStreamReader::name() == "item")
            {
                commitEpisode();
                commitChannel(); //DEBUG: should not commit here.

            }
            else if( QXmlStreamReader::name() == "channel" )
            {
                commitChannel();
                emit finished( this, true );
            }
            break;
        }
        else if( isStartElement() )
        {
            debug() << "startElement: " << QXmlStreamReader::name().toString() << endl;
            Q_ASSERT( m_current );
            if (QXmlStreamReader::name() == "title")
                m_current->setTitle( readTitle() );
            else if (QXmlStreamReader::name() == "description")
                m_current->setDescription( readDescription() );
            else if (QXmlStreamReader::name() == "item")
            {
                debug() << "new episode" << endl;
                m_current = new PodcastTrack();
            }
        }
    }

    if ( error() && error() != QXmlStreamReader::PrematureEndOfDocumentError )
    {
        debug() << "XML ERROR:" << lineNumber() << ": " << errorString() << endl;
        emit finished( this, result );
    }

    if ( error() == QXmlStreamReader::PrematureEndOfDocumentError)
    {
        debug() << "waiting for data at line " << lineNumber() << endl;
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

QString
PodcastReader::readEnclosure()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && QXmlStreamReader::name() == "enclosure" );
    //TODO: need to get the url argument here
    QString url = attributes().value( "", "url").toString();
    readElementText();
    return url;
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

void PodcastReader::readUnknownElement()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() );

    debug() << "unknown element: " << QXmlStreamReader::name().toString() << endl;

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
    debug() << "redirected to: " << url.url() << endl;

}

void
PodcastReader::slotPermanentRedirection( KIO::Job * job, const KUrl & fromUrl,
        const KUrl & toUrl)
{
    DEBUG_BLOCK
    debug() << "premanently redirected to: " << toUrl.url() << endl;

}

void
PodcastReader::commitChannel()
{
    Q_ASSERT( m_channel );
    debug() << "commit Podcast Channel (as Album) " << m_channel->title() << endl;
    PodcastAlbumPtr album = PodcastAlbumPtr( m_channel );
    album->setAlbumArtist( ArtistPtr::staticCast( m_artist) );

    m_collection->acquireReadLock();
    m_collection->addAlbum( album->name(), AlbumPtr::dynamicCast( album ) );
    m_collection->addArtist( m_artist->name(), ArtistPtr::staticCast( m_artist) );
    m_collection->releaseLock();

//     emit finished( this, true );
}

void
PodcastReader::commitEpisode()
{
    Q_ASSERT( m_current );
    debug() << "commit episode " << m_current->title() << endl;
    PodcastTrackPtr item = PodcastTrackPtr( static_cast<PodcastTrack*>(m_current) );
    item->setAlbum( AlbumPtr::staticCast( PodcastAlbumPtr( m_channel ) ) );
    item->setArtist( ArtistPtr::dynamicCast( m_artist ) );
    m_artist->addTrack( TrackPtr::dynamicCast( item ) );

    m_collection->acquireReadLock();
    m_collection->addTrack( item->name(), TrackPtr::dynamicCast( item ) );
    m_collection->releaseLock();

    Q_ASSERT( m_channel );
    m_channel->addTrack( TrackPtr::staticCast( item ) );

    m_current = m_channel;
}

#include "PodcastReader.moc"
