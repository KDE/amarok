// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "config.h"

#include "scrobbler.h"
#include "collectiondb.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include <unistd.h>

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS Scrobbler
//////////////////////////////////////////////////////////////////////////////////////////

Scrobbler::Scrobbler()
{
}


Scrobbler::~Scrobbler()
{
}


void
Scrobbler::relatedArtists( QString artist )
{
    QString url = QString( "http://www.audioscrobbler.com/similar/%1" )
                     .arg( artist );

    kdDebug() << "Using this url: " << url << endl;

    m_buffer = "";

    KIO::TransferJob* job = KIO::get( url, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( audioScrobblerRelatedArtistResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( audioScrobblerRelatedArtistData( KIO::Job*, const QByteArray& ) ) );

}


void
Scrobbler::audioScrobblerRelatedArtistData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_buffer += QString( data );
}


void
Scrobbler::audioScrobblerRelatedArtistResult( KIO::Job* job ) //SLOT
{
    int x = 0;
    QStringList artists;

    if ( !job->error() == 0 )
    {
        kdWarning() << "[AudioScrobbler] KIO error! errno: " << job->error() << endl;
        return;
    }

    m_buffer = m_buffer.mid( m_buffer.find( "<div class=\"content\">" ) );
    m_buffer = m_buffer.mid( 0, m_buffer.find( "<div id=\"footer\">" ) );

    while ( m_buffer.find( "<small>[<a href=\"/similar/" ) )
    {
        if ( x++ > 10 ) break;

        m_buffer = m_buffer.mid( m_buffer.find( "<small>[<a href=\"/similar/" ) );

        QString artist;
        artist = m_buffer.mid( m_buffer.find( "/similar/" ) + 9 );
        artist = KURL::decode_string( artist.mid( 0, artist.find( "\" title" ) ) );

        kdDebug() << artist << endl;
        artists << artist;

        m_buffer = m_buffer.mid( m_buffer.find( "</td>" ) );

    }

    if ( artists.count() > 0 )
        emit relatedArtistsFetched( artists );
}


#include "scrobbler.moc"
