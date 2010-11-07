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

// Amarok
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ContextView.h"
#include "EngineController.h"

// Qt
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
{
    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ), this, SLOT( update() ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ), this, SLOT( update() ) );
}

/**
 * \brief Destructor
 *
 * Destroys a TabsEngine instance
 */
TabsEngine::~TabsEngine()
{
    DEBUG_BLOCK
    foreach ( TabsInfo *info, m_tabs )
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

/**
 * When a source that does not currently exist is requested by the
 * consumer, this method is called to give the DataEngine the
 * opportunity to create one.
 */
bool
TabsEngine::sourceRequestEvent( const QString& name )
{
    QStringList tokens = name.split( ':' );

    // data coming from the applet configuration dialogue
    if( tokens.contains( "fetchGuitar" ) && tokens.size() > 1 )
        if( ( tokens.at( 1 ) == QString( "fetchGuitar" ) )  && ( tokens.size() > 2 ) )
            m_fetchGuitar = tokens.at( 2 ).toInt();
    if( tokens.contains( "fetchBass" ) && tokens.size() > 1 )
        if( ( tokens.at( 1 ) == QString( "fetchBass" ) )  && ( tokens.size() > 2 ) )
            m_fetchBass = tokens.at( 2 ).toInt();
    if( name.contains( ":AMAROK_TOKEN:" ) && tokens.size() == 5 )
    {
        tokens = name.split( ":AMAROK_TOKEN:" );
        if( tokens.size() == 3 )
        {
            QString artist = tokens.at( 1 );
            QString title = tokens.at( 2 );
            removeAllData( name );
            setData( name, QVariant());
            requestTab( artist, title );
            return true;
        }
    }

    // a new track is playing.
    removeAllData( name );
    setData( name, QVariant() );
    update();

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
        if(( track->playableUrl().protocol() == "lastfm" ) ||
            ( track->playableUrl().protocol() == "daap" ) ||
            !The::engineController()->isStream() )
            newArtist = artistPtr->name();
        else
            newArtist = artistPtr->prettyName();
    }

    QString newTitle = track->name();

    // check if something changed
    if( newTitle == m_titleName && newArtist == m_artistName )
    {
        debug() << "nothing changed";
        return; // nothing changed
    }

    // stop fetching for unknown artists or titles
    if( newTitle.isEmpty() || newArtist.isEmpty() )
    {
        setData("tabs", "message", i18n( "No valid artist or titlename found for the current track." ) );
        return;
    }
    requestTab( newArtist, newTitle );
}

/**
 * starts a new tab-search
 */
void
TabsEngine::requestTab( QString artist, QString title )
{
    DEBUG_BLOCK
    debug() << "request tabs for artis: " << artist << " and title " << title;

    // clean all previously allocated stuff
    foreach( TabsInfo *tab, m_tabs )
        delete tab;
    m_tabs.clear();
    m_urls.clear();
    removeAllData( "tabs" );

    m_artistName = artist;
    m_titleName = title;

    // status update
    setData( "tabs", "state", "Fetching" );
    setData( "tabs", "title", m_titleName );
    setData( "tabs", "artist", m_artistName );

    QString searchArtist = artist.trimmed().replace( ' ', '+' );
    QString searchTitle  = title.trimmed().replace( ' ', '+' );

    // remove trailing "The" (otherwise no results for 'The Cure', 'The Smashing Pumpkins', ...)
    if( searchArtist.startsWith( "The+", Qt::CaseInsensitive ) )
        searchArtist.remove( "The+", Qt::CaseInsensitive );

    // Query UltimateGuitar.com
    const KUrl ultimateGuitarUrl( QString( "http://www.ultimate-guitar.com/search.php?view_state=advanced" ) +
                                  QString( "&band_name=" ) + searchArtist + QString( "&song_name=") + searchTitle +
                                  QString( "&type[]=200&type[]=400&type[]=300&version_la=" ) );  // this is a filter for guitar (tabs and chords) + bass
    The::networkAccessManager()->getData( ultimateGuitarUrl, this, SLOT( resultUltimateGuitarSearch( KUrl, QByteArray, NetworkAccessManagerProxy::Error ) ) );
    m_urls.insert( ultimateGuitarUrl, UltimateGuitar );

    // Query fretplay.com (search for song name and filter afterwards according to artist)
    // fretplay.com : http://www.fretplay.com/search-tabs?search=SongName
    const KUrl fretplayUrl( QString( "http://www.fretplay.com/search-tabs?search=" ) + searchTitle );
    The::networkAccessManager()->getData( fretplayUrl, this, SLOT( resultFretplaySearch( KUrl, QByteArray, NetworkAccessManagerProxy::Error ) ) );
    m_urls.insert( fretplayUrl, FretPlay );
}

/**
 * * starts a tab-search on UltimateGuitar.com
 */
void
TabsEngine::resultUltimateGuitarSearch( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    // specific job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // an error occured during the HTTP-request
    if( e.code != QNetworkReply::NoError )
    {
        setData( "tabs", "message", i18n( "Unable to retrieve tab information from Ultimate Guitar: %1", e.description ) );
        debug() << "Unable to search for tab on UltimateGuitar.com: " << e.description;
        resultFinalize();
        return;
    }

    // get and parse the result
    const QString result( data );
    const QString resultsTable = subStringBetween( result, "class=\"tresults\">", "</table>" );
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
                // fetch the the actual tab
                const KUrl tabFetchUrl = KUrl( tabUrl );
                The::networkAccessManager()->getData( tabFetchUrl, this, SLOT( resultUltimateGuitarTab( KUrl, QByteArray, NetworkAccessManagerProxy::Error ) ) );
                m_urls.insert( tabFetchUrl, UltimateGuitar );
            }
        }
    }
    resultFinalize();
}

/**
 * * retrieves the information for a single tab from UltimateGuitar.com
 */
void
TabsEngine::resultUltimateGuitarTab( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e  )
{
    // specific tab search job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // an error occured during the HTTP-request
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve Ultimate Guitar information: " << e.description;
        resultFinalize();
        return;
    }

    // TODO: is this valid in all cases?
    // without fromLatin1, umlauts in german tabs are not displayed correctly
    QString result;
    if( QTextCodec::codecForUtfText( data )->name().contains( "ISO-8859-1" ) )
        result = QString::fromLatin1( data );
    else
        result = QString( data );

    const QString title = subStringBetween( result, "<strong>", "</strong>");
    QString tabs = subStringBetween( result, "<pre>", "</pre>" );
    tabs.remove( "<span>", Qt::CaseInsensitive );
    tabs.remove( "</span>", Qt::CaseInsensitive );

    TabsInfo::TabType tabType = TabsInfo::GUITAR;
    const QString tabTypeString = subStringBetween( result, "<title>", " by " );
    if( tabTypeString.contains( "bass", Qt::CaseInsensitive ) )
        tabType = TabsInfo::BASS;

    if( !tabs.isEmpty() )
    {
        if( ( m_fetchGuitar && tabType == TabsInfo::GUITAR ) || ( m_fetchBass && tabType == TabsInfo::BASS ) )
        {
            TabsInfo *item = new TabsInfo;
            item->url      = url;
            item->tabType  = tabType;
            item->title    = title;
            item->tabs     = tabs;
            item->source   = "UltimateGuitar";

            m_tabs << item;
        }
    }
    // update the results
    resultFinalize();
}

/**
 * * starts a tab-search on fretplay.com
 */
void
TabsEngine::resultFretplaySearch( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    // specific tab search job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // an error occured during the HTTP-request
    if( e.code != QNetworkReply::NoError )
    {
        setData( "tabs", "message", i18n( "Unable to retrieve tab information from fretplay.com: %1", e.description ) );
        debug() << "Unable to search for tab on fretplay.com: " << e.description;
        resultFinalize();
        return;
    }

    // get and parse the result, we searched for song name, so filter out the artist
    const QString result ( data );
    const QString resultsTable = subStringBetween( result, "<H2>Matching guitar tabs and chords</H2>", "</div>" );
    if( !resultsTable.isEmpty() )
    {
        QStringList results = resultsTable.split( "<BR>" );
        foreach ( const QString &result, results )
        {
            const QString artist = subStringBetween( result, "\">", "</a>" );
            if( artist.compare( m_artistName, Qt::CaseInsensitive ) == 0 )
            {
                // lastIndex on purpose (due to the fact that tabledata for the first url contains the artist tabs, second the title tab
                const KUrl tabFetchUrl = KUrl( subStringBetween( result,  "a href=\"", "\" title", true ) );
                if( !tabFetchUrl.url().isEmpty() )
                {
                    // Query fretplay.com for the specific tab using the url found in the results
                    The::networkAccessManager()->getData( tabFetchUrl, this, SLOT( resultFretplayTab( KUrl, QByteArray, NetworkAccessManagerProxy::Error ) ) );
                    m_urls.insert( tabFetchUrl, FretPlay );
                }
            }
        }
    }
    resultFinalize();
}

/**
 * * retrieves the information for a single tab from fretplay.com
 */
void
TabsEngine::resultFretplayTab( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    // specific tab search job has finished -> remove from queue
    if( !m_urls.contains( url ) )
        return;
    m_urls.remove( url );

    // an error occured during the HTTP-request
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Unable to retrieve fretplay information: " << e.description;
        resultFinalize();
        return;
    }

    // TODO: is this valid in all cases?
    // without fromLatin1, umlauts in german tabs are not displayed correctly
    QString result;
    if( QTextCodec::codecForUtfText( data )->name().contains( "ISO-8859-1" ) )
        result = QString::fromLatin1( data );
    else
        result = QString( data );

    QString title = subStringBetween( result, "title\" content=\"", ". Accurate and free" );
    QString tabs = subStringBetween( result, "<pre>", "</pre>" );
    tabs.remove( "<span>", Qt::CaseInsensitive );
    tabs.remove( "</span>", Qt::CaseInsensitive );

    TabsInfo::TabType tabType = TabsInfo::GUITAR;
    if( title.contains( "Bass", Qt::CaseInsensitive ) )
        tabType = TabsInfo::BASS;

    title.remove( "Bass tabs", Qt::CaseInsensitive );
    title.remove( "Guitar tabs", Qt::CaseInsensitive );
    if( !tabs.isEmpty() )
    {
        if( ( m_fetchGuitar && tabType == TabsInfo::GUITAR ) || ( m_fetchBass && tabType == TabsInfo::BASS ) )
        {
            TabsInfo *item = new TabsInfo;
            item->url      = url;
            item->tabType  = tabType;
            item->title    = title;
            item->tabs     = tabs;
            item->source   = "fretplay";

            m_tabs << item;
        }
    }
    // update the results
    resultFinalize();
}

/**
 * checks wether all fetching jobs have finished and send the tab-data to the applet afterwards
 */
void
TabsEngine::resultFinalize()
{
    if( m_urls.count() > 0 )
        return;

    // reset the fetching state
    removeData( "tabs", "state" );

            // else if all the parallel jobs have finished and they have been called
    debug() << "Total # of fetched tabs: " << m_tabs.size();

    if( m_tabs.size() == 0 )
    {
        setData( "tabs", "state", "noTabs" );
        return;
    }
    else
    {
        // sort against tabtype
        QList < QPair < TabsInfo::TabType, KUrl > > sorting;
        foreach ( TabsInfo *item, m_tabs )
            sorting << QPair < TabsInfo::TabType, KUrl> ( item->tabType, item->url) ;
        qSort(sorting.begin(), sorting.end(), qLess<QPair < TabsInfo::TabType, KUrl> >() );

        // debug info
        foreach ( TabsInfo *item, m_tabs )
            debug() << " Title: " << item->title << " (" << item->url << ")";

        // if the song hasn't change while fetching, we sent the data
        if( m_currentTrack != The::engineController()->currentTrack() )
            return;

        // otherwise send the fetched data to the subscribed applets
        QList < QPair <TabsInfo::TabType, KUrl > >::iterator i;
        int pos = 0;
        for (i = sorting.begin(); i != sorting.end(); ++i)
        {
            foreach ( TabsInfo *item, m_tabs)
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
TabsEngine::subStringBetween( const QString src, const QString from, const QString to, bool lastIndexForFrom )
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

#include "TabsEngine.moc"
