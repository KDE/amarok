// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// See COPYING file for licensing information.

#define DEBUG_PREFIX "Scrobbler"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "config.h"
#include "debug.h"
#include "enginecontroller.h"
#include "playlist.h"
#include "scrobbler.h"
#include "statusbar.h"

#include <unistd.h>

#include <qdatetime.h>

#include <kapplication.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <kurl.h>

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


Scrobbler::Scrobbler()
    : EngineObserver( EngineController::instance() )
    , m_similarArtistsJob( 0 )
    , m_validForSending( true )
    , m_submitter( new ScrobblerSubmitter() )
    , m_item( 0 )
{}


Scrobbler::~Scrobbler()
{
    delete m_submitter;
    delete m_item;
}


/**
 * Queries similar artists from Audioscrobbler.
 */
void Scrobbler::similarArtists( const QString & artist )
{
    if ( AmarokConfig::retrieveSimilarArtists() )
    {
//         Request looks like this:
//         <?xml version="1.0"?>
//         <methodCall>
//             <methodName>getSimilarArtists</methodName>
//             <params>
//                 <param>
//                     <value>
//                         <string>Pearl Jam</string>
//                     </value>
//                 </param>
//                 <param>
//                     <value>
//                         <int>30</int>
//                     </value>
//                 </param>
//             </params>
//         </methodCall>

        QDomDocument reqdoc;

        QDomElement methodCall = reqdoc.createElement( "methodCall" );
        QDomElement methodName = reqdoc.createElement( "methodName" );
        QDomText methodNameValue = reqdoc.createTextNode( "getSimilarArtists" );
        methodName.appendChild( methodNameValue );
        methodCall.appendChild( methodName );

        QDomElement params = reqdoc.createElement( "params" );
        QDomElement param1 = reqdoc.createElement( "param" );
        QDomElement value1 = reqdoc.createElement( "value" );
        QDomElement type1 = reqdoc.createElement( "string" );
        QDomText param1Value = reqdoc.createTextNode( artist );
        type1.appendChild( param1Value );
        value1.appendChild( type1 );
        param1.appendChild( value1 );
        params.appendChild( param1 );

        QDomElement param2 = reqdoc.createElement( "param" );
        QDomElement value2 = reqdoc.createElement( "value" );
        QDomElement type2 = reqdoc.createElement( "int" );
        QDomText param2Value = reqdoc.createTextNode( "30" );
        type2.appendChild( param2Value );
        value2.appendChild( type2 );
        param2.appendChild( value2 );
        params.appendChild( param2 );

        methodCall.appendChild( params );
        reqdoc.appendChild( methodCall );

        const QString xmlRequest = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + reqdoc.toString();

        QByteArray postData;
        QDataStream stream( postData, IO_WriteOnly );
        stream.writeRawBytes( xmlRequest.utf8(), xmlRequest.length() );

        m_similarArtistsBuffer = QString::null;
        m_artist = artist;

        m_similarArtistsJob = KIO::http_post( "http://ws.audioscrobbler.com/xmlrpc", postData, false );
        m_similarArtistsJob->addMetaData( "content-type", "Content-Type: text/xml" );

        connect( m_similarArtistsJob, SIGNAL( result( KIO::Job* ) ),
                 this,                  SLOT( audioScrobblerSimilarArtistsResult( KIO::Job* ) ) );
        connect( m_similarArtistsJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 this,                  SLOT( audioScrobblerSimilarArtistsData( KIO::Job*, const QByteArray& ) ) );
    }
}


/**
 * Called when the similar artists TransferJob finishes.
 */
void Scrobbler::audioScrobblerSimilarArtistsResult( KIO::Job* job ) //SLOT
{
    if ( m_similarArtistsJob != job )
        return; //not the right job, so let's ignore it

    if ( job->error() )
    {
        warning() << "KIO error! errno: " << job->error() << endl;
        return;
    }

//     Result looks like this:
//     <?xml version="1.0" encoding="UTF-8"?>
//     <methodResponse>
//         <params>
//             <param>
//                 <value><array>
//                     <data>
//                         <value><string>Barenaked Ladies</string></value>
//                         <value><string>Talking Heads</string></value>
//                         <value><string>Bob Marley &amp; The Wailers</string></value>
//                         <value><string>Sting</string></value>
//                         <value><string>???</string></value>
//                         <value><string>Massive Attack</string></value>
//                         <value><string>Alt for Egil</string></value>
//                         <value><string>Paul Simon</string></value>
//                         <value><string>Joe Firstman</string></value>
//                     </data>
//                 </array></value>
//             </param>
//         </params>
//     </methodResponse>

    QDomDocument document;
    if ( !document.setContent( m_similarArtistsBuffer ) )
    {
        debug() << "Couldn't read similar artists response" << endl;
        return;
    }

    QDomNodeList values =
        document.elementsByTagName( "methodResponse" ).item( 0 )
            .namedItem( "params" ).namedItem( "param" ).namedItem( "value" )
            .namedItem( "array" ).namedItem( "data" ).childNodes();

    QStringList suggestions;
    for ( uint i = 0; i < values.count(); i++ )
    {
        QDomNode item = values.item( i );
        QString artist = item.namedItem( "string" ).toElement().text();
        //debug() << "Suggestion: " << artist << endl;
        suggestions << artist;
    }

    debug() << "Suggestions retrieved (" << suggestions.count() << ")" << endl;
    if ( !suggestions.isEmpty() )
        emit similarArtistsFetched( m_artist, suggestions );

    m_similarArtistsJob = 0;
}


/**
 * Called when similar artists data is received for the TransferJob.
 */
void Scrobbler::audioScrobblerSimilarArtistsData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_similarArtistsBuffer += QString::fromUtf8( data, data.size() );
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineNewMetaData( const MetaBundle& bundle, bool trackChanged )
{
    if ( !trackChanged )
    {
        // Tags were changed, update them if not yet submitted.
        // TODO: In this case submit could be enabled if the artist or title
        // tag was missing initially and disabled submit
        debug() << "It's still the same track." << endl;
        if ( m_item )
        {
            m_item->setArtist( bundle.artist() );
            m_item->setAlbum( bundle.album() );
            m_item->setTitle( bundle.title() );
        }
        return;
    }

    // Plugins must not submit tracks played from online radio stations, even
    // if they appear to be providing correct metadata.
    if ( bundle.streamUrl() != NULL ) {
        debug() << "Won't submit: It's a stream." << endl;
        m_validForSending = false;
    }
    else
    {
        delete m_item;

        // Songs with no artist or title data or a duration of less than
        // 30 seconds must not be submitted.
        if ( bundle.artist() != NULL && bundle.title() != NULL && bundle.length() >= 30 )
        {
            m_item = new SubmitItem( bundle.artist(), bundle.album(), bundle.title(), bundle.length() );
            m_validForSending = true;
        }
        else
        {
            m_item = 0;
            m_validForSending = false;
            debug() << "Won't submit: No artist, no title, or less than 30 seconds." << endl;;
        }
    }
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineTrackPositionChanged( long position, bool userSeek )
{
    if ( !m_validForSending || !m_item )
        return;

    if ( userSeek )
    {
        m_validForSending = false;
        debug() << "Won't submit: Seek detected." << endl;
        return;
    }

    // Each track must be submitted to the server when it is 50% or 240
    // seconds complete, whichever comes first.
    if ( position > 240 * 1000 || position > 0.5 * m_item->length() * 1000 )
    {
        m_submitter->submitItem( m_item );
        m_item = 0;
        m_validForSending = false;
    }
}


/**
 * Applies settings from the config dialog.
 */
void Scrobbler::applySettings()
{
    bool handshakeNeeded = false;
    if ( m_submitter->username() != AmarokConfig::scrobblerUsername() )
    {
        handshakeNeeded = true;
    }
    else if ( m_submitter->password() != AmarokConfig::scrobblerPassword() )
    {
        handshakeNeeded = true;
    }

    m_submitter->setEnabled( AmarokConfig::submitPlayedSongs() );
    m_submitter->setUsername( AmarokConfig::scrobblerUsername() );
    m_submitter->setPassword( AmarokConfig::scrobblerPassword() );

    if ( handshakeNeeded )
        m_submitter->handshake();
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


SubmitItem::SubmitItem( const QDomElement& element )
{
    m_artist = element.namedItem( "artist" ).toElement().text();
    m_album = element.namedItem( "album" ).toElement().text();
    m_title = element.namedItem( "title" ).toElement().text();
    m_length = element.namedItem( "length" ).toElement().text().toInt();
    m_playStartTime = element.namedItem( "playtime" ).toElement().text().toUInt();
}


bool SubmitItem::operator==( const SubmitItem& item )
{
    bool result = true;

    if ( m_artist != item.artist() || m_album != item.album() || m_title != item.title() ||
         m_length != item.length() || m_playStartTime != item.playStartTime() )
    {
        result = false;
    }

    return result;
}


QDomElement SubmitItem::toDomElement( QDomDocument& document ) const
{
    QDomElement item = document.createElement( "item" );
    // TODO: In the future, it might be good to store url too
    //item.setAttribute("url", item->url().url());

    QDomElement artist = document.createElement( "artist" );
    QDomText artistText = document.createTextNode( m_artist );
    artist.appendChild( artistText );
    item.appendChild( artist );

    QDomElement album = document.createElement( "album" );
    QDomText albumText = document.createTextNode( m_album );
    album.appendChild( albumText );
    item.appendChild( album );

    QDomElement title = document.createElement( "title" );
    QDomText titleText = document.createTextNode( m_title );
    title.appendChild( titleText );
    item.appendChild( title );

    QDomElement length = document.createElement( "length" );
    QDomText lengthText = document.createTextNode( QString::number( m_length ) );
    length.appendChild( lengthText );
    item.appendChild( length );

    QDomElement playtime = document.createElement( "playtime" );
    QDomText playtimeText = document.createTextNode( QString::number( m_playStartTime ) );
    playtime.appendChild( playtimeText );
    item.appendChild( playtime );

    return item;
}


////////////////////////////////////////////////////////////////////////////////
// CLASS SubmitQueue
////////////////////////////////////////////////////////////////////////////////


int SubmitQueue::compareItems( QPtrCollection::Item item1, QPtrCollection::Item item2 )
{
    SubmitItem *sItem1 = (SubmitItem*) item1;
    SubmitItem *sItem2 = (SubmitItem*) item2;
    int result;

    if ( sItem1 == sItem2 )
    {
        result = 0;
    }
    else if ( sItem1->playStartTime() > sItem2->playStartTime() )
    {
        result = 1;
    }
    else
    {
        result = -1;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////
// CLASS ScrobblerSubmitter
////////////////////////////////////////////////////////////////////////////////

QString ScrobblerSubmitter::PROTOCOL_VERSION = "1.1";
QString ScrobblerSubmitter::CLIENT_ID = "ark";
QString ScrobblerSubmitter::CLIENT_VERSION = "0.1";
QString ScrobblerSubmitter::HANDSHAKE_URL = "http://post.audioscrobbler.com/?hs=true";


ScrobblerSubmitter::ScrobblerSubmitter()
    : m_username( 0 )
    , m_password( 0 )
    , m_submitUrl( 0 )
    , m_challenge( 0 )
    , m_scrobblerEnabled( false )
    , m_prevSubmitTime( 0 )
    , m_interval( 0 )
{
    readSubmitQueue();
}


ScrobblerSubmitter::~ScrobblerSubmitter()
{
    saveSubmitQueue();

    m_ongoingSubmits.setAutoDelete( true );
    m_ongoingSubmits.clear();
    m_submitQueue.setAutoDelete( true );
    m_submitQueue.clear();
}


/**
 * Performs handshake with Audioscrobbler.
 */
void ScrobblerSubmitter::handshake()
{
    if ( !canSubmit() )
        return;

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
        debug() << "Handshake not implemented for protocol version: " << PROTOCOL_VERSION << endl;
        return;
    }

    debug() << "Handshake url: " << handshakeUrl << endl;

    m_prevSubmitTime = currentTime;
    m_submitResultBuffer = "";

    KIO::TransferJob* job = KIO::storedGet( handshakeUrl, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( audioScrobblerHandshakeResult( KIO::Job* ) ) );
}


/**
 * Sets item for submission to Audioscrobbler. Actual submission
 * depends on things like (is scrobbling enabled, are Audioscrobbler
 * profile details filled in etc).
 */
void ScrobblerSubmitter::submitItem( SubmitItem* item )
{
    if ( !canSubmit() )
    {
        if ( m_scrobblerEnabled )
        {
            // If scrobbling is enabled but can't submit for some reason,
            // enqueue item.
            enqueueItem( item );
            announceSubmit( item, 1, false );
        }
        return;
    }
    else if ( m_challenge.isEmpty() )
    {
        enqueueItem( item );
        announceSubmit( item, 1, false );
        handshake();
        return;
    }
    else
        enqueueItem( item );

    QString data;
    uint currentTime = QDateTime::currentDateTime().toTime_t();
    // Audioscrobbler accepts max 10 tracks on one submit.
    SubmitItem* items[10];
    for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
        items[submitCounter] = 0;

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
            "u=" + KURL::encode_string_no_slash( m_username ) +
            "&s=" +
                KURL::encode_string_no_slash( KMD5( KMD5( m_password.utf8() ).hexDigest() +
                    m_challenge.utf8() ).hexDigest() );

        m_submitQueue.first();
        for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
        {
            SubmitItem* itemFromQueue = dequeueItem();
            if ( itemFromQueue == 0 )
                break;
            else
                data += "&";

            items[submitCounter] = itemFromQueue;
            QDateTime playStartTime = QDateTime();
            playStartTime.setTime_t( itemFromQueue->playStartTime() );

            const QString count = QString::number( submitCounter );

            data +=
                "a["  + count + "]=" + KURL::encode_string_no_slash( itemFromQueue->artist(), 106 /*utf-8*/ ) +
                "&t[" + count + "]=" + KURL::encode_string_no_slash( itemFromQueue->title(), 106 /*utf-8*/ ) +
                "&b[" + count + "]=" + KURL::encode_string_no_slash( itemFromQueue->album(), 106 /*utf-8*/ ) +
                "&m[" + count + "]=" +
                "&l[" + count + "]=" + QString::number( itemFromQueue->length() ) +
                "&i[" + count + "]=" + KURL::encode_string_no_slash( playStartTime.toString( "yyyy-MM-dd hh:mm:ss" ) );
        }
    }

    else
    {
        debug() << "Submit not implemented for protocol version: " << PROTOCOL_VERSION << endl;
        return;
    }

    debug() << "Submit data: " << data << endl;

    m_prevSubmitTime = currentTime;
    m_submitResultBuffer = "";

    KIO::TransferJob* job = KIO::http_post( m_submitUrl, data.utf8(), false );
    job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );

    // Loop in reverse order, which helps when items are later fetched from
    // m_ongoingSubmits and possibly put back to queue, in correct order
    // (i.e. oldest first).
    for ( int submitCounter = 9; submitCounter >= 0; submitCounter-- )
        if ( items[submitCounter] != 0 )
            m_ongoingSubmits.insert( job, items[submitCounter] );

    amaroK::StatusBar::instance()->newProgressOperation( job )
            .setDescription( i18n( "Submitting to last.fm" ) );

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

    if ( !enabled )
    {
        // If submit is disabled, clear submitqueue.
        m_ongoingSubmits.setAutoDelete( true );
        m_ongoingSubmits.clear();
        m_ongoingSubmits.setAutoDelete( false );
        m_submitQueue.setAutoDelete( true );
        m_submitQueue.clear();
        m_submitQueue.setAutoDelete( false );
    }
}


/**
 * Called when handshake TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerHandshakeResult( KIO::Job* job ) //SLOT
{
    if ( job->error() ) {
        warning() << "KIO error! errno: " << job->error() << endl;
        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_submitResultBuffer = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

//     debug()
//         << "Handshake result received: "
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
            m_interval = interval.mid( 9 ).toUInt();
    }
    // UPDATE <updateurl (optional)>
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "UPDATE" ) )
    {
        warning() << "A new version of amaroK is available" << endl;

        m_challenge = m_submitResultBuffer.section( "\n", 1, 1 );
        m_submitUrl = m_submitResultBuffer.section( "\n", 2, 2 );
        QString interval = m_submitResultBuffer.section( "\n", 3, 3 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.find( "\n" ) );
        if ( reason.length() > 6 )
            reason = reason.mid( 7 ).stripWhiteSpace();

        warning() << "Handshake failed (" << reason << ")" << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    // BADUSER (protocol 1.1) or BADAUTH (protocol 1.2)
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADUSER" ) ||
              m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        warning() << "Handshake failed (Authentication failed)" << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    else
        warning() << "Unknown handshake response: " << m_submitResultBuffer << endl;

    debug() << "Handshake result parsed: challenge=" << m_challenge << ", submitUrl=" << m_submitUrl << endl;
}


/**
 * Called when submit TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerSubmitResult( KIO::Job* job ) //SLOT
{
    if ( job->error() ) {
        warning() << "KIO error! errno: " << job->error() << endl;
        enqueueJob( job );
        return;
    }

//     debug()
//         << "Submit result received: "
//         << endl << m_submitResultBuffer << endl;

    // OK
    // INTERVAL n (protocol 1.1)
    if (m_submitResultBuffer.startsWith( "OK" ) )
    {
        debug() << "Submit successful" << endl;
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        finishJob( job );
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.find( "\n" ) );
        if ( reason.length() > 6 )
            reason = reason.mid( 7 ).stripWhiteSpace();

        warning() << "Submit failed (" << reason << ")" << endl;

        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        enqueueJob( job );
    }
    // BADAUTH
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        warning() << "Submit failed (Authentication failed)" << endl;

        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        enqueueJob( job );
        handshake();
    }
    else
    {
        warning() << "Unknown submit response" << endl;
        enqueueJob( job );
    }
}


/**
 * Receives the data from the TransferJob.
 */
void ScrobblerSubmitter::audioScrobblerSubmitData(
    KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_submitResultBuffer += QString::fromUtf8( data, data.size() );
}


/**
 * Checks if it is possible to try to submit the data to Audioscrobbler.
 */
bool ScrobblerSubmitter::canSubmit() const
{
    if ( !m_scrobblerEnabled || m_username.isEmpty() || m_password.isEmpty() )
        return false;

    if ( m_interval != 0 )
    {
        uint currentTime = QDateTime::currentDateTime().toTime_t();
        if ( ( currentTime - m_prevSubmitTime ) < m_interval )
            // Not enough time passed since previous handshake/submit
            return false;
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
    m_submitQueue.first();
    for ( uint size = m_submitQueue.count(); size >= 500; size-- )
    {
        SubmitItem* itemFromQueue = m_submitQueue.getFirst();
        m_submitQueue.removeFirst();
        debug() << "Dropping " << itemFromQueue->artist()
                  << " - " << itemFromQueue->title() << " from submit queue" << endl;

        delete itemFromQueue;
    }

    m_submitQueue.inSort( item );

    // Save submit queue to disk so it is more uptodate in case of crash.
    saveSubmitQueue();
}


/**
 * Dequeues one item from the queue.
 */
SubmitItem* ScrobblerSubmitter::dequeueItem()
{
    SubmitItem* item = m_submitQueue.take();

    return item;
}


/**
 * Enqueues items associated with the job. This is used when the job
 * has failed (e.g. network problems).
 */
void ScrobblerSubmitter::enqueueJob( KIO::Job* job )
{
    SubmitItem *lastItem = 0;
    SubmitItem *item = 0;
    int counter = 0;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        counter++;
        lastItem = item;
        enqueueItem( item );
    }
    m_submitQueue.first();

    announceSubmit( lastItem, counter, false );
}


/**
 * Deletes items associated with the job. This is used when the job
 * has succeeded.
 */
void ScrobblerSubmitter::finishJob( KIO::Job* job )
{
    SubmitItem *firstItem = 0;
    SubmitItem *item = 0;
    int counter = 0;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        counter++;
        if ( firstItem == 0 )
            firstItem = item;
        else
            delete item;

        m_submitQueue.remove( item );
    }
    m_submitQueue.first();

    announceSubmit( firstItem, counter, true );
    delete firstItem;
}


/**
 * Announces on StatusBar if the submit was successful or not.
 *
 * @param item One of the items
 * @param tracks Amount of tracks that were submitted
 * @param success Indicates if the submission was successful or not
 */
void ScrobblerSubmitter::announceSubmit(
    SubmitItem *item, int tracks, bool success ) const
{
    QString _long, _short;

    if ( success )
    {
        if ( tracks == 1 )
            _short = i18n( "'%1' submitted to last.fm" ).arg( item->title() );
        else
        {
            _short = i18n( "Several tracks submitted to last.fm" );

            _long = "<p>";
            _long  = i18n( "'%1' and one other track submitted",
                           "'%1' and %n other tracks submitted", tracks-1 )
                            .arg( item->title() );
        }
    }
    else
    {
        if ( tracks == 1 )
            _short = i18n( "Failed to submit '%1' to last.fm" ).arg( item->title() );
        else
        {
            _short = i18n( "Failed to submit several tracks to last.fm" );
            _long  = "<p>";
            _long  = i18n( "Failed to submit '%1' and one other track",
                           "Failed to submit '%1' and %n other tracks", tracks-1 )
                      .arg( item->title() );
        }
    }

    if ( m_submitQueue.count() > 0 )
    {
        _long += "<p>";
        _long += i18n( "One track still in queue", "%n tracks still in queue", m_submitQueue.count() );
    }

    amaroK::StatusBar::instance()->shortLongMessage( _short, _long );
}


void ScrobblerSubmitter::saveSubmitQueue()
{
    QFile file( m_savePath );

    if( !file.open( IO_WriteOnly ) )
    {
        debug() << "[SCROBBLER] Couldn't write submit queue to file: " << m_savePath << endl;
        return;
    }

    QDomDocument newdoc;
    QDomElement submitQueue = newdoc.createElement( "submit" );
    submitQueue.setAttribute( "product", "amaroK" );
    submitQueue.setAttribute( "version", APP_VERSION );

    m_submitQueue.first();
    for ( uint idx = 0; idx < m_submitQueue.count(); idx++ )
    {
        SubmitItem *item = m_submitQueue.at( idx );
        QDomElement i = item->toDomElement( newdoc );
        submitQueue.appendChild( i );
    }

    QDomNode submitNode = newdoc.importNode( submitQueue, true );
    newdoc.appendChild( submitNode );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
    file.close();
}


void ScrobblerSubmitter::readSubmitQueue()
{
    m_savePath = amaroK::saveLocation() + "submit.xml";
    QFile file( m_savePath );

    if ( !file.open( IO_ReadOnly ) )
    {
        debug() << "Couldn't open file: " << m_savePath << endl;
        return;
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    if( !d.setContent( stream.read() ) )
    {
        debug() << "Couldn't read file: " << m_savePath << endl;
        return;
    }

    const QString ITEM( "item" ); //so we don't construct these QStrings all the time

    for( QDomNode n = d.namedItem( "submit" ).firstChild(); !n.isNull() && n.nodeName() == ITEM; n = n.nextSibling() )
        enqueueItem( new SubmitItem( n.toElement() ) );

    m_submitQueue.first();
}


#include "scrobbler.moc"
