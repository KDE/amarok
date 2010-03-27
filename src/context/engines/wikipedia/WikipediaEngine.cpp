/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#include "WikipediaEngine.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"

using namespace Context;

WikipediaEngine::WikipediaEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_wikiJob( 0 )
    , m_currentSelection( "artist" )
    , m_requested( true )
    , m_sources( "current" )
    , m_wikiWideLang( "aut" )
    , m_triedRefinedSearch( 0 )
{
    update();
}

WikipediaEngine::~WikipediaEngine()
{
    DEBUG_BLOCK

    // Ensure that no KIO job keeps running
    if( m_wikiJob )
    {
        m_wikiJob->kill();
        delete m_wikiJob;
    }
}

QStringList WikipediaEngine::sources() const
{
    return m_sources;
}

bool WikipediaEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ":AMAROK_TOKEN:" );

    // User has requested a reload
    if( tokens.contains( "reload" ) && tokens.size() > 1 )
    {
        if ( tokens.at( 1 ) == QString( "reload" ) )
        {
            reloadWikipedia();
            return true;
        }
    }

    // User has clicked on a link, let's fetch the page
    if( tokens.contains( "get" ) && tokens.size() > 1 )
    {
        if ( ( tokens.at( 1 ) == QString( "get" ) ) && ( tokens.size() > 2 ) )
        {
            m_wikiCurrentUrl = tokens.at( 2 ) ;
        
            removeSource( "wikipedia" );
            setData( "wikipedia", "busy", "busy" );
            m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
            connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
            return true;
        }
    }

    // user has selected is favorite language.
    if ( tokens.contains( "lang" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "lang" ) )  && ( tokens.size() > 2 ) )
            m_wikiWideLang = tokens.at( 2 );

    // User want to switch from artist to album, track etc ...
    if( tokens.contains( "goto" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "goto" ) ) && ( tokens.size() > 2 ) )
            setSelection( tokens.at ( 2 ) );
    
    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant());
    update();

    return true;
}

void WikipediaEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
        update();
}

void WikipediaEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK
    
    update();
}

void WikipediaEngine::update()
{
    DEBUG_BLOCK

    // We've got a new track, great, let's fetch some info from Wikipedia !
    m_triedRefinedSearch = 0;
    QString tmpWikiStr;
    

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;
    
    // default, or applet told us to fetch artist
    if( selection() == "artist" ) 
    {
        if( currentTrack->artist() )
        {
            if ( currentTrack->artist()->prettyName().isEmpty() )
            {
                debug() << "Requesting an empty string, skipping !";
                removeAllData( "wikipedia" );
                setData( "wikipedia", "message", i18n( "No information found..." ) );
                return;
            }
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->artist()->name() + wikiArtistPostfix();
            else
                tmpWikiStr = currentTrack->artist()->prettyName() + wikiArtistPostfix();
        }
    }
    else if( selection() == "album" )
    {
        if ( currentTrack->album() )
        {
            if ( currentTrack->album()->prettyName().isEmpty() )
            {
                debug() << "Requesting an empty string, skipping !";
                removeAllData( "wikipedia" );
                setData( "wikipedia", "message", i18n( "No information found..." ) );
                return;
            }
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->album()->name() + wikiAlbumPostfix();

        }
    }
    else if ( selection() == "track" )
    {
        if ( currentTrack->prettyName().isEmpty() )
        {
            debug() << "Requesting an empty string, skipping !";
            removeAllData( "wikipedia" );
            setData( "wikipedia", "message", i18n( "No information found..." ) );
            return;
        }
        tmpWikiStr = currentTrack->prettyName() + wikiTrackPostfix();
    }
    //Hack to make wiki searches work with magnatune preview tracks
    if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) )
    {
        tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );
        
        int index = tmpWikiStr.indexOf( '-' );
        if ( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( m_wikiCurrentLastEntry == tmpWikiStr )
    {        
        debug() << "Same entry requested again. Ignoring.";
        return;
    }
    
    removeAllData( "wikipedia" );

    // FIXME: what's that supposed to do? nothing?
    DataEngine::Data data;
    foreach( const QString &key, data.keys() )
    {
        setData( key, data[key] );
    }

    m_wikiCurrentLastEntry = tmpWikiStr;
    m_wikiCurrentEntry = tmpWikiStr;
    m_wikiCurrentUrl = wikiUrl( tmpWikiStr );


    debug() << "wiki url: " << m_wikiCurrentUrl;

    // Inform the applet that we are fetching info, set the busy thing
    setData( "wikipedia", "busy", "busy" );
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );

}

void
WikipediaEngine::wikiResult( KJob* job )
{
    DEBUG_BLOCK
    
    if( !m_wikiJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if( job->error() != KJob::NoError && job == m_wikiJob )
    {
        setData( "wikipedia", "message", i18n( "Unable to retrieve Wikipedia information: %1", job->errorString() ) );
        m_wikiJob = 0; // clear job
        return;
    }
    // not the right job, so let's ignore it
    if( job != m_wikiJob )
        return;

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_wiki = storedJob->data();

    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( m_wiki.contains( "charset=utf-8"  ) )
        m_wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );


    // FIXME: For now we test if we got an article or not with a test on this string "wgArticleId=0"
    // This is bad
    if( m_wiki.contains( "wgArticleId=0" ) && m_wiki.contains( "wgNamespaceNumber=0" ) ) // The article does not exist
    {
        // Refined search is done here 
        if ( m_triedRefinedSearch == -1 )
        {
            debug() << "We already tried some refined search. Lets end this madness...";
            removeAllData( "wikipedia" );
            setData( "wikipedia", "message", i18n( "No information found..." ) );
            m_wikiJob = 0;
            return;
        }
        m_triedRefinedSearch++;
        QString entry;
        debug() << "Article not found. Retrying with refinements.";


        if ( m_wikiCurrentEntry.lastIndexOf( "(" ) != -1)
            m_wikiCurrentEntry.remove( m_wikiCurrentEntry.lastIndexOf( "(" )-1, m_wikiCurrentEntry.size() );

        if ( selection() == "artist" )
            entry = m_wikiCurrentEntry+= wikiArtistPostfix() ;
        else if ( selection() == "album" )
            entry = m_wikiCurrentEntry+= wikiAlbumPostfix() ;
        else if ( selection() == "track" )
            entry = m_wikiCurrentEntry+= wikiTrackPostfix() ;

        m_wikiCurrentUrl = wikiUrl( entry );
        reloadWikipedia();
        return;
    }

    // We've find a page
    removeAllData( "wikipedia" );

    Data data;
    data["page"] = wikiParse();
    data["url"] = m_wikiCurrentUrl;

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack )
    {
        if( selection() == "artist" ) // default, or applet told us to fetch artist
        {
            if( currentTrack->artist() )
            {
                data["label"] =  "Artist";
                data["title"] = currentTrack->artist()->prettyName();
            }
        }
        else if( selection() == "track" )
        {
            data["label"] = "Title";
            data["title"] = currentTrack->prettyName();
        }
        else if( selection() == "album" )
        {
            if( currentTrack->album() )
            {
                data["label"] = "Album";
                data["title"] = currentTrack->album()->prettyName();
            }
        }
    }

    setData( "wikipedia", data );

    m_wikiJob = 0;
}

QString
WikipediaEngine::wikiParse()
{
    //remove the new-lines and tabs(replace with spaces IS needed).
    m_wiki.replace( '\n', ' ' );
    m_wiki.replace( '\t', ' ' );
    
    m_wikiLanguages.clear();
    // Get the available language list
    if ( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") != -1 )
    {
        m_wikiLanguages = m_wiki.mid( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") );
        m_wikiLanguages = m_wikiLanguages.mid( m_wikiLanguages.indexOf("<ul>") );
        m_wikiLanguages = m_wikiLanguages.mid( 0, m_wikiLanguages.indexOf( "</div>" ) );
    }
    
    QString copyright;
    QString copyrightMark = "<li id=\"f-copyright\">";
    if ( m_wiki.indexOf( copyrightMark ) != -1 )
    {
        copyright = m_wiki.mid( m_wiki.indexOf(copyrightMark) + copyrightMark.length() );
        copyright = copyright.mid( 0, copyright.indexOf( "</li>" ) );
        copyright.remove( "<br />" );
        //only one br at the beginning
        copyright.prepend( "<br />" );
    }
    
    // Ok lets remove the top and bottom parts of the page
    m_wiki = m_wiki.mid( m_wiki.indexOf( "<!-- start content -->" ) );
    m_wiki = m_wiki.mid( 0, m_wiki.indexOf( "<div class=\"printfooter\">" ) );
    
    // lets remove the warning box
    QString mbox = "<table class=\"metadata plainlinks ambox";
    QString mboxend = "</table>";
    while ( m_wiki.indexOf( mbox ) != -1 )
        m_wiki.remove( m_wiki.indexOf( mbox ), m_wiki.mid( m_wiki.indexOf( mbox ) ).indexOf( mboxend ) + mboxend.size() );
    
    QString protec = "<div><a href=\"/wiki/Wikipedia:Protection_policy" ;
    QString protecend = "</a></div>" ;
    while ( m_wiki.indexOf( protec ) != -1 )
        m_wiki.remove( m_wiki.indexOf( protec ), m_wiki.mid( m_wiki.indexOf( protec ) ).indexOf( protecend ) + protecend.size() );

    // lets also remove the "lock" image
    QString topicon = "<div class=\"metadata topicon\" " ;
    QString topiconend = "</a></div>";
     while ( m_wiki.indexOf( topicon ) != -1 )
        m_wiki.remove( m_wiki.indexOf( topicon ), m_wiki.mid( m_wiki.indexOf( topicon ) ).indexOf( topiconend ) + topiconend.size() );
    
    
    // Adding back style and license information
    m_wiki = "<div id=\"bodyContent\"" + m_wiki;
    m_wiki += copyright;
    m_wiki.append( "</div>" );
    m_wiki.remove( QRegExp("<h3 id=\"siteSub\">[^<]*</h3>") );
    
    m_wiki.remove( QRegExp( "<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>" ) );
    
    m_wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );
    
    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    m_wiki.remove( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ) );
    
    // Remove hidden table rows as well
    QRegExp hidden( "<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>", Qt::CaseInsensitive );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    m_wiki.remove( hidden );
    
    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    m_wiki.remove( QRegExp( "style= *\"[^\"]*\"" ) );
    // We need to leave the classes behind, otherwise styling it ourselves gets really nasty and tedious and roughly impossible to do in a sane maner
    //m_wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    m_wiki.remove( QRegExp( "<input[^>]*>" ) );
    m_wiki.remove( QRegExp( "<select[^>]*>" ) );
    m_wiki.remove( "</select>\n"  );
    m_wiki.remove( QRegExp( "<option[^>]*>" ) );
    m_wiki.remove( "</option>\n"  );
    m_wiki.remove( QRegExp( "<textarea[^>]*>" ) );
    m_wiki.remove( "</textarea>" );
    
    QString m_wikiHTMLSource = "<html><body>\n";
    m_wikiHTMLSource.append( m_wiki );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_wikiHTMLSource.append( "<br/><div id=\"wiki_otherlangs\" >" + i18n( "Wikipedia Other Languages: <br/>" )+ m_wikiLanguages + " </div>" );
    }
    m_wikiHTMLSource.append( "</body></html>\n" );
    
    return m_wikiHTMLSource;
}

inline QString
WikipediaEngine::wikiLocale() const
{
    // if there is no language set (QLocale::C) then return english as default
    if ( m_wikiWideLang == "aut" )
    {
        if( m_wikiLang.language() == QLocale::C )
            return "en";
        else
            return m_wikiLang.name().split( '_' )[0];
    }
    else
        return m_wikiWideLang;
}

inline QString
WikipediaEngine::wikiArtistPostfix()
{
    // prepare every case in each langage, if it's the last option, set m_triedRefinedSearch to -1

    if( wikiLocale() == "en" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return " (band)";
            case 1:
                return " (musician)";
            case 2:
                return " (singer)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else if( wikiLocale() == "de" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return " (Band)";
            case 1:
                return " (Musiker)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else if( wikiLocale() == "pl" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return " (grupa muzyczna)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else if( wikiLocale() == "fr" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return " (groupe)";
            case 1:
                return " (musicien)";
            case 2:
                return " (chanteur)";
            case 3:
                return " (chanteuse)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else // for every other country
    {
        m_triedRefinedSearch = -1;
        return "";
    }
}

inline QString
WikipediaEngine::wikiAlbumPostfix()
{
    if( wikiLocale() == "en" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return QString(" (")+m_currentTrack->artist()->prettyName()+QString(" album)");
            case 1:
                return " (album)";
            case 2:
                return " (score)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else if( wikiLocale() == "fr" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return QString(" (")+m_currentTrack->artist()->prettyName()+QString(" album)");
            case 1:
                return " (album)";
            case 2:
                return " (BO)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return QString(" (")+m_currentTrack->artist()->prettyName()+QString(" album)");
            case 1:
                return " (album)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
}

inline QString
WikipediaEngine::wikiTrackPostfix()
{
    if( wikiLocale() == "en" )
    {
        switch ( m_triedRefinedSearch )
        {
            case 0:
                return QString(" (")+m_currentTrack->artist()->prettyName()+QString(" song)");
            case 1:
                return " (song)";
            default:
                m_triedRefinedSearch = -1;
                return "";
        }
    }
    else
    {
        m_triedRefinedSearch = -1;
        return "";
    }
}

inline QString
WikipediaEngine::wikiUrl( const QString &item ) const
{
    // We now use:  http://en.wikipedia.org/w/index.php?title=The_Beatles&useskin=monobook
    // instead of:  http://en.wikipedia.org/wiki/The_Beatles
    // So that wikipedia skin is forced to default "monoskin", and the page can be parsed correctly (see BUG 205901 )
    return QString( "http://%1.wikipedia.org/w/index.php?title=" ).arg( wikiLocale() ) + KUrl::toPercentEncoding( item, "/" ) + QString( "&useskin=monobook" );
}

inline QString
WikipediaEngine::wikiSiteUrl()
{
    return QString( "http://%1.wikipedia.org/" ).arg( wikiLocale() );
}

void
WikipediaEngine::reloadWikipedia()
{
    DEBUG_BLOCK
        
    debug() << "wiki url: " << m_wikiCurrentUrl;
    removeSource( "wikipedia" );
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
}

#include "WikipediaEngine.moc"

