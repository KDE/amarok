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

#define DEBUG_PREFIX "LabelsEngine"

#include "LabelsEngine.h"

#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include "core/support/Debug.h"
#include "core/collections/MetaQueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KIO/Job>
#include <KLocale>

#include <QDomDocument>

using namespace Context;

LabelsEngine::LabelsEngine( QObject *parent, const QList<QVariant> &args )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
{
    Q_UNUSED( args )
    m_sources << "lastfm" ;

    m_timeoutTimer.setInterval( 10000 );
    m_timeoutTimer.setSingleShot( true );
    connect( &m_timeoutTimer, SIGNAL(timeout()), this, SLOT(timeout()) );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
             this, SLOT( update() ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( update() ) );
}

LabelsEngine::~LabelsEngine()
{
    DEBUG_BLOCK
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
    Q_UNUSED( name )

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    if( coll )
    {
        Collections::QueryMaker *qm = coll->queryMaker();
        qm->setAutoDelete( true );
        qm->setQueryType( Collections::QueryMaker::Label );
        m_allLabels.clear();

        connect( qm, SIGNAL( newResultReady( QString, Meta::LabelList ) ),
                SLOT( resultReady( QString, Meta::LabelList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( dataQueryDone() ) );

        qm->run();
    }

    update();

    return true;
}

void
LabelsEngine::resultReady( const QString &collectionId, const Meta::LabelList &labels )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::LabelPtr &label, labels )
    {
        if( !label->name().isEmpty() )
            m_allLabels << label->name();
    }
}

void
LabelsEngine::dataQueryDone()
{
    DEBUG_BLOCK
    QVariant varAll;
    varAll.setValue< QStringList >( m_allLabels );
    setData( "labels", "all", varAll );
}

void
LabelsEngine::update()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    QString title;
    Meta::ArtistPtr artist;
    QStringList labels;

    if( track )
    {
        title = track->name();
        artist = track->artist();
        foreach( const Meta::LabelPtr &label, track->labels() )
            labels += label->name();
    }

    labels.sort();
    m_userLabels.sort();

    // -- check if really something changed
    if( artist->name() == m_artist &&
        title == m_title &&
        labels == m_userLabels )
        return; // nothing to do

    removeAllData( "labels" );
    setData( "labels", "state", "started" );

    m_artist = artist->name();
    m_title = title;
    m_userLabels = labels;

    QVariant varUser;
    varUser.setValue< QStringList >( m_userLabels );
    setData( "labels", "user", varUser );

    // send the web labels too, because the labels applet clears all web labels if user labels arrive
    QVariant varWeb;
    varWeb.setValue< QMap< QString, QVariant > > ( m_webLabels );
    setData( "labels", "web", varWeb );

    if( m_title.isEmpty() || m_artist.isEmpty() )
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on last.fm" ) );
        debug()  << "LabelsEngine:" << "current track is invalid, returning";
        return;
    }
    else
    {
        m_try = 0;
        fetchLastFm();
    }
}

void
LabelsEngine::fetchLastFm()
{
    DEBUG_BLOCK
    QStringList separators;
    QString currentArtist;
    QString currentTitle;

    if( m_title.isEmpty() || m_artist.isEmpty() )
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on last.fm" ) );
        debug()  << "LabelsEngine:" << "current track is invalid, returning";
        return;
    }
    
    if ( m_try == 0 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        m_timeoutTimer.start();
    }
    else if ( m_try == 1 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        separators.clear();
        separators << " (" << " [" << " - " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << "/";
        foreach( const QString &separator, separators )
        {
            if( m_title.contains(separator,Qt::CaseInsensitive) )
            {
                currentTitle = m_title.left( m_title.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        if ( currentTitle == m_title )
        {
            debug() << "try 2: title is the same, retrying";
            m_try++;
            fetchLastFm();
            return;
        }
    }
    else if ( m_try == 2 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        separators.clear();
        separators << " vs. " << " vs " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << ", " << " and " << " & " << "/";
        foreach( const QString &separator, separators )
        {
            if( m_artist.contains(separator,Qt::CaseInsensitive) )
            {
                currentArtist = m_artist.left( m_artist.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        separators.clear();
        separators << " (" << " [" << " - " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << "/";
        foreach( const QString &separator, separators )
        {
            if( m_title.contains(separator,Qt::CaseInsensitive) )
            {
                currentTitle = m_title.left( m_title.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        if ( currentArtist == m_artist ) // the title got modified the same way as on the last try
        {
            // stop timeout timer
            m_timeoutTimer.stop();
            setData( "labels", "message", i18n( "No labels found on last.fm" ) );
            debug()  << "LabelsEngine:" << "try 3: artist and title are the same, returning";
            return;
        }
    }
    else
    {
        // shouldn't happen
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on last.fm" ) );
        debug() << "try > 2, returning";
        return;
    }

    debug()  << "LabelsEngine:" << "currentArtist:" << currentArtist << "currentTitle:" << currentTitle;

    if ( !currentArtist.isEmpty() && !currentTitle.isEmpty() )
    {
        setData( "labels", "message", "fetching");
        // send the atist and title actually used for searching labels
        setData( "labels", "artist", currentArtist );
        setData( "labels", "title", currentTitle );
        setData( "labels", "album", m_album );

        // Query lastfm
        KUrl lastFmUrl;
        lastFmUrl.setScheme( "http" );
        lastFmUrl.setHost( "ws.audioscrobbler.com" );
        lastFmUrl.setPath( "/2.0/" );
        lastFmUrl.addQueryItem( "method", "track.gettoptags" );
        lastFmUrl.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
        lastFmUrl.addQueryItem( "artist", currentArtist.toLocal8Bit() );
        lastFmUrl.addQueryItem( "track", currentTitle.toLocal8Bit() );
        m_lastFmUrl = lastFmUrl;
        
        debug() << "last.fm : " << lastFmUrl.toMimeDataString();
        QNetworkRequest req( lastFmUrl );
        The::networkAccessManager()->get( req );
        The::networkAccessManager()->getData( lastFmUrl, this,
            SLOT(resultLastFm(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
    else
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on last.fm" ) );
        debug() << "artist or track empty";
    }
}

void LabelsEngine::resultLastFm( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    DEBUG_BLOCK;

    if( m_lastFmUrl != url )
    {
        debug() << "urls not matching, returning";
        return;
    }

    if ( e.code != QNetworkReply::NoError )
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "Unable to retrieve from last.fm" ) );
        debug() << "Unable to retrieve last.fm information: " << e.description;
        return;
    }

    QDomDocument xmlDoc;
    xmlDoc.setContent( data );
    QDomElement topElement = xmlDoc.elementsByTagName("toptags").at(0).toElement();
    QDomNodeList xmlNodeList = topElement.elementsByTagName( "tag" );

    for ( uint i = 0; i < xmlNodeList.length(); i++ )
    {
        // Get all the information
        QDomElement nd = xmlNodeList.at( i ).toElement();
        QDomElement nameElement = nd.elementsByTagName("name").at(0).toElement();
        QString name = nameElement.text().toLower();
        QDomElement countElement = nd.elementsByTagName("count").at(0).toElement();
        int count = countElement.text().toInt();
        m_webLabels.insert( name, count );
    }

    if ( m_webLabels.isEmpty() )
    {
        if( m_try < 2 )
        {
            m_try++;
            fetchLastFm();
        }
        else
        {
            // stop timeout timer
            m_timeoutTimer.stop();
            setData( "labels", "message", i18n( "No labels found on last.fm" ) );
        }
    }
    else
    {
        // remove previous message
        removeData( "labels", "message" );
        // stop timeout timer
        m_timeoutTimer.stop();

        QVariant varWeb;
        varWeb.setValue< QMap< QString, QVariant > > ( m_webLabels );
        setData( "labels", "web", varWeb );
    }
}

void LabelsEngine::timeout()
{
    setData( "labels", "message", i18n( "No connection to last.fm" ) );
}


#include "LabelsEngine.moc"

