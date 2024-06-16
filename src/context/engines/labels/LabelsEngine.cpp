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

#include "EngineController.h"
#include "context/ContextObserver.h"
#include "context/ContextView.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KIO/Job>
#include <KLocalizedString>

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

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(update()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(update()) );
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

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    if( coll )
    {
        Collections::QueryMaker *qm = coll->queryMaker();
        qm->setAutoDelete( true );
        qm->setQueryType( Collections::QueryMaker::Label );
        m_allLabels.clear();

        connect( qm, SIGNAL(newResultReady(Meta::LabelList)),
                SLOT(resultReady(Meta::LabelList)), Qt::QueuedConnection );
        connect( qm, SIGNAL(queryDone()), SLOT(dataQueryDone()) );

        qm->run();
    }

    update( name == "reload" );

    return true;
}

void
LabelsEngine::resultReady( const Meta::LabelList &labels )
{
    for( const Meta::LabelPtr &label : labels )
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
LabelsEngine::update( bool reload )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( !track )
    {
        removeAllData( "labels" );
        m_artist.clear();
        m_title.clear();
        m_album.clear();
        m_userLabels.clear();
        m_webLabels.clear();
        setData( "labels", "state", "stopped" );
        return;
    }

    const QString title = track->name();
    Meta::ArtistPtr artist = track->artist();
    if( !artist )
    {
        setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
        debug() << "track has no artist, returning";
        return;
    }

    QStringList userLabels;

    for( const Meta::LabelPtr &label : track->labels() )
        userLabels += label->name();

    userLabels.sort();
    m_userLabels.sort();

    // check what changed
    if( !reload && artist->name() == m_artist && title == m_title && userLabels == m_userLabels )
    {
        // nothing important changed
        return;
    }
    else if( !reload && artist->name() == m_artist && title == m_title )
    {
        // only the labels changed - no download necessary
        debug() << "only the labels changed - no download necessary";
        QVariant varUser;
        varUser.setValue< QStringList >( userLabels );
        setData( "labels", "user", varUser );
        m_userLabels = userLabels;
        return;
    }

    removeAllData( "labels" );
    setData( "labels", "state", "started" );

    m_artist = artist->name();
    m_title = title;
    if( track->album() )
        m_album = track->album()->name();
    else
        m_album.clear();
    
    m_userLabels = userLabels;
    m_webLabels.clear();

    QVariant varUser;
    varUser.setValue< QStringList >( m_userLabels );
    setData( "labels", "user", varUser );

    m_try = 0;
    fetchLastFm();
}

void
LabelsEngine::fetchLastFm()
{
    QStringList separators;
    QString currentArtist;
    QString currentTitle;

    if( m_title.isEmpty() || m_artist.isEmpty() )
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
        debug() << "current track is invalid, returning";
        return;
    }
    
    if( m_try == 0 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        m_timeoutTimer.start();
    }
    else if( m_try == 1 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        separators.clear();
        separators << " (" << " [" << " - " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << "/";
        for( const QString &separator : separators )
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
    else if( m_try == 2 )
    {
        currentArtist = m_artist;
        currentTitle = m_title;
        separators.clear();
        separators << " vs. " << " vs " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << ", " << " and " << " & " << "/";
        for( const QString &separator : separators )
        {
            if( m_artist.contains(separator,Qt::CaseInsensitive) )
            {
                currentArtist = m_artist.left( m_artist.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        separators.clear();
        separators << " (" << " [" << " - " << " featuring " << " feat. " << " feat " << " ft. " << " ft " << "/";
        for( const QString &separator : separators )
        {
            if( m_title.contains(separator,Qt::CaseInsensitive) )
            {
                currentTitle = m_title.left( m_title.indexOf(separator,0,Qt::CaseInsensitive) );
                break;
            }
        }
        if( currentArtist == m_artist ) // the title got modified the same way as on the last try
        {
            // stop timeout timer
            m_timeoutTimer.stop();
            setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
            debug() << "try 3: artist and title are the same, returning";
            return;
        }
    }
    else
    {
        // shouldn't happen
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
        debug() << "try > 2, returning";
        return;
    }

    if( !currentArtist.isEmpty() && !currentTitle.isEmpty() )
    {
        setData( "labels", "message", "fetching");
        // send the artist and title actually used for searching labels
        setData( "labels", "artist", currentArtist );
        setData( "labels", "title", currentTitle );
        setData( "labels", "album", m_album );

        // Query lastfm
        QUrl lastFmUrl;
        lastFmUrl.setScheme( "http" );
        lastFmUrl.setHost( "ws.audioscrobbler.com" );
        lastFmUrl.setPath( "/2.0/" );
        lastFmUrl.addQueryItem( "method", "track.gettoptags" );
        lastFmUrl.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
        lastFmUrl.addQueryItem( "artist", currentArtist.toLocal8Bit() );
        lastFmUrl.addQueryItem( "track", currentTitle.toLocal8Bit() );
        m_lastFmUrl = lastFmUrl;
        
        QNetworkRequest req( lastFmUrl );
//         req.setAttribute( QNetworkRequest::ConnectionEncryptedAttribute, QNetworkRequest::AlwaysNetwork );
        The::networkAccessManager()->get( req );
        The::networkAccessManager()->getData( lastFmUrl, this,
            SLOT(resultLastFm(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
    else
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
        debug() << "artist or track empty";
    }
}

void LabelsEngine::resultLastFm( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    DEBUG_BLOCK;

    if( m_lastFmUrl != url )
    {
        debug() << "urls not matching, returning";
        return;
    }

    if( e.code != QNetworkReply::NoError )
    {
        // stop timeout timer
        m_timeoutTimer.stop();
        setData( "labels", "message", i18n( "Unable to retrieve from Last.fm" ) );
        debug() << "Unable to retrieve last.fm information: " << e.description;
        return;
    }

    QDomDocument xmlDoc;
    xmlDoc.setContent( data );
    const QDomElement topElement = xmlDoc.elementsByTagName("toptags").at(0).toElement();
    const QDomNodeList xmlNodeList = topElement.elementsByTagName( "tag" );

    for( uint i = 0; i < xmlNodeList.length(); i++ )
    {
        // Get all the information
        const QDomElement nd = xmlNodeList.at( i ).toElement();
        const QDomElement nameElement = nd.elementsByTagName("name").at(0).toElement();
        const QString name = nameElement.text().toLower();
        const QDomElement countElement = nd.elementsByTagName("count").at(0).toElement();
        const int count = countElement.text().toInt();
        m_webLabels.insert( name, count );
    }

    if( m_webLabels.isEmpty() )
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
            setData( "labels", "message", i18n( "No labels found on Last.fm" ) );
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
    setData( "labels", "message", i18n( "No connection to Last.fm" ) );
}



