/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 * copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>  *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "WikipediaEngine.h"

#include "Amarok.h"
#include "Debug.h"
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
    , m_triedRefinedSearch( false )
{
    update();
}

WikipediaEngine::~WikipediaEngine()
{
    DEBUG_BLOCK
}

QStringList WikipediaEngine::sources() const
{
    return m_sources;
}

bool WikipediaEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK
    Q_UNUSED( name )

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );
    if( tokens.contains( "reload" ) )
    {
        reloadWikipedia();
    }
    else
    {
        removeAllData( name );
        setData( name, QVariant());
        update();
    }

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

    m_triedRefinedSearch = false;
    
    QString tmpWikiStr;

    unsubscribeFrom( m_currentTrack );
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;
    
    DataEngine::Data data;

    if( selection() == "artist" ) // default, or applet told us to fetch artist
    {
        data["wikipedia"] = "label, artist";
        if( currentTrack->artist() )
        {
            data["wikipedia"] = "title", currentTrack->artist()->prettyName();

            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
            {
                tmpWikiStr = currentTrack->artist()->name();
                tmpWikiStr += wikiArtistPostfix(); //makes wikipedia bail out

                debug() << "tmpWikiStr: " << tmpWikiStr;
            }
            else
            {
                tmpWikiStr = currentTrack->artist()->prettyName();
                tmpWikiStr += wikiArtistPostfix(); //makes wikipedia bail out
            }
        }
    }
    else if( selection() == "title" )
    {
        tmpWikiStr = currentTrack->prettyName();
        data["wikipedia"] = QString( "label" ), QString( "Title" );
        data["wikipedia"] = "title", currentTrack->prettyName();
    }
    else if( selection() == "album" )
    {
        if( currentTrack->album() )
        {
            data["wikipedia"] = QString( "label" ), QString( "Album" );
            data["wikipedia"] = "title", currentTrack->album()->prettyName();
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
            {
                tmpWikiStr = currentTrack->album()->name();
                tmpWikiStr += wikiAlbumPostfix();
            }
        }
    }

    //Hack to make wiki searches work with magnatune preview tracks

    if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) )
    {
        tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );

        int index = tmpWikiStr.indexOf( '-' );
        if ( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( m_wikiCurrentEntry == tmpWikiStr )
    {
        debug() << "Same entry requested again. Ignoring.";
        return;
    }

    removeAllData( "wikipedia" );

    foreach( const QString &key, data.keys() )
        setData( key, data[key] );

    m_wikiCurrentEntry = tmpWikiStr;
    m_wikiCurrentUrl = wikiUrl( tmpWikiStr );

    debug() << "wiki url: " << m_wikiCurrentUrl;

    setData( "wikipedia", "message", i18n( "Fetching content.." ) );
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

    if( m_wiki.contains( "var wgArticleId = 0" ) ) // The article does not exist
    {
        if ( m_triedRefinedSearch )
        {
            debug() << "We already tried a refined search. Lets end this madness...";
            setData( "wikipedia", "message", i18n( "No information found..." ) );
            m_wikiJob = 0;
            return;
        }

        m_triedRefinedSearch = true;
        QString entry;
        debug() << "Article not found. Retrying with refinements.";

        // article was not found
        if( m_wikiCurrentEntry.endsWith( wikiArtistPostfix() ) )
            entry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiArtistPostfix().length() );
        else if( m_wikiCurrentEntry.endsWith( wikiAlbumPostfix() ) )
            entry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiAlbumPostfix().length() );
        else if( m_wikiCurrentEntry.endsWith( wikiTrackPostfix() ) )
            entry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiTrackPostfix().length() );

        m_wikiCurrentUrl = wikiUrl( entry );
        reloadWikipedia();
        return;
    }

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

    // Make sure that the relative links inside the wikipedia HTML is forcibly made into absolute links (yes, this is deep linking, but we're showing wikipedia data as wikipedia data, not stealing any credz here)
    m_wiki.replace( QRegExp( "href= *\"/" ), "href=\"" + wikiSiteUrl() );
    m_wiki.replace( QRegExp( "src= *\"/" ), "src=\"" + wikiSiteUrl() );
    
    QString m_wikiHTMLSource = "<html><body>\n";
    m_wikiHTMLSource.append( m_wiki );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_wikiHTMLSource.append( "<br/><div id=\"wiki_otherlangs\" >" + i18n( "Wikipedia Other Languages: <br/>" )+ m_wikiLanguages + " </div>" );
    }
    m_wikiHTMLSource.append( "</body></html>\n" );

    //debug() << "wikidata: " << m_wikiHTMLSource;

    removeAllData( "wikipedia" );
    setData( "wikipedia", "page", m_wikiHTMLSource );

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack )
    {
        if( selection() == "artist" ) // default, or applet told us to fetch artist
        {
            if( currentTrack->artist() )
            {
                setData( "wikipedia", "label", "Artist" );
                setData( "wikipedia", "title", currentTrack->artist()->prettyName() );
            }
        }
        else if( selection() == "title" )
        {
            setData( "wikipedia", "label", "Title" );
            setData( "wikipedia", "title", currentTrack->prettyName() );
        }
        else if( selection() == "album" )
        {
            if( currentTrack->album() )
            {
                setData( "wikipedia", "label", "Album" );
                setData( "wikipedia", "title", currentTrack->album()->prettyName() );
            }
        }
    }

    m_wikiJob = 0;
}

inline QString
WikipediaEngine::wikiLocale() const
{
    // if there is no language set (QLocale::C) then return english as default
	if( m_wikiLang.language() == QLocale::C )
        return "en";
    else
        return m_wikiLang.name().split( "_" )[0];
}

inline QString
WikipediaEngine::wikiArtistPostfix()
{
    if( wikiLocale() == "en" )
        return " (band)";
    else if( wikiLocale() == "de" )
        return " (Band)";
    else if( wikiLocale() == "pl" )
        return " (Kapela)";
    return QString();
}

inline QString
WikipediaEngine::wikiAlbumPostfix()
{
    return wikiLocale() == "en" ? " (album)" : QString();
}

inline QString
WikipediaEngine::wikiTrackPostfix()
{
    return wikiLocale() == "en" ? " (song)" : QString();
}

inline QString
WikipediaEngine::wikiUrl( const QString &item ) const
{
    return QString( "http://%1.wikipedia.org/wiki/" ).arg( wikiLocale() ) + KUrl::toPercentEncoding( item, "/" );
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
    setData( "wikipedia", "message", i18n( "Fetching content.." ) );
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
}

#include "WikipediaEngine.moc"

