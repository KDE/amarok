/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#include "LabelsEngine.h"

// Amarok
#include "core/support/Amarok.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "core/support/Debug.h"
#include "EngineController.h"

// KDE
#include <KIO/Job>
#include <KLocale>

// Qt
#include <QDomDocument>


#define DEBUG_PREFIX "LabelsEngine"

using namespace Context;


LabelsEngine::LabelsEngine( QObject *parent, const QList<QVariant> &args )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_jobLastFm( 0 )
        , m_numLastFm( -1 )
        , m_requested( true )
        , m_reload( false )
{
    Q_UNUSED( args )
    m_sources << "lastfm" ;
    m_try = 0;
    update();
}

LabelsEngine::~LabelsEngine()
{
    DEBUG_BLOCK
    m_labels.clear();
}

QStringList 
LabelsEngine::sources() const
{
    return m_sources;
}

bool 
LabelsEngine::sourceRequestEvent( const QString &name )
{
    DEBUG_BLOCK
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // we've been notified by the applet to be in state stop
    if ( tokens.contains( "stopped" ) && tokens.size() > 1 )
    {
        if ( tokens.at( 1 ) == QString( "stopped" ) )
        {
            removeSource( "labels" );
            m_reload = true;
            return false;
        }
    }
    // we've been notified by the applet to reload the labels
    else if ( tokens.contains( "reload" ) && tokens.size() > 1 )
    {
        if ( tokens.at( 1 ) == QString( "reload" ) )
        {
            m_reload = true;
        }
    }
    
    removeAllData( name );
    setData( name, QVariant() );
    m_try = 0;
    update();
    return true;
}

void 
LabelsEngine::message( const ContextState &state )
{
    if ( state == Current && m_requested )
    {
        m_try = 0;
        update();
    }
}

void 
LabelsEngine::metadataChanged( Meta::TrackPtr track )
{
    const bool hasChanged = track->artist()->name() != m_artist || track->name() != m_title;
    if ( hasChanged )
    {
        m_try = 0;
        update();
    }
}

void
LabelsEngine::update()
{
    DEBUG_BLOCK
    QStringList separators;
    // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    QString currentArtist;
    QString currentTitle;
    if ( m_try == 0 )
    {
        currentArtist = ( currentTrack && currentTrack->artist() ) ? currentTrack->artist()->name() : "";
        currentTitle = currentTrack ? currentTrack->name() : "";
    }
    else if ( m_try == 1 )
    {
        currentArtist = m_artist;
        separators.clear();
        separators << " (" << " [" << " -" << " featuring" << " feat." << " ft." << "/";
        foreach( const QString &separator, separators )
        {
            if( m_title.contains(separator,Qt::CaseInsensitive) )
            {
                currentTitle = m_title.left( m_title.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        if ( currentTitle == m_title && !m_reload )
        {
            m_try++;
            update();
            return;
        }
    }
    else if ( m_try == 2 )
    {
        separators.clear();
        separators << " vs." << " &" << " featuring" << " feat." << " ft." << ", " << " and " << "/";
        foreach( const QString &separator, separators )
        {
            if( m_artist.contains(separator,Qt::CaseInsensitive) )
            {
                currentArtist = m_title.left( m_artist.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        separators.clear();
        separators << " (" << " [" << " -" << " featuring" << " feat." << " ft." << "/";
        foreach( const QString &separator, separators )
        {
            if( m_title.contains(separator,Qt::CaseInsensitive) )
            {
                currentTitle = m_title.left( m_title.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
    }
    if ( currentArtist == "" || currentTitle == "" )
        return;
    else if ( currentArtist == m_artist && currentTitle == m_title && !m_reload )
        return;
    else
    {
        m_reload = false;
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = currentTrack;
        subscribeTo( currentTrack );

        if ( !currentTrack )
            return;

        // Save artist and title
        if ( m_try == 0 )
        {
            m_artist = currentArtist;
            m_title = currentTitle;
        }

        m_numLastFm = -1;
            
        removeAllData( "labels" );

        qDeleteAll( m_labels );
        m_labels.clear();
        
        // Show the information
        if( !currentArtist.isEmpty() && !currentTitle.isEmpty() ) // NOTE redundent if clause
        {
            setData( "labels", "message", "Fetching");
            setData( "labels", "artist", currentArtist );
            setData( "labels", "title", currentTitle );
        }
        else
        {
            debug() << "No Labels found; artist or title is empty";
            setData( "labels", "message", i18n( "No information found..." ) );
            resultFinalize();
            return;
        }

        // Query lastfm
        KUrl lastFmUrl;
        lastFmUrl.setScheme( "http" );
        lastFmUrl.setHost( "ws.audioscrobbler.com" );
        lastFmUrl.setPath( "/2.0/" );
        lastFmUrl.addQueryItem( "method", "track.gettoptags" );
        lastFmUrl.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
        lastFmUrl.addQueryItem( "artist", currentArtist.toLocal8Bit() );
        lastFmUrl.addQueryItem( "track", currentTitle.toLocal8Bit() );
        
        debug()<< "last.fm : " << lastFmUrl.toMimeDataString() ;
        m_jobLastFm = KIO::storedGet( lastFmUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( m_jobLastFm, SIGNAL( result( KJob * ) ), SLOT( resultLastFm( KJob * ) ) );
    }
}

void LabelsEngine::resultLastFm( KJob *job )
{

    if ( !m_jobLastFm ) //track changed while we were fetching
        return;

    DEBUG_BLOCK
    if ( job->error() != KJob::NoError && job == m_jobLastFm ) // It's the correct job but it errored out
    {
        setData( "labels", "message", i18n( "Unable to retrieve from last.fm") );
        debug() << "Unable to retrieve last.fm information: " << job->errorString();
        m_jobLastFm = 0; // clear job
        m_numLastFm = 0; //say that we didn't fetch any labels (which is true !)
        resultFinalize();
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
    QDomElement topElement = xmlDoc.elementsByTagName("toptags").at(0).toElement();
    QDomNodeList xmlNodeList = topElement.elementsByTagName( "tag" );

    QTime tim, time( 0, 0 );
    m_numLastFm = 0;
    for ( uint i = 0; i < xmlNodeList.length(); i++ )
    {
        // Get all the information
        QDomElement nd = xmlNodeList.at( i ).toElement();
        QDomElement nameElement = nd.elementsByTagName("name").at(0).toElement();
        QString name = nameElement.text().toLower();
        QDomElement countElement = nd.elementsByTagName("count").at(0).toElement();
        int count = countElement.text().toInt();
        debug() << name << " (" << count << ")";
        if( name != m_artist.toLower() && name != m_title.toLower() && name.length() <= 40 )
        {
            // repare the new labels info
            LabelsInfo *item = new LabelsInfo;
            item->name = name;
            item->count = count;
            // Insert the item in the list
            m_labels += item;
            m_numLastFm++;
        }
    }
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "last.fm fetch : " << m_numLastFm << " labels ";
    
    m_jobLastFm = 0;
    resultFinalize();
}

void LabelsEngine::resultFinalize()
{
    if ( m_numLastFm == 0 )
    {
        DEBUG_BLOCK
        debug() << "No Labels found";
        setData( "labels", "message", i18n( "No information found..." ) );
        if( m_try < 2 )
        {
            m_try++;
            update();
        }
        return;
    }

    if ( m_numLastFm == -1 )
        return;

    if ( !m_labels.empty() )
    {
        // remove previous message
        removeData( "labels", "message" );

        // if the song hasn't change while fetching, we sent the info
        if ( m_currentTrack != The::engineController()->currentTrack() )
            return;

        QVariant var;
        var.setValue< QList< LabelsInfo *> > ( m_labels );
        setData( "labels", "data", var );
    }
}

#include "LabelsEngine.moc"

