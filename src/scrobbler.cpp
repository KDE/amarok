// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#include "amarokconfig.h"
#include "collectiondb.h"
#include "config.h"
#include "enginecontroller.h"
#include <kapplication.h>
#include <kdebug.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <qdatetime.h>
#include "scrobbler.h"
#include <unistd.h>

//some setups require this
#undef PROTOCOL_VERSION



////////////////////////////////////////////////////////////////////////////////
// CLASS Scrobbler
////////////////////////////////////////////////////////////////////////////////


Scrobbler* Scrobbler::instance()
{
    static Scrobbler scrobbler;
    return &scrobbler;
}


Scrobbler::Scrobbler() :
    m_prevPos( 0 ),
    m_validForSending( true ),
    m_submitter( new ScrobblerSubmitter() ),
    m_item( NULL )
{
    EngineController::instance()->attach( this );
}


Scrobbler::~Scrobbler()
{
    delete m_submitter;
    if ( m_item != NULL )
    {
        delete m_item;
    }
    EngineController::instance()->detach( this );
}


/**
 * Queries similar artists from Audioscrobbler.
 */
void Scrobbler::similarArtists( QString artist )
{
    QString url = QString( "http://www.audioscrobbler.com/similar/%1" )
                     .arg( artist.utf8() );

    kdDebug() << "[AudioScrobbler] Similar artists: " << url << endl;

    m_similarArtistsBuffer = "";
    m_artist = artist;

    KIO::TransferJob* job = KIO::get( url, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( audioScrobblerSimilarArtistsResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( audioScrobblerSimilarArtistsData( KIO::Job*, const QByteArray& ) ) );

}


/**
 * Called when the similar artists TransferJob finishes.
 */
void Scrobbler::audioScrobblerSimilarArtistsResult( KIO::Job* job ) //SLOT
{
    int x = 0;
    QStringList suggestions;

    if ( job->error() )
    {
        kdWarning()
            << "[AudioScrobbler] KIO error! errno: " << job->error() << endl;
        return;
    }

    m_similarArtistsBuffer =
        m_similarArtistsBuffer.mid(
            m_similarArtistsBuffer.find( "<div class=\"content\">" ) );
    m_similarArtistsBuffer =
        m_similarArtistsBuffer.mid(
            0, m_similarArtistsBuffer.find( "<div id=\"footer\">" ) );

    while ( m_similarArtistsBuffer.find( "<small>[<a href=\"/similar/" ) )
    {
        if ( x++ > 15 ) break;

        m_similarArtistsBuffer =
            m_similarArtistsBuffer.mid(
                m_similarArtistsBuffer.find( "<small>[<a href=\"/similar/" ) );

        QString artist;
        artist =
            m_similarArtistsBuffer.mid(
                m_similarArtistsBuffer.find( "/similar/" ) + 9 );
        artist =
            KURL::decode_string( artist.mid( 0, artist.find( "\" title" ) ) );

        //kdDebug() << artist << endl;
        if ( !artist.isEmpty() ) suggestions << artist;

        m_similarArtistsBuffer =
            m_similarArtistsBuffer.mid(
                m_similarArtistsBuffer.find( "</td>" ) );

    }

    if ( suggestions.count() > 0 )
        emit relatedArtistsFetched( m_artist, suggestions );
}


/**
 * Called when similar artists data is received for the TransferJob.
 */
void Scrobbler::audioScrobblerSimilarArtistsData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_similarArtistsBuffer += QString( data );
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineNewMetaData( const MetaBundle& bundle, bool /*trackChanged*/ )
{
    m_prevPos = 0;

    // Plugins must not submit tracks played from online radio stations, even
    // if they appear to be providing correct metadata.
    if ( bundle.streamUrl() != NULL )
    {
        m_validForSending = false;
    }
    else
    {
        if ( m_item != NULL )
        {
            delete m_item;
        }
        // Songs with no artist or title data or a duration of less than
        // 30 seconds must not be submitted.
        if ( bundle.artist() != NULL &&
             bundle.title() != NULL &&
             bundle.length() >= 30 )
        {
            m_item =
                new SubmitItem(
                    bundle.artist(),
                    bundle.album(),
                    bundle.title(),
                    bundle.length() );
            m_validForSending = true;
        }
        else
        {
            m_item = NULL;
            m_validForSending = false;
        }
    }
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineTrackPositionChanged( long position )
{
    if ( !m_validForSending )
    {
        return;
    }

    long posChange = position - m_prevPos;
    // If this is not the first position changed signal for this song.
    if ( m_prevPos != 0 )
    {
        // TODO: It would be nice to have some more accurate method for
        // detecting user seek. Now use MAIN_TIMER events with 2 sec
        // tolerance.
        if ( posChange > 2000 + EngineController::MAIN_TIMER )
        {
            // Position has changed more than it would during normal
            // playback.
            m_validForSending = false;
        }
    }

    // Each track must be submitted to the server when it is 50% or 240
    // seconds complete, whichever comes first.
    if ( position > 240 * 1000 || position > 0.5 * m_item->length() * 1000 )
    {
        m_submitter->submitItem( m_item );
        m_item = NULL;
        m_validForSending = false;
    }

    m_prevPos = position;
}


/**
 * Applies settings from the config dialog.
 */
void Scrobbler::applySettings()
{
    m_submitter->setEnabled( AmarokConfig::submitPlayedSongs() );
    m_submitter->setUsername( AmarokConfig::scrobblerUsername() );
    m_submitter->setPassword( AmarokConfig::scrobblerPassword() );

    m_submitter->handshake();
}


////////////////////////////////////////////////////////////////////////////////
// CLASS ScrobblerSubmitter
////////////////////////////////////////////////////////////////////////////////


QString ScrobblerSubmitter::PROTOCOL_VERSION = "1.1";
QString ScrobblerSubmitter::CLIENT_ID = "ark";
QString ScrobblerSubmitter::CLIENT_VERSION = "0.1";
QString ScrobblerSubmitter::HANDSHAKE_URL =
    "http://post.audioscrobbler.com/?hs=true";


ScrobblerSubmitter::ScrobblerSubmitter() :
    m_username( NULL ),
    m_password( NULL ),
    m_submitUrl( NULL ),
    m_challenge( NULL ),
    m_scrobblerEnabled( false ),
    m_prevSubmitTime( 0 ),
    m_interval( 0 )
{}


ScrobblerSubmitter::~ScrobblerSubmitter()
{
    m_ongoingSubmits.setAutoDelete( TRUE );
    m_ongoingSubmits.clear();
    m_submitQueue.setAutoDelete( TRUE );
    m_submitQueue.clear();
}


/**
 * Performs handshake with Audioscrobbler.
 */
void ScrobblerSubmitter::handshake()
{
    if ( !canSubmit() )
    {
        return;
    }

    QString handshakeUrl = QString::null;
    uint currentTime = QDateTime::currentDateTime().toTime_t();

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/?hs=true
        // &p=1.1
        // &c=<clientid>
        // &v=<clientver>
        // &u=<user>
        handshakeUrl =
            HANDSHAKE_URL +
            QString(
                "&p=%1"
                "&c=%2"
                "&v=%3"
                "&u=%4" )
                .arg( PROTOCOL_VERSION )
                .arg( CLIENT_ID )
                .arg( CLIENT_VERSION )
                .arg( m_username );
    }

    else if ( PROTOCOL_VERSION == "1.2" )
    {
        // Audioscrobbler protocol 1.2 (RFC)
        // http://post.audioscrobbler.com/?hs=true
        // &p=1.2
        // &c=<clientid>
        // &v=<clientversion>
        // &u=<username>
        // &t=<unix_timestamp>
        // &a=<passcode>
        handshakeUrl =
            HANDSHAKE_URL +
            QString(
                "&p=%1"
                "&c=%2"
                "&v=%3"
                "&u=%4"
                "&t=%5"
                "&a=%6" )
                .arg( PROTOCOL_VERSION )
                .arg( CLIENT_ID )
                .arg( CLIENT_VERSION )
                .arg( m_username )
                .arg( currentTime )
                .arg( KMD5( KMD5( m_password.utf8() ).hexDigest() +
                    currentTime ).hexDigest() );
    }

    else
    {
        kdDebug()
            << "[AudioScrobbler] Handshake not implemented for protocol version: "
            << PROTOCOL_VERSION << endl;
        return;
    }

    kdDebug() << "[AudioScrobbler] Handshake url: " << handshakeUrl << endl;

    m_prevSubmitTime = currentTime;
    m_submitResultBuffer = "";

    KIO::TransferJob* job = KIO::get( handshakeUrl, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( audioScrobblerHandshakeResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( audioScrobblerSubmitData( KIO::Job*, const QByteArray& ) ) );
}


/**
 * Sets item for submission to Audioscrobbler. Actual submission
 * depends on things like (is scrobbling enabled, are Audioscrobbler
 * profile details filled in etc).
 */
void ScrobblerSubmitter::submitItem( SubmitItem* item )
{
    enqueueItem( item );
    
    if ( !canSubmit() )
    {
        return;
    }
    else if ( m_challenge.isEmpty() )
    {
        handshake();
        return;
    }

    QString data = QString::null;
    uint currentTime = QDateTime::currentDateTime().toTime_t();
    // Audioscrobbler accepts max 10 tracks on one submit.
    SubmitItem* items[10];
    for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
    {
        items[submitCounter] = 0;
    }

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/v1.1-lite.php
        // u=<user>
        // &s=<MD5 response>&
        // a[0]=<artist 0>&t[0]=<track 0>&b[0]=<album 0>&
        // m[0]=<mbid 0>&l[0]=<length 0>&i[0]=<time 0>&
        // a[1]=<artist 1>&t[1]=<track 1>&b[1]=<album 1>&
        // m[1]=<mbid 1>&l[1]=<length 1>&i[1]=<time 1>&
        // ...
        // a[n]=<artist n>&t[n]=<track n>&b[n]=<album n>&
        // m[n]=<mbid n>&l[n]=<length n>&i[n]=<time n>&
        
        
        data =
            "u=" + KURL::encode_string( m_username ) +
            "&s=" +
                KURL::encode_string( KMD5( KMD5( m_password.utf8() ).hexDigest() +
                    m_challenge.utf8() ).hexDigest() );
        
        for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
        {
            SubmitItem* itemFromQueue = dequeueItem();
            if ( itemFromQueue == 0 )
            {
                break;
            }
            else
            {
                data += "&";
            }
            items[submitCounter] = itemFromQueue;
            QDateTime playStartTime = QDateTime();
            playStartTime.setTime_t( itemFromQueue->playStartTime() );
            data +=
                "a[" + QString::number( submitCounter ) + "]=" +
                KURL::encode_string( itemFromQueue->artist().utf8() ) +
                "&t[" + QString::number( submitCounter ) + "]=" +
                KURL::encode_string( itemFromQueue->title().utf8() ) +
                "&b[" + QString::number( submitCounter ) + "]=" +
                KURL::encode_string( itemFromQueue->album().utf8() ) +
                "&m[" + QString::number( submitCounter ) + "]=" +
                "&l[" + QString::number( submitCounter ) + "]=" +
                QString::number( itemFromQueue->length() ) +
                "&i[" + QString::number( submitCounter ) + "]=" + KURL::encode_string(
                    playStartTime.toString( "yyyy-MM-dd hh:mm:ss" ) );
        }
    }

    else
    {
        kdDebug()
            << "[AudioScrobbler] Submit not implemented for protocol version: "
            << PROTOCOL_VERSION << endl;
        return;
    }

    kdDebug() << "[AudioScrobbler] Submit data: " << data << endl;

    m_prevSubmitTime = currentTime;
    m_submitResultBuffer = "";

    KIO::TransferJob* job =
        KIO::http_post( m_submitUrl, data.utf8(), false );
    job->addMetaData(
        "content-type", "Content-Type: application/x-www-form-urlencoded" );
    // Loop in reverse order, which helps when items are later fetched from
    // m_ongoingSubmits and possibly put back to queue, in correct order
    // (i.e. oldest first).
    for ( int submitCounter = 9; submitCounter >= 0; submitCounter-- )
    {
        if ( items[submitCounter] != 0 )
        {
            m_ongoingSubmits.insert( job, items[submitCounter] );
        }
    }
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( audioScrobblerSubmitResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( audioScrobblerSubmitData( KIO::Job*, const QByteArray& ) ) );
}


/**
 * Sets Audioscrobbler profile username.
 */
void ScrobblerSubmitter::setUsername( const QString& username )
{
    m_username = username;
}


/**
 * Sets Audioscrobbler profile password.
 */
void ScrobblerSubmitter::setPassword( const QString& password )
{
    m_password = password;
}


/**
 * Sets whether scrobbling is enabled.
 */
void ScrobblerSubmitter::setEnabled( bool enabled )
{
    m_scrobblerEnabled = enabled;
}


/**
 * Called when handshake TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerHandshakeResult( KIO::Job* job ) //SLOT
{
    if ( job->error() )
    {
        kdWarning()
            << "[AudioScrobbler] KIO error! errno: " << job->error() << endl;
        return;
    }

//     kdDebug()
//         << "[AudioScrobbler] Handshake result received: "
//         << endl << m_submitResultBuffer << endl;

    // UPTODATE
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    if (m_submitResultBuffer.startsWith( "UPTODATE" ) )
    {
        m_challenge = m_submitResultBuffer.section( "\n", 1, 1 );
        m_submitUrl = m_submitResultBuffer.section( "\n", 2, 2 );
        QString interval = m_submitResultBuffer.section( "\n", 3, 3 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
    }
    // UPDATE <updateurl (optional)>
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "UPDATE" ) )
    {
        kdWarning()
            << "[AudioScrobbler] A new version of amaroK is available"
            << endl;
        m_challenge = m_submitResultBuffer.section( "\n", 1, 1 );
        m_submitUrl = m_submitResultBuffer.section( "\n", 2, 2 );
        QString interval = m_submitResultBuffer.section( "\n", 3, 3 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.find( "\n" ) );
        if ( reason.length() > 6 )
        {
            reason = reason.mid( 7 ).stripWhiteSpace();
        }
        kdWarning()
            << "[AudioScrobbler] Handshake failed (" << reason << ")"
            << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
    }
    // BADUSER (protocol 1.1) or BADAUTH (protocol 1.2)
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADUSER" ) ||
              m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        kdWarning()
            << "[AudioScrobbler] Handshake failed (Authentication failed)"
            << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
    }
    else
    {
        kdWarning() << "[AudioScrobbler] Unknown handshake response" << endl;
    }

    kdDebug()
        << "[AudioScrobbler] Handshake result parsed: challenge=" << m_challenge
        << ", submitUrl=" << m_submitUrl << endl;
}


/**
 * Called when submit TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerSubmitResult( KIO::Job* job ) //SLOT
{
    if ( job->error() )
    {
        kdWarning()
            << "[AudioScrobbler] KIO error! errno: " << job->error() << endl;
        enqueueJob( job );
        return;
    }

//     kdDebug()
//         << "[AudioScrobbler] Submit result received: "
//         << endl << m_submitResultBuffer << endl;

    // OK
    // INTERVAL n (protocol 1.1)
    if (m_submitResultBuffer.startsWith( "OK" ) )
    {
        kdDebug() << "[AudioScrobbler] Submit successful" << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
        finishJob( job );
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason =
            m_submitResultBuffer.mid(
                0, m_submitResultBuffer.find( "\n" ) );
        if ( reason.length() > 6 )
        {
            reason = reason.mid( 7 ).stripWhiteSpace();
        }
        kdWarning()
            << "[AudioScrobbler] Submit failed (" << reason << ")"
            << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
        enqueueJob( job );
    }
    // BADAUTH
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        kdWarning()
            << "[AudioScrobbler] Submit failed (Authentication failed)"
            << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
        {
            m_interval = interval.mid( 9 ).toUInt();
        }
        enqueueJob( job );
        handshake();
    }
    else
    {
        kdWarning() << "[AudioScrobbler] Unknown submit response" << endl;
    }
}


/**
 * Receives the data from the TransferJob.
 */
void ScrobblerSubmitter::audioScrobblerSubmitData(
    KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_submitResultBuffer += QString( data );
}


/**
 * Checks if it is possible to try to submit the data to Audioscrobbler.
 */
bool ScrobblerSubmitter::canSubmit() const
{
    if ( !m_scrobblerEnabled )
    {
        return false;
    }
    else if ( m_username.isEmpty() )
    {
        return false;
    }
    else if ( m_password.isEmpty() )
    {
        return false;
    }

    if ( m_interval != 0 )
    {
        uint currentTime = QDateTime::currentDateTime().toTime_t();
        if ( ( currentTime - m_prevSubmitTime ) < m_interval )
        {
            // Not enough time passed since previous handshake/submit
            return false;
        }
    }

    return true;
}


/**
 * Enqueues the given item for later submission.
 */
void ScrobblerSubmitter::enqueueItem( SubmitItem* item )
{
    // Maintain max size of the queue, Audioscrobbler won't accept too old
    // submissions anyway.
    for ( uint size = m_submitQueue.count(); size >= 50; size-- )
    {
        SubmitItem* itemFromQueue = dequeueItem();
        kdDebug()
            << "[Audioscrobbler] Dropping " << itemFromQueue->artist()
            << " - " << itemFromQueue->title() << " from submit queue" << endl;
        delete itemFromQueue;
    }
    
    m_submitQueue.enqueue( item );
}


/**
 * Dequeues one item from the queue.
 */
SubmitItem* ScrobblerSubmitter::dequeueItem()
{
    SubmitItem* item = m_submitQueue.dequeue();
    
    return item;
}


/**
 * Enqueues items associated with the job. This is used when the job
 * has failed (e.g. network problems).
 */
void ScrobblerSubmitter::enqueueJob( KIO::Job* job )
{
    SubmitItem* item;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        enqueueItem( item );
    }
}


/**
 * Deletes items associated with the job. This is used when the job
 * has succeeded.
 */
void ScrobblerSubmitter::finishJob( KIO::Job* job )
{
    SubmitItem* item;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        delete item;
    }
}


////////////////////////////////////////////////////////////////////////////////
// CLASS SubmitItem
////////////////////////////////////////////////////////////////////////////////


SubmitItem::SubmitItem(
    const QString& artist,
    const QString& album,
    const QString& title,
    int length)
{
    m_artist = artist;
    m_album = album;
    m_title = title;
    m_length = length;
    m_playStartTime = QDateTime::currentDateTime( Qt::UTC ).toTime_t();
}


#include "scrobbler.moc"
