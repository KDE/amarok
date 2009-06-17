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

#include "WikiEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"

using namespace Context;

WikiEngine::WikiEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_requested( true )
    , m_sources( "current" )
    , m_triedRefinedSearchArtist( false )
    , m_triedRefinedSearchAlbum ( false )
    , m_triedRefinedSearchTitle ( false )
    , m_artistJob(0)
    , m_titleJob(0)
    , m_albumJob(0)
{
}

WikiEngine::~WikiEngine()
{
    DEBUG_BLOCK
}

QStringList WikiEngine::sources() const
{
    return m_sources;
}

bool WikiEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );
    if( tokens.contains( "reload" ) )
    {
        if(tokens.contains( "artist" ) )
             reloadArtistInfo();
        else if(tokens.contains( "album" ) )
             reloadAlbumInfo();
        else if(tokens.contains( "title" ) )
             reloadTitleInfo();
        else
             reloadAll();
    }
    else
    {
        removeAllData( name );
        setData( name, QVariant());
        m_sources.append(name);
        if(name.contains("artist"))
            updateArtistInfo();

        else if(name.contains("album"))
            updateAlbumInfo();

        else if(name.contains("title"))
            updateTitleInfo(); 
    }

    return true;
}

void WikiEngine :: updateArtistInfo()
{
    DEBUG_BLOCK
    QString sourceName = "wikipedia-artist";
    
    m_triedRefinedSearchArtist = false;

    QString tmpWikiStr;

    unsubscribeFrom( m_currentTrack );
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;

        if( currentTrack->artist() )
        {
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
         if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) )
    {
        tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );

        int index = tmpWikiStr.indexOf( '-' );
        if ( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( m_wikiCurrentArtistEntry == tmpWikiStr )
    {
        debug() << "Same entry requested again. Ignoring.";
        return;
    }

    removeAllData(sourceName );

    m_wikiCurrentArtistEntry = tmpWikiStr;
    m_artistUrl = wikiUrl( tmpWikiStr );

    debug() << "wiki url: " << m_artistUrl;
    
    if( currentTrack )
    {
            if( currentTrack->artist() )
            {
                setData( sourceName, "label", "Artist" );
                setData( sourceName, "title", currentTrack->artist()->prettyName() );
            }
    }
    setData(sourceName, "message", i18n( "Fetching content.." ) );
    m_artistJob = KIO::storedGet( m_artistUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_artistJob, SIGNAL( result( KJob* ) ), SLOT( artistResult( KJob* ) ) );
    

}

void WikiEngine :: artistResult(KJob *job)
{
    DEBUG_BLOCK
     QString sourceName = "wikipedia-artist";
     if( !m_artistJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if( job->error() != KJob::NoError && job == m_artistJob )
    {
        setData( "wikipedia-artist", "message", i18n( "Unable to retrieve Wikipedia information: %1", job->errorString() ) );
        m_artistJob = 0; // clear job
        return;
    }
    // not the right job, so let's ignore it
    if( job != m_artistJob )
        return;
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString wiki = storedJob->data();

    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( wiki.contains( "charset=utf-8"  ) )
        wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    if( wiki.contains( "var wgArticleId = 0" ) ) // The article does not exist
    {
        if ( m_triedRefinedSearchArtist )
        {
            debug() << "We already tried a refined search. Lets end this madness...";
            setData(sourceName, "message", i18n( "No information found..." ) );
            m_artistJob = 0;
            return;
        }

        m_triedRefinedSearchArtist = true;
        QString entry;
        debug() << "Article not found. Retrying with refinements.";

        // article was not found
        entry = m_wikiCurrentArtistEntry.left( m_wikiCurrentArtistEntry.length() - wikiArtistPostfix().length() );
        m_artistUrl = wikiUrl( entry );
        reloadArtistInfo();
        return;
    }
    parse(wiki);
    removeAllData(sourceName);
    setData( sourceName, "page", wiki);
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack )
    {
            if( currentTrack->artist() )
            {
                setData( sourceName, "label", "Artist" );
                setData( sourceName, "title", currentTrack->artist()->prettyName() );
            }
    }
    m_artistJob = 0;
  
}

void WikiEngine :: updateAlbumInfo()
{
    QString sourceName = "wikipedia-album";
    
    m_triedRefinedSearchAlbum = false;

    QString tmpWikiStr;
    unsubscribeFrom( m_currentTrack );
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;

    DataEngine::Data data;

    if( currentTrack->album() )
        {
            data[sourceName] = QString( "label" ), QString( "Album" );
            data[sourceName] = "album", currentTrack->album()->prettyName();
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
            {
                tmpWikiStr = currentTrack->album()->name();
                tmpWikiStr += wikiAlbumPostfix();
            }
        }

    if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) )
    {
        tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );

        int index = tmpWikiStr.indexOf( '-' );
        if ( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( m_wikiCurrentAlbumEntry == tmpWikiStr )
    {
        debug() << "Same entry requested again. Ignoring.";
        return;
    }

    removeAllData(sourceName );

    foreach( const QString &key, data.keys() )
        setData( key, data[key] );

    m_wikiCurrentAlbumEntry = tmpWikiStr;
    m_albumUrl = wikiUrl( tmpWikiStr );

    debug() << "wiki url: " << m_titleUrl;

    if( currentTrack->album() )
    {
         setData( sourceName, "label", "Album" );
         setData( sourceName, "title", currentTrack->album()->prettyName() );
    }
    setData( sourceName, "message", i18n( "Fetching content.." ) );
    m_albumJob = KIO::storedGet( m_albumUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_albumJob, SIGNAL( result( KJob* ) ), SLOT( albumResult( KJob* ) ) );
}

void WikiEngine :: albumResult(KJob * job)
{
    QString sourceName = "wikipedia-album";
     if( !m_albumJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if( job->error() != KJob::NoError && job == m_albumJob )
    {
        setData( sourceName, "message", i18n( "Unable to retrieve Wikipedia information: %1", job->errorString() ) );
        m_albumJob = 0; // clear job
        return;
    }
    // not the right job, so let's ignore it
    if( job != m_albumJob )
        return;
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString wiki = storedJob->data();

    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( wiki.contains( "charset=utf-8"  ) )
        wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    if( wiki.contains( "var wgArticleId = 0" ) ) // The article does not exist
    {
        if ( m_triedRefinedSearchAlbum )
        {
            debug() << "We already tried a refined search. Lets end this madness...";
            setData(sourceName, "message", i18n( "No information found..." ) );
            m_albumJob = 0;
            return;
        }

        m_triedRefinedSearchAlbum = true;
        QString entry;
        debug() << "Article not found. Retrying with refinements.";

        // article was not found
        entry = m_wikiCurrentAlbumEntry.left( m_wikiCurrentAlbumEntry.length() - wikiAlbumPostfix().length() );
        m_albumUrl = wikiUrl( entry );
        reloadAlbumInfo();
        return;
    }
    parse(wiki);
    setData( sourceName, "page", wiki);
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack->album() )
    {
         setData( sourceName, "label", "Album" );
         setData( sourceName, "title", currentTrack->album()->prettyName() );
    }
     m_albumJob = 0;
}

void WikiEngine :: updateTitleInfo()
{
    QString sourceName = "wikipedia-title";
    
    m_triedRefinedSearchTitle = false;

    QString tmpWikiStr;
    unsubscribeFrom( m_currentTrack );
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;

    DataEngine::Data data;

    tmpWikiStr = currentTrack->prettyName();
    data[sourceName] = QString( "label" ), QString( "Title" );
    data[sourceName] = "title", currentTrack->prettyName();

    if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) )
    {
        tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );

        int index = tmpWikiStr.indexOf( '-' );
        if ( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( m_wikiCurrentTitleEntry == tmpWikiStr )
    {
        debug() << "Same entry requested again. Ignoring.";
        return;
    }

    removeAllData(sourceName );

    foreach( const QString &key, data.keys() )
        setData( key, data[key] );

    m_wikiCurrentTitleEntry = tmpWikiStr;
    m_titleUrl = wikiUrl( tmpWikiStr );

    debug() << "wiki url: " << m_titleUrl;
    
    setData( sourceName, "label", "Title" );
    setData( sourceName, "title", currentTrack->prettyName() );
    setData( sourceName, "message", i18n( "Fetching content.." ) );
    m_titleJob = KIO::storedGet( m_titleUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_titleJob, SIGNAL( result( KJob* ) ), SLOT( titleResult( KJob* ) ) );
    
}

void WikiEngine :: titleResult( KJob *job )
{
    QString sourceName = "wikipedia-title";
     if( !m_titleJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if( job->error() != KJob::NoError && job == m_titleJob )
    {
        setData( sourceName, "message", i18n( "Unable to retrieve Wikipedia information: %1", job->errorString() ) );
        m_titleJob = 0; // clear job
        return;
    }
    // not the right job, so let's ignore it
    if( job != m_titleJob )
        return;
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString wiki = storedJob->data();

    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( wiki.contains( "charset=utf-8"  ) )
        wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    if( wiki.contains( "var wgArticleId = 0" ) ) // The article does not exist
    {
        if ( m_triedRefinedSearchTitle )
        {
            debug() << "We already tried a refined search. Lets end this madness...";
            setData(sourceName, "message", i18n( "No information found..." ) );
            m_titleJob = 0;
            return;
        }

        m_triedRefinedSearchTitle = true;
        QString entry;
        debug() << "Article not found. Retrying with refinements.";

        // article was not found
        entry = m_wikiCurrentTitleEntry.left( m_wikiCurrentTitleEntry.length() - wikiTrackPostfix().length() );
        m_titleUrl = wikiUrl( entry );
        reloadTitleInfo();
        return;
    }
    parse(wiki);
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    setData( sourceName, "page", wiki);
    setData( sourceName, "label", "Title" );
    setData( sourceName, "title", currentTrack->prettyName() );
    m_titleJob = 0;
}

void WikiEngine :: parse(QString& wiki)
{
     //remove the new-lines and tabs(replace with spaces IS needed).
    wiki.replace( '\n', ' ' );
    wiki.replace( '\t', ' ' );

    m_wikiLanguages.clear();
    // Get the available language list
    if ( wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") != -1 )
    {
        m_wikiLanguages = wiki.mid( wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") );
        m_wikiLanguages = m_wikiLanguages.mid( m_wikiLanguages.indexOf("<ul>") );
        m_wikiLanguages = m_wikiLanguages.mid( 0, m_wikiLanguages.indexOf( "</div>" ) );
    }

    QString copyright;
    QString copyrightMark = "<li id=\"f-copyright\">";
    if ( wiki.indexOf( copyrightMark ) != -1 )
    {
        copyright = wiki.mid( wiki.indexOf(copyrightMark) + copyrightMark.length() );
        copyright = copyright.mid( 0, copyright.indexOf( "</li>" ) );
        copyright.remove( "<br />" );
        //only one br at the beginning
        copyright.prepend( "<br />" );
    }

    // Ok lets remove the top and bottom parts of the page
    wiki = wiki.mid( wiki.indexOf( "<!-- start content -->" ) );
    wiki = wiki.mid( 0, wiki.indexOf( "<div class=\"printfooter\">" ) );

    // Adding back style and license information
    wiki = "<div id=\"bodyContent\"" + wiki;
    wiki += copyright;
    wiki.append( "</div>" );
    wiki.remove( QRegExp("<h3 id=\"siteSub\">[^<]*</h3>") );

    wiki.remove( QRegExp( "<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>" ) );

    wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );

    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    wiki.remove( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ) );

    // Remove hidden table rows as well
    QRegExp hidden( "<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>", Qt::CaseInsensitive );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    wiki.remove( hidden );

    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    wiki.remove( QRegExp( "style= *\"[^\"]*\"" ) );
    // We need to leave the classes behind, otherwise styling it ourselves gets really nasty and tedious and roughly impossible to do in a sane maner
    //wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    wiki.remove( QRegExp( "<input[^>]*>" ) );
    wiki.remove( QRegExp( "<select[^>]*>" ) );
    wiki.remove( "</select>\n"  );
    wiki.remove( QRegExp( "<option[^>]*>" ) );
    wiki.remove( "</option>\n"  );
    wiki.remove( QRegExp( "<textarea[^>]*>" ) );
    wiki.remove( "</textarea>" );

    // Make sure that the relative links inside the wikipedia HTML is forcibly made into absolute links (yes, this is deep linking, but we're showing wikipedia data as wikipedia data, not stealing any credz here)
    wiki.replace( QRegExp( "href= *\"/" ), "href=\"" + wikiSiteUrl() );
    wiki.replace( QRegExp( "src= *\"/" ), "src=\"" + wikiSiteUrl() );

    QString m_wikiHTMLSource = "<html><body>\n";
    m_wikiHTMLSource.append(wiki );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_wikiHTMLSource.append( "<br/><div id=\"wiki_otherlangs\" >" + i18n( "Wikipedia Other Languages: <br/>" )+ m_wikiLanguages + " </div>" );
    }
    m_wikiHTMLSource.append( "</body></html>\n" );

    //debug() << "wikidata: " << m_wikiHTMLSource;
    wiki = m_wikiHTMLSource;
    
}

void WikiEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
    {
        //QStringList m_sources = sources();
        if(m_sources.contains("wikipedia-artist"))
            updateArtistInfo();
        if(m_sources.contains("wikipedia-album"))
            updateAlbumInfo();
        if(m_sources.contains("wikipedia-title"))
            updateTitleInfo();
    }
}

void WikiEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

        //QStringList m_sources = sources();
        if(m_sources.contains("wikipedia-artist"))
            updateArtistInfo();
        if(m_sources.contains("wikipedia-album"))
            updateAlbumInfo();
        if(m_sources.contains("wikipedia-title"))
            updateTitleInfo();
}

inline QString
WikiEngine::wikiLocale() const
{
    // if there is no language set (QLocale::C) then return english as default
	if( m_wikiLang.language() == QLocale::C )
        return "en";
    else
        return m_wikiLang.name().split( '_' )[0];
}

inline QString
WikiEngine::wikiArtistPostfix()
{
 if( wikiLocale() == "en" )
        return " (band)";
    else if( wikiLocale() == "de" )
        return " (Band)";
    else if( wikiLocale() == "pl" )
        return " (grupa muzyczna)";
    return QString();
}

inline QString
WikiEngine::wikiAlbumPostfix()
{
        return wikiLocale() == "en" ? " (album)" : QString();
}

inline QString
WikiEngine::wikiTrackPostfix()
{
    return wikiLocale() == "en" ? " (song)" : QString();
}

inline QString
WikiEngine::wikiUrl( const QString &item ) const
{
    return QString( "http://%1.wikipedia.org/wiki/" ).arg( wikiLocale() ) + KUrl::toPercentEncoding( item, "/" );
}

inline QString
WikiEngine::wikiSiteUrl()
{
    return QString( "http://%1.wikipedia.org/" ).arg( wikiLocale() );
}


void WikiEngine :: reloadAlbumInfo()
{
  
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if(!currentTrack)
	return;

    QString sourceName = "wikipedia-album";
    removeSource( sourceName );

    if( currentTrack->album() )
    {
         setData( sourceName, "label", "Album" );
         setData( sourceName, "title", currentTrack->album()->prettyName() );
    }

    setData( sourceName, "message", i18n( "Fetching content.." ) );
    m_albumJob = KIO::storedGet( m_albumUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_albumJob, SIGNAL( result( KJob* ) ), SLOT( albumResult( KJob* ) ) );
    
}

void WikiEngine :: reloadArtistInfo()
{

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if(!currentTrack)
	return;

    QString sourceName = "wikipedia-artist";
    removeSource( sourceName );
    if( currentTrack->artist() )
            {
                setData( sourceName, "label", "Artist" );
                setData( sourceName, "title", currentTrack->artist()->prettyName() );
            }
    
    setData( sourceName, "message", i18n( "Fetching content.." ) );
    m_artistJob = KIO::storedGet( m_artistUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_artistJob, SIGNAL( result( KJob* ) ), SLOT( artistResult( KJob* ) ) );
}

void WikiEngine :: reloadTitleInfo()
{
   
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if(!currentTrack)
	  return;

    QString sourceName = "wikipedia-title";
    removeSource( sourceName );

    setData( sourceName, "label", "Title" );
    setData( sourceName, "title", currentTrack ->prettyName() );

    setData( sourceName, "message", i18n( "Fetching content.." ) );
    m_titleJob = KIO::storedGet( m_titleUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_titleJob, SIGNAL( result( KJob* ) ), SLOT( titleResult( KJob* ) ) );
}

void WikiEngine :: reloadAll()
{
    reloadArtistInfo();
    reloadAlbumInfo();
    reloadTitleInfo();
}
    
    
#include "WikiEngine.moc"

