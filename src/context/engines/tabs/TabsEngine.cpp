/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#define DEBUG_PREFIX "TabsEngine"

#include "TabsEngine.h"

#include "EngineController.h"
#include "context/ContextView.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QTextCodec>

using namespace Context;

/**
 * \brief Constructor
 *
 * Creates a new instance of the TabsEngine
 */
TabsEngine::TabsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , m_fetchGuitar( true )
        , m_fetchBass( true )
        , m_numAbortedUrls( 0 )
{
    EngineController *engine = The::engineController();
    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
            this, SLOT(update()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(update()) );
}

/**
 * \brief Destructor
 *
 * Destroys a TabsEngine instance
 */
TabsEngine::~TabsEngine()
{
    DEBUG_BLOCK
    foreach( TabsInfo *info, m_tabs )
        delete info;
    m_tabs.clear();
    m_urls.clear();
}

/**
 * Returns our sources
 */
QStringList
TabsEngine::sources() const
{
    // one source within the tabs-engine
    QStringList sources;
    sources << "tabs";
    return sources;
}

bool
TabsEngine::fetchGuitar() const
{
    return m_fetchGuitar;
}

void
TabsEngine::setFetchGuitar( const bool fetch )
{
    m_fetchGuitar = fetch;
}

bool
TabsEngine::fetchBass() const
{
    return m_fetchBass;
}

void
TabsEngine::setFetchBass( const bool fetch )
{
    m_fetchBass = fetch;
}

QString
TabsEngine::artistName() const
{
    return m_artistName;
}

void
TabsEngine::setArtistName( const QString &artistName )
{
    m_artistName = artistName;
}

QString
TabsEngine::titleName() const
{
    return m_titleName;
}

void
TabsEngine::setTitleName( const QString &titleName )
{
    m_titleName = titleName;
}

bool
TabsEngine::sourceRequestEvent( const QString &name )
{
    removeAllData( name );
    setData( name, QVariant() );

    QStringList tokens = name.split( QLatin1Char(':'), Qt::SkipEmptyParts );
    if( tokens.contains( QLatin1String( "forceUpdate" ) ) )
    {
        // data coming from the applet configuration dialog
        m_titleName.clear();
        m_artistName.clear();
        update();
    }
    else if( tokens.contains( QLatin1String( "forceUpdateSpecificTitleArtist" ) ) )
    {
        // handle reload of a specific artist and title
        requestTab( m_artistName, m_titleName );
    }
    else
    {
        // update on initial request
        update();
    }
    return true;
}

/**
 * called whenever metadata of the current track has changed
 */
void
TabsEngine::update()
{
    DEBUG_BLOCK

    // get the current track
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
    {
        debug() << "no track";
        m_titleName.clear();
        m_artistName.clear();
        removeAllData( "tabs" );
        setData( "tabs", "state", "Stopped" );
        return;
    }
    m_currentTrack = track;
    Meta::ArtistPtr artistPtr = track->artist();
    QString newArtist;
    if( artistPtr )
    {
        if( ( track->playableUrl().protocol() == "lastfm" ) ||
            ( track->playableUrl().protocol() == "daap" ) ||
            !The::engineController()->isStream() )
            newArtist = artistPtr->name();
        else
            newArtist = artistPtr->prettyName();
    }

    QString newTitle = track->name();
    if( newTitle.isEmpty() )
        newTitle = track->prettyName();

    // check if something changed
    if( newTitle == m_titleName && newArtist == m_artistName )
    {
        debug() << "nothing changed";
        return;
    }

    // stop fetching for unknown artists or titles
    if( newTitle.isEmpty() || newArtist.isEmpty() )
    {
        setData("tabs", "state", "noTabs" );
        return;
    }
    requestTab( newArtist, newTitle );
}

/**
 * starts a new tab-search
 */
void
TabsEngine::requestTab( const QString &artist, const QString &title )
{
    DEBUG_BLOCK
    debug() << "request tabs for artist: " << artist << " and title " << title;

    // clean all previously allocated stuff
    foreach( TabsInfo *tab, m_tabs )
        delete tab;
    m_tabs.clear();
    m_urls.clear();
    m_numAbortedUrls = 0;
    removeAllData( "tabs" );

    m_artistName = artist;
    m_titleName = title;

    // status update
    setData( "tabs", "state", "Fetching" );
    setData( "tabs", "title", m_titleName );
    setData( "tabs", "artist", m_artistName );

    // define search criteria for the current artist/track
    QStringList artistSearchList = defineArtistSearchCriteria( artist );
    QStringList titleSearchList = defineTitleSearchCriteria( title );
    foreach( const QString &searchArtist, artistSearchList )
    {
        foreach( const QString &searchTitle, titleSearchList )
        {
            queryUltimateGuitar( searchArtist, searchTitle );
        }
    }
}

/**
 * * starts a tab-search on UltimateGuitar.com
 */
void
TabsEngine::queryUltimateGuitar( const QString &artist, const QString &title )
{
    // Query UltimateGuitar.com (filtering guitar (tabs + chords) and bass tabs)
    QUrl ultimateGuitarUrl;
    ultimateGuitarUrl.setScheme( "https" );
    ultimateGuitarUrl.setHost( "www.ultimate-guitar.com" );
    ultimateGuitarUrl.setPath( "/search.php" );
    ultimateGuitarUrl.addQueryItem( "view_state", "advanced" );
    ultimateGuitarUrl.addQueryItem( "band_name", artist );
    ultimateGuitarUrl.addQueryItem( "song_name", title );
    ultimateGuitarUrl.addQueryItem( "type%5B%5D", QString::number ( 200 ) );  // filter guitar tabs
    ultimateGuitarUrl.addQueryItem( "type%5B%5D", QString::number ( 300 ) );  // filter guitar chords
    ultimateGuitarUrl.addQueryItem( "type%5B%5D", QString::number ( 400 ) );  // filter bass tabs
    ultimateGuitarUrl.addQueryItem( "version_la", "" );

    The::networkAccessManager()->getData( ultimateGuitarUrl, this,
        SLOT(resultUltimateGuitarSearch(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    m_urls.insert( ultimateGuitarUrl );
}

/**
 *  parses the tab search results from UltimateGuitar
 */
void
TabsEngine::resultUltimateGuitarSearch( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    // specific job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // check if an error occurred during the HTTP-request
    if( netReplyError( e ) )
        return;

    // get and parse the result
    const QString result( data );
    const QString resultsTable = subStringBetween( result, "class=\"tresults\"", "</table>" );
    if( !resultsTable.isEmpty() )
    {
        const QStringList results = resultsTable.split( "</tr>" );
        foreach ( const QString &result, results )
        {
            // lastIndex on purpose (due to the fact that tabledata for the first result contains two hrefs)
            // get the link to the actual tab
            const QString tabUrl = subStringBetween( result, "a href=\"", "\" class", true );
            if( !tabUrl.isEmpty() )
            {
                // fetch the actual tab
                const QUrl tabFetchUrl = QUrl( tabUrl );
                The::networkAccessManager()->getData( tabFetchUrl, this,
                    SLOT(resultUltimateGuitarTab(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
                m_urls.insert( tabFetchUrl );
            }
        }
    }
    resultFinalize();
}

/**
 * * retrieves the information for a single tab from UltimateGuitar.com
 */
void
TabsEngine::resultUltimateGuitarTab( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    // specific tab search job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // check if an error occurred during the HTTP-request
    if( netReplyError( e ) )
        return;

    // TODO: is this valid in all cases?
    // without fromLatin1, umlauts in german tabs are not displayed correctly
    QString result;
    if( QTextCodec::codecForUtfText( data )->name().contains( "ISO-8859-1" ) )
        result = QString::fromLatin1( data );
    else
        result = QString( data );

    // extract tab title and data
    const QString title = subStringBetween( result, "<strong>", "</strong>" );
    result.remove( subStringBetween( result, "<div class=\"dn\">", "</div>" ) );
    QRegularExpression regex = QRegularExpression( "<pre>.*</pre>", Qt::CaseInsensitive );
    if( regex.indexIn( result ) == -1 )
        return;
    QString tabs = regex.cap();
    tabs.remove( "<span>", Qt::CaseInsensitive );
    tabs.remove( "</span>", Qt::CaseInsensitive );

    TabsInfo::TabType tabType = TabsInfo::GUITAR;
    const QString tabTypeString = subStringBetween( result, "<title>", " by " );
    if( tabTypeString.contains( "bass", Qt::CaseInsensitive ) )
        tabType = TabsInfo::BASS;

    if( !tabs.isEmpty() )
    {
        if( ( m_fetchGuitar && tabType == TabsInfo::GUITAR ) ||
            ( m_fetchBass && tabType == TabsInfo::BASS ) )
        {
            TabsInfo *item = new TabsInfo;
            item->url      = url;
            item->tabType  = tabType;
            item->title    = title;
            item->tabs     = tabs;
            item->source   = "Ultimate-Guitar";

            m_tabs << item;
        }
    }
    // update the results
    resultFinalize();
}

/**
 * checks if all fetching jobs have finished and send the tab-data to the applet afterwards
 */
void
TabsEngine::resultFinalize()
{
    if( m_urls.count() > 0 )
        return;

    // remove fetching state
    removeData( "tabs", "state" );

    debug() << "Total # of fetched tabs: " << m_tabs.size();
    if( m_numAbortedUrls > 0 )
    {
        setData( "tabs", "state", "FetchError" );
        return;
    }
    else if( m_tabs.size() == 0 )
    {
        setData( "tabs", "state", "noTabs" );
        return;
    }
    else
    {
        // sort against tabtype
        QList < QPair < TabsInfo::TabType, QUrl > > sorting;
        foreach( TabsInfo *item, m_tabs )
            sorting << QPair < TabsInfo::TabType, QUrl> ( item->tabType, item->url) ;
        qSort(sorting.begin(), sorting.end(), qLess<QPair < TabsInfo::TabType, QUrl> >() );

        // debug info
        foreach( TabsInfo *item, m_tabs )
            debug() << " Title: " << item->title << " (" << item->url << ")";

        // if the song hasn't change while fetching, we sent the data
        if( m_currentTrack != The::engineController()->currentTrack() )
            return;

        // otherwise send the fetched data to the subscribed applets
        QList < QPair <TabsInfo::TabType, QUrl > >::iterator i;
        int pos = 0;
        for(i = sorting.begin(); i != sorting.end(); ++i)
        {
            foreach( TabsInfo *item, m_tabs)
            {
                if( (*i).second == item->url )
                {
                    QVariant var;
                    var.setValue<TabsInfo *>( item );
                    setData( "tabs", QString( "tabs:" ) + QString().setNum( pos ), var );
                    pos++;
                }
            }
        }
    }
}

/**
 * helper-function for html-parsing
 */
QString
TabsEngine::subStringBetween( const QString &src, const QString &from, const QString &to,
                              bool lastIndexForFrom )
{
    int startIdx;

    if( lastIndexForFrom )
        startIdx = src.lastIndexOf( from );
    else
        startIdx = src.indexOf( from );

    if( startIdx == -1 )
        return QString();
    startIdx += from.length();

    int endIdx = src.indexOf( to, startIdx );
    if( endIdx == -1 )
        return QString();

    return src.mid( startIdx, endIdx - startIdx );
}

/**
 * modifications on the artist to get more results
 */
QStringList
TabsEngine::defineArtistSearchCriteria( const QString &artist )
{
    QStringList artists;

    QString searchArtist = artist.trimmed();
    artists << searchArtist;

    // remove trailing "The" (otherwise no results for 'The Cure', 'The Smashing Pumpkins', ...)
    if( searchArtist.startsWith( "The ", Qt::CaseInsensitive ) )
        artists << searchArtist.remove( "The ", Qt::CaseInsensitive );

    return artists;
}


/**
 * modifications on the title to get more results
 */
QStringList
TabsEngine::defineTitleSearchCriteria( const QString &title )
{
    QStringList titles;

    QString searchTitle  = title.trimmed();
    titles << searchTitle;

    // remove trailing "The"
    if( searchTitle.startsWith( "The ", Qt::CaseInsensitive ) )
        titles << searchTitle.remove( "The ", Qt::CaseInsensitive );

    // remove anything like (live), (demo-tape), ...
    QRegularExpression regex = QRegularExpression( "\\s*\\(.*\\)", Qt::CaseInsensitive );
    if( regex.indexIn( searchTitle ) > 0 )
        titles << searchTitle.remove( regex );

    // remove anything like [xxxx].
    regex = QRegularExpression( "\\s*\\[.*\\]", Qt::CaseInsensitive );
    if( regex.indexIn( searchTitle ) > 0 )
        titles << searchTitle.remove( regex );

    return titles;
}

/**
 * checks if a tab-fetch job aborted with an error
 * returns true in case of error, false otherwise
 */
bool
TabsEngine::netReplyError( NetworkAccessManagerProxy::Error e )
{
    // check the access manager network replay
    if( e.code == QNetworkReply::NoError )
    {
        // at least one job successful, clear list of aborted urls
        m_numAbortedUrls = 0;
        return false;
    }
    else
    {
        // store url, list gets checked in resultFinalize
        m_numAbortedUrls++;
        resultFinalize();
        return true;
    }
}

