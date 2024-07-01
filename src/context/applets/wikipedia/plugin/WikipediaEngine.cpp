/****************************************************************************************
 * Copyright (c) 2012 Ryan McCoskrie <ryan.mccoskrie@gmail.com                          *
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

#define DEBUG_PREFIX "Wikipedia"

#include "WikipediaEngine.h"

#include "EngineController.h"
#include "PaletteHandler.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <KLocalizedString>


WikipediaEngine::WikipediaEngine( QObject* parent )
    : QObject( parent )
    , currentSelection( Artist )
    , useMobileVersion( false )
    , m_pauseState( false )
{
    preferredLangs = Amarok::config(QStringLiteral("Wikipedia Applet")).readEntry( "PreferredLang", QStringList() << QStringLiteral("en") );

    EngineController *engine = The::engineController();

    _checkRequireUpdate( engine->currentTrack() );
    _paletteChanged( The::paletteHandler()->palette() );

    connect( engine, &EngineController::trackChanged,
             this, &WikipediaEngine::_checkRequireUpdate );
    connect( engine, &EngineController::trackMetadataChanged,
             this, &WikipediaEngine::_checkRequireUpdate );
    connect( engine, &EngineController::stopped,
             this, &WikipediaEngine::_stopped );
    connect( The::paletteHandler(), &PaletteHandler::newPalette,
             this, &WikipediaEngine::_paletteChanged );
    connect( The::networkAccessManager(), &NetworkAccessManagerProxy::requestRedirectedUrl,
             [=](auto url, auto redirurl) { if( urls.contains( url ) ) { urls << redirurl; } } );
}

WikipediaEngine::~WikipediaEngine()
{
}

void
WikipediaEngine::_wikiResult( const QUrl &url, const QByteArray &result, const NetworkAccessManagerProxy::Error &e )
{
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        clear();
        setMessage( i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        return;
    }

    debug() << "Received page from wikipedia:" << url;
    QString wiki( result );

    // FIXME: For now we test if we got an article or not with a test on this string "wgArticleId=0"
    // This is bad
    if( wiki.contains(QLatin1String("wgArticleId=0")) &&
        (wiki.contains(QLatin1String("wgNamespaceNumber=0")) ||
         wiki.contains(QLatin1String("wgPageName=\"Special:Badtitle\"")) ) ) // The article does not exist
    {
        debug() << "article does not exist";
        clear();
        setMessage( i18n( "No information found..." ) );
        return;
    }

    // We've found a page
    wikiParse( wiki );
    setPage( wiki );
    setBusy( false );

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack )
        return;

    if( currentSelection == Artist ) // default, or applet told us to fetch artist
    {
        if( currentTrack && currentTrack->artist() )
        {
            setTitle( currentTrack->artist()->prettyName() );
        }
    }
    else if( currentSelection == Composer )
    {
        setTitle( currentTrack->composer()->prettyName() );
    }
    else if( currentSelection == Track )
    {
        setTitle( currentTrack->prettyName() );
    }
    else if( currentSelection == Album )
    {
        if( currentTrack && currentTrack->album() )
        {
            setTitle( currentTrack->album()->prettyName() );
        }
    }
}

void
WikipediaEngine::_parseLangLinksResult( const QUrl &url, const QByteArray &data,
                                               const NetworkAccessManagerProxy::Error &e )
{
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError || data.isEmpty() )
    {
        debug() << "Parsing langlinks result failed" << e.description;
        clear();
        setMessage( i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        return;
    }

    QString hostLang = url.host();
    hostLang.remove( QLatin1String(".wikipedia.org") );
    const QString &title = QUrlQuery( url ).queryItemValue( QLatin1String("titles") );

    QHash<QString, QString> langTitleMap; // a hash of langlinks and their titles
    QString llcontinue;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == QLatin1String("page") )
        {
            if( xml.attributes().hasAttribute(QLatin1String("missing")) )
                break;

            QXmlStreamAttributes a = xml.attributes();
            if( a.hasAttribute(QLatin1String("pageid")) && a.hasAttribute(QLatin1String("title")) )
            {
                const QString &pageTitle = a.value( QLatin1String("title") ).toString();
                if( pageTitle.endsWith(QLatin1String("(disambiguation)")) )
                {
                    fetchListing( title, hostLang );
                    return;
                }
                langTitleMap[hostLang] = title;
            }

            while( !xml.atEnd() )
            {
                xml.readNext();
                if( xml.isEndElement() && xml.name() == QLatin1String("page") )
                    break;

                if( xml.isStartElement() )
                {
                    if( xml.name() == QLatin1String("ll") )
                    {
                        QXmlStreamAttributes a = xml.attributes();
                        if( a.hasAttribute(QLatin1String("lang")) )
                        {
                            QString lang = a.value( QLatin1String("lang") ).toString();
                            langTitleMap[lang] = xml.readElementText();
                        }
                    }
                    else if( xml.name() == QLatin1String("query-continue") )
                    {
                        xml.readNext();
                        if( xml.isStartElement() && xml.name() == QLatin1String("langlinks") )
                        {
                            QXmlStreamAttributes a = xml.attributes();
                            if( a.hasAttribute(QLatin1String("llcontinue")) )
                                llcontinue = a.value( QLatin1String("llcontinue") ).toString();
                        }
                    }
                }
            }
        }
    }

    if( !langTitleMap.isEmpty() )
    {
        /* When we query langlinks using a particular language, interwiki
         * results will not contain links for that language. However, it may
         * appear as part of the "page" element if there's a match or a redirect
         * has been set. So we need to manually add it here if it's still empty. */
        if( preferredLangs.contains(hostLang) && !langTitleMap.contains(hostLang) )
            langTitleMap[hostLang] = title;

        setBusy( false );
        QStringListIterator langIter( preferredLangs );
        while( langIter.hasNext() )
        {
            QString prefix = langIter.next().split( QLatin1Char(':') ).back();
            if( langTitleMap.contains(prefix) )
            {
                QString pageTitle = langTitleMap.value( prefix );
                fetchListing( pageTitle, prefix );
                return;
            }
        }
    }

    if( !llcontinue.isEmpty() )
    {
        fetchLangLinks( title, hostLang, llcontinue );
    }
    else
    {
        QRegularExpression regex( QLatin1Char('^') + hostLang + QLatin1String(".*$") );
        int index = preferredLangs.indexOf( regex );
        if( (index != -1) && (index < preferredLangs.count() - 1) )
        {
            // use next preferred language as base for fetching langlinks since
            // the current one did not get any results we want.
            QString prefix = preferredLangs.value( index + 1 ).split( QLatin1Char(':') ).back();
            fetchLangLinks( title, prefix );
        }
        else
        {
            QStringList refinePossibleLangs = preferredLangs.filter( QRegularExpression(QStringLiteral("^(en|fr|de|pl).*$")) );
            if( refinePossibleLangs.isEmpty() )
            {
                clear();
                setMessage( i18n( "No information found..." ) );
                return;
            }
            fetchListing( title, refinePossibleLangs.first().split( QLatin1Char(':') ).back() );
        }
    }
}

void
WikipediaEngine::_parseListingResult( const QUrl &url,
                                             const QByteArray &data,
                                             const NetworkAccessManagerProxy::Error &e )
{
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError || data.isEmpty() )
    {
        debug() << "Parsing listing result failed" << e.description;
        clear();
        setMessage( i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        return;
    }

    QString hostLang = url.host();
    hostLang.remove( QLatin1String(".wikipedia.org") );
    const QString &title = QUrlQuery( url ).queryItemValue( QLatin1String("srsearch") );

    QStringList titles;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == QLatin1String("search") )
        {
            while( xml.readNextStartElement() )
            {
                if( xml.name() == QLatin1String("p") )
                {
                    if( xml.attributes().hasAttribute( QLatin1String("title") ) )
                        titles << xml.attributes().value( QLatin1String("title") ).toString();
                    xml.skipCurrentElement();
                }
                else xml.skipCurrentElement();
            }
        }
    }

    if( titles.isEmpty() )
    {
        QStringList refinePossibleLangs = preferredLangs.filter( QRegularExpression(QStringLiteral("^(en|fr|de|pl).*$")) );
        int index = refinePossibleLangs.indexOf( hostLang );
        if( (index != -1) && (index < refinePossibleLangs.count() - 1) )
            fetchListing( title, refinePossibleLangs.value( index + 1 ).split( QLatin1Char(':') ).back() );
        else
        {
            clear();
            setMessage( i18n( "No information found..." ) );
        }
        return;
    }

    QString pattern;
    switch( currentSelection )
    {
    default:
    case Artist:
        pattern = i18nc("Search pattern for an artist or band", ".*\\(.*(artist|band).*\\)").toLatin1();
        break;
    case Composer:
            pattern = i18nc("Search pattern for a composer", ".*\\(.*(composer|musician).*\\)").toLatin1();
        break;
    case Album:
            pattern = i18nc("Search pattern for an album", ".*\\(.*(album|score|soundtrack).*\\)").toLatin1();
        break;

    case Track:
            pattern = i18nc("Search pattern for a song", ".*\\(.*(song|track).*\\)").toLatin1();
        break;
    }

    pattern.prepend( title );
    int patternIndex = titles.indexOf( QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption) );
    const QString result = ( patternIndex != -1 ) ? titles.at( patternIndex ) : titles.first();
    fetchWikiUrl( result, hostLang ); // end of the line
}

void
WikipediaEngine::_checkRequireUpdate( Meta::TrackPtr track )
{
    if( m_pauseState )
        return;
    if( !track )
        return;

    bool updateNeeded(false);

    switch( currentSelection )
    {
    case WikipediaEngine::Artist:
        if( track->artist() )
            updateNeeded = track->artist()->name() != m_previousTrackMetadata.artist;
        break;
    case WikipediaEngine::Composer:
        if( track->composer() )
            updateNeeded = track->composer()->name() != m_previousTrackMetadata.composer;
        break;
    case WikipediaEngine::Album:
        if( track->album() )
            updateNeeded = track->album()->name() != m_previousTrackMetadata.album;
        break;

    case WikipediaEngine::Track:
        updateNeeded = track->name() != m_previousTrackMetadata.track;
        break;
    }

    if( updateNeeded )
    {
        m_previousTrackMetadata.clear();
        if( track->artist() )
            m_previousTrackMetadata.artist = track->artist()->name();
        if( track->composer() )
            m_previousTrackMetadata.composer = track->composer()->name();
        if( track->album() )
            m_previousTrackMetadata.album = track->album()->name();
        m_previousTrackMetadata.track = track->name();

        urls.clear();
        updateEngine();
    }
}

void
WikipediaEngine::_stopped()
{
    DEBUG_BLOCK

    if( m_pauseState )
        return;

    clear();
//     dataContainer->setData( "stopped", 1 );
    m_previousTrackMetadata.clear();
}

void
WikipediaEngine::_paletteChanged( const QPalette &palette )
{
    DEBUG_BLOCK

    // read css, replace color placeholders, write to file, load into page
    QFile file( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/data/WikipediaCustomStyle.css") ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QString contents = QString( file.readAll() );
        contents.replace( QStringLiteral("/*{text_color}*/"), palette.text().color().name() );
        contents.replace( QStringLiteral("/*{link_color}*/"), palette.link().color().name() );
        contents.replace( QStringLiteral("/*{link_hover_color}*/"), palette.linkVisited().color().name() );
        contents.replace( QStringLiteral("/*{background_color}*/"), palette.base().color().name() );

        const QString abg = palette.window().color().name();
        contents.replace( QStringLiteral("/*{shaded_text_background_color}*/"), abg );
        contents.replace( QStringLiteral("/*{table_background_color}*/"), abg );
        contents.replace( QStringLiteral("/*{headings_background_color}*/"), abg );

        const QString atbg = palette.alternateBase().color().name();
        contents.replace( QStringLiteral("/*{alternate_table_background_color}*/"), atbg );

        if( m_css == contents )
            return;

        m_css = contents;
        updateEngine();
    }
    else
    {
        error() << "Could not load WikipediaCustomStyle.css";
    }
}

void
WikipediaEngine::fetchWikiUrl( const QString &title, const QString &urlPrefix )
{
    QUrl pageUrl;
    QString host( QStringLiteral(".wikipedia.org") );
    pageUrl.setScheme( QLatin1String( "https" ) );

//     if( useMobileVersion )
//     {
//         host.prepend( ".m" );
//         host.prepend( urlPrefix );
//         pageUrl.setHost( host );
//         pageUrl.setPath( QStringLiteral("/wiki/%1").arg(title) );
//         DataEngine::Data data;
//         data[QLatin1String("sourceUrl")] = pageUrl;
//         removeAllData( QLatin1String("wikipedia") );
//         setData( QLatin1String("wikipedia"), data );
//         return;
//     }

    // We now use:  http://en.wikipedia.org/w/index.php?title=The_Beatles&useskin=monobook
    // instead of:  http://en.wikipedia.org/wiki/The_Beatles
    // So that wikipedia skin is forced to default "monoskin", and the page can be parsed correctly (see BUG 205901 )
    host.prepend( urlPrefix );
    pageUrl.setHost( host );
    pageUrl.setPath( QLatin1String("/w/index.php") );
    QUrlQuery query;
    query.addQueryItem( QLatin1String("title"), title );
    query.addQueryItem( QLatin1String("redirects"), QString::number(1) );
    query.addQueryItem( QLatin1String("useskin"), QLatin1String("monobook") );
    pageUrl.setQuery( query );
    wikiCurrentUrl = pageUrl;
    urls << pageUrl;
    setMessage( QString() );
    Q_EMIT urlChanged();
    The::networkAccessManager()->getData( pageUrl, this, &WikipediaEngine::_wikiResult );
}

void
WikipediaEngine::fetchLangLinks( const QString &title,
                                 const QString &hostLang,
                                 const QString &llcontinue )
{
    QUrl url;
    url.setScheme( QLatin1String( "https" ) );
    url.setHost( hostLang + QLatin1String(".wikipedia.org") );
    url.setPath( QLatin1String("/w/api.php") );
    QUrlQuery query;
    query.addQueryItem( QLatin1String("action"), QLatin1String("query") );
    query.addQueryItem( QLatin1String("prop"), QLatin1String("langlinks") );
    query.addQueryItem( QLatin1String("titles"), title );
    query.addQueryItem( QLatin1String("format"), QLatin1String("xml") );
    query.addQueryItem( QLatin1String("lllimit"), QString::number(100) );
    query.addQueryItem( QLatin1String("redirects"), QString::number(1) );
    if( !llcontinue.isEmpty() )
        query.addQueryItem( QLatin1String("llcontinue"), llcontinue );
    url.setQuery( query );
    urls << url;
    debug() << "Fetching langlinks:" << url;
    The::networkAccessManager()->getData( url, this, &WikipediaEngine::_parseLangLinksResult );
}

void
WikipediaEngine::fetchListing( const QString &title, const QString &hostLang )
{
    QUrl url;
    url.setScheme( QLatin1String( "https" ) );
    url.setHost( hostLang + QLatin1String(".wikipedia.org") );
    url.setPath( QLatin1String("/w/api.php") );
    QUrlQuery query;
    query.addQueryItem( QLatin1String("action"), QLatin1String("query") );
    query.addQueryItem( QLatin1String("list"), QLatin1String("search") );
    query.addQueryItem( QLatin1String("srsearch"), title );
    query.addQueryItem( QLatin1String("srprop"), QLatin1String("size") );
    query.addQueryItem( QLatin1String("srlimit"), QString::number(20) );
    query.addQueryItem( QLatin1String("format"), QLatin1String("xml") );
    url.setQuery( query );
    urls << url;
    debug() << "Fetching listing:" << url;
    The::networkAccessManager()->getData( url, this, &WikipediaEngine::_parseListingResult );
}

void
WikipediaEngine::updateEngine()
{
    static QMap<SelectionType, qint64> typeToFieldMap;
    if( typeToFieldMap.isEmpty() )
    {
        typeToFieldMap.insert( Artist, Meta::valArtist );
        typeToFieldMap.insert( Composer, Meta::valComposer );
        typeToFieldMap.insert( Album, Meta::valAlbum );
        typeToFieldMap.insert( Track, Meta::valTitle );
    }

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack )
        return;

    //TODO: Look into writing one function that can be used with different arguments for each case in this switch.
    QString tmpWikiStr;
    QString notice = i18nc( "%1 is field name such as 'Artist Name'",
                            "%1 is needed for searching Wikipedia.",
                            Meta::i18nForField( typeToFieldMap.value( currentSelection ) ) );
    switch( currentSelection )
    {
    case Artist:
        if( currentTrack->artist() )
        {
            if( currentTrack->artist()->name().isEmpty() )
            {
                clear();
                setMessage( notice );
                return;
            }
            if( ( currentTrack->playableUrl().scheme() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().scheme() == QLatin1String("daap") ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->artist()->name();
            else
                tmpWikiStr = currentTrack->artist()->prettyName();
        }
        break;

    case Composer:
        if( currentTrack->composer() )
        {
            if( currentTrack->composer()->name().isEmpty() )
            {
                clear();
                setMessage( notice );
                return;
            }
            if( ( currentTrack->playableUrl().scheme() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().scheme() == QLatin1String("daap") ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->composer()->name();
            else
                tmpWikiStr = currentTrack->composer()->prettyName();
        }
        break;
    case Album:
        if( currentTrack->album() )
        {
            if( currentTrack->album()->name().isEmpty() )
            {
                clear();
                setMessage( notice );
                return;
            }
            if( ( currentTrack->playableUrl().scheme() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().scheme() == QLatin1String("daap") ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->album()->name();
            else
                tmpWikiStr = currentTrack->album()->prettyName();
        }
        break;

    case Track:
        if( currentTrack->name().isEmpty() )
        {
            clear();
            setMessage( notice );
            return;
        }
        tmpWikiStr = currentTrack->prettyName();
        break;
    }

    //Hack to make wiki searches work with magnatune preview tracks
    if( tmpWikiStr.contains( QLatin1String("PREVIEW: buy it at www.magnatune.com") ) )
    {
        tmpWikiStr = tmpWikiStr.remove(QLatin1String(" (PREVIEW: buy it at www.magnatune.com)") );
        int index = tmpWikiStr.indexOf( QLatin1Char('-') );
        if( index != -1 )
            tmpWikiStr = tmpWikiStr.left (index - 1);
    }

    if( preferredLangs.isEmpty() )
        preferredLangs = QStringList() << QLatin1String("en:en");

    fetchLangLinks( tmpWikiStr, preferredLangs.first().split( QLatin1Char(':') ).back() );
}

void
WikipediaEngine::wikiParse( QString &wiki )
{
    //remove the new-lines and tabs(replace with spaces IS needed).
//     wiki.replace( '\n', QLatin1Char(' ') );
//     wiki.replace( '\t', QLatin1Char(' ') );

    // Get the available language list
    QString wikiLanguagesSection;
    QMap<QString, QString> langMap;
    int langSectionIndex = 0;
    if( (langSectionIndex = wiki.indexOf( QLatin1String("<div id=\"p-lang\" class=\"portlet\">") )) != -1 )
    {
        int ulpos = wiki.indexOf( QLatin1String("<ul>"), langSectionIndex );
        int devpos = wiki.indexOf( QLatin1String("</dev>"), ulpos );
        wikiLanguagesSection = wiki.mid( langSectionIndex, devpos - langSectionIndex );
        QXmlStreamReader xml( wikiLanguagesSection );
        while( !xml.atEnd() && !xml.hasError() )
        {
            xml.readNext();
            if( xml.isStartElement() && xml.name() == QLatin1String("li") )
            {
                while( xml.readNextStartElement() )
                {
                    if( xml.name() == QLatin1String("a") )
                    {
                        QString url = xml.attributes().value( QLatin1String("href") ).toString();
                        langMap[ xml.readElementText() ] = url;
                    }
                    else xml.skipCurrentElement();
                }
            }
        }
    }

    QString copyright;
    const QString copyrightMark = QLatin1String("<li id=\"f-copyright\">");
    int copyrightIndex = wiki.indexOf( copyrightMark );
    if( copyrightIndex != -1 )
    {
        const uint pos = copyrightIndex + copyrightMark.length();
        copyright = wiki.mid( pos, wiki.indexOf( QLatin1String("</li>"), pos ) - pos );
        copyright.remove( QLatin1String("<br />") );
        //only one br at the beginning
        copyright.prepend( QLatin1String("<br />") );
    }

    const int titleIndex = wiki.indexOf( QRegularExpression( QLatin1String("<title>[^<]*</title>") ) ) + 7;
    const int bsTitleIndex = wiki.indexOf( QLatin1String("</title>"), titleIndex ) - titleIndex;
    const QString title = wiki.mid( titleIndex, bsTitleIndex );

    // Ok lets remove the top and bottom parts of the page
    QStringView wikiRef;
    const int contentStart = wiki.indexOf( QLatin1String("<!-- start content -->") );
    const int contentEnd = wiki.indexOf( QLatin1String("<div class=\"printfooter\">"), contentStart );

    wiki = wiki.mid( contentStart, contentEnd - contentStart );

    auto removeTag = [&wiki] ( const QString& tagStart, const QString& tagEnd )
    {
        const int tagEndSize = tagEnd.size();
        int matchIndex = 0;
        const QStringMatcher tagMatcher( tagStart );
        while( ( matchIndex = tagMatcher.indexIn( wiki, matchIndex ) ) != -1 )
        {
            const int nToTagEnd = wiki.indexOf( tagEnd, matchIndex ) - matchIndex;
            const QStringView tagRef = wiki.midRef( matchIndex, nToTagEnd + tagEndSize );
            wiki.remove( tagRef.toString() );
        }
    };

    QString header = QStringLiteral( "<html>\n<head>\n<title>%1</title>\n</head>\n<body>\n" ).arg( title );

    // add own stylesheet
    if( !m_css.isEmpty() )
    {
        removeTag( QStringLiteral("<link rel=\"stylesheet\""), QStringLiteral("/>") );
        int index = header.indexOf( QStringLiteral( "</head>" ) );
        header.insert( index, QStringLiteral( "\n<style>\n%1\n</style>\n" ).arg( m_css ) );
    }

    wiki.prepend( header );

    // lets remove the warning box
    removeTag( QStringLiteral("<table class=\"metadata plainlinks ambox"), QStringLiteral("</table>") );
    // remove protection policy (we don't do edits)
    removeTag( QStringLiteral("<div><a href=\"/wiki/Wikipedia:Protection_policy"), QStringLiteral("</a></div>") );
    // lets also remove the "lock" image
    removeTag( QStringLiteral("<div class=\"metadata topicon\" "), QStringLiteral("</a></div>") );
    // remove <audio> tags (can lead to crashes in QtWebKit)
//     removeTag( "<audio", "</audio>" );

    // Adding back style and license information
    wiki = QLatin1String("<div id=\"bodyContent\"") + wiki;
    wiki += copyright;
    wiki.append( QLatin1String("</div>") );
    wiki.remove( QRegularExpression( QLatin1String("<h3 id=\"siteSub\">[^<]*</h3>") ) );

    wiki.remove( QRegularExpression( QLatin1String("<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>") ) );
    wiki.remove( QRegularExpression( QLatin1String("<p><span[^>]*><[^\"]*\"#_skip_noteTA\">[^<]*<[^<]*</span></p>") ) );

    wiki.replace( QRegularExpression( QLatin1String("<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>") ), QLatin1String("\\1") );

    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    wiki.remove( QRegularExpression( QLatin1String("<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>") ) );

    // Remove hidden table rows as well, greedy behaviour wouldn't be any good!
    QRegularExpression hidden( QLatin1String("<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>"), QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption );

    wiki.remove( hidden );

    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    wiki.remove( QRegularExpression( QLatin1String("style= *\"[^\"]*\"") ) );
    // We need to leave the classes behind, otherwise styling it ourselves gets really nasty and tedious and roughly impossible to do in a sane manner
    //wiki.replace( QRegularExpression( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    wiki.remove( QRegularExpression( QLatin1String("<input[^>]*>") ) );
    wiki.remove( QRegularExpression( QLatin1String("<select[^>]*>") ) );
    wiki.remove( QLatin1String("</select>\n")  );
    wiki.remove( QRegularExpression( QLatin1String("<option[^>]*>") ) );
    wiki.remove( QLatin1String("</option>\n")  );
    wiki.remove( QRegularExpression( QLatin1String("<textarea[^>]*>") ) );
    wiki.remove( QLatin1String("</textarea>") );

    // wiki.append( createLanguageComboBox(langMap) ); // BUG:259075
    wiki.append( QLatin1String("</body></html>\n") );
}

QString
WikipediaEngine::createLanguageComboBox( const QMap<QString, QString> &languageMap )
{
    if( languageMap.isEmpty() )
        return QString();

    QString html;
    QMapIterator<QString, QString> i(languageMap);
    while( i.hasNext() )
    {
        i.next();
        html += QStringLiteral( "<option value=\"%1\">%2</option>" ).arg( i.value(), i.key() );
    }
    html.prepend( QStringLiteral("<form name=\"langform\"><select name=\"links\" size=\"1\">") );
    html.append( QStringLiteral("/select><input type=\"button\" value=\"%1\" ").arg( i18n("Choose Language") ) );
    html.append( QStringLiteral("onClick=\"mWebPage.loadWikipediaUrl(document.langform.links.options[document.langform.links.selectedIndex].value);\"></form>") );
    return html;
}

void
WikipediaEngine::reloadWikipedia()
{
    if( !wikiCurrentUrl.isValid() )
        return;
    urls << wikiCurrentUrl;
    setBusy( true );
    The::networkAccessManager()->getData( wikiCurrentUrl, this, &WikipediaEngine::_wikiResult );
}

WikipediaEngine::SelectionType
WikipediaEngine::selection() const
{
    return currentSelection;
}

bool
WikipediaEngine::setSelection( SelectionType type )
{
    if( currentSelection == type )
        return false;

    currentSelection = type;
    Q_EMIT selectionChanged();

    updateEngine();

    return true;
}

bool
WikipediaEngine::setSelection( const QString &type )
{
    bool changed( false );
    if( type == QLatin1String("artist") )
        changed = setSelection( Artist );
    else if( type == QLatin1String("composer") )
        changed = setSelection( Composer );
    else if( type == QLatin1String("album") )
        changed = setSelection( Album );
    else if( type == QLatin1String("track") )
        changed = setSelection( Track );
    return changed;
}

void
WikipediaEngine::setPage(const QString& page)
{
    if( m_page == page )
        return;

    m_page = page;
    Q_EMIT pageChanged();
}

void
WikipediaEngine::setMessage(const QString& message)
{
    if( m_message == message )
        return;

    m_message = message;
    Q_EMIT messageChanged();
}


void
WikipediaEngine::setBusy(bool busy)
{
    if( m_busy == busy )
        return;

    m_busy = busy;
    Q_EMIT busyChanged();
}

bool
WikipediaEngine::pauseState() const
{
    return m_pauseState;
}

void
WikipediaEngine::setPauseState( const bool state )
{
    m_pauseState = state;
    if( !m_pauseState )
        _checkRequireUpdate(  The::engineController()->currentTrack() );
}

void
WikipediaEngine::setTitle(const QString& title)
{
    if( m_title == title )
        return;

    m_title = title;
    Q_EMIT titleChanged();
}

void
WikipediaEngine::clear()
{
    setPage( QString() );
    setBusy( false );
    setTitle( QString() );
}

void
WikipediaEngine::setLanguage(const QString& language)
{
    if( preferredLangs.first() == language )
        return;

    preferredLangs.removeAll( language );
    preferredLangs.prepend( language );
    Q_EMIT languageChanged();

    updateEngine();
}

void
WikipediaEngine::setUrl(const QUrl& url)
{
    if( !url.host().endsWith( QStringLiteral(".wikipedia.org") ) )
    {
        setMessage( QStringLiteral( "<a href=\"%1\">").arg( url.toString() ) +
            i18nc( "A message (link) shown for clicked non-Wikipedia links in Wikipedia applet. %1 is the url of the link",
                   "Click to open %1 in a web browser", url.toString() ) + QStringLiteral("</a>") );
        return;
    }
    QUrl monobookUrl = url;
    monobookUrl.setPath( QLatin1String("/w/index.php") );

    QUrlQuery query;
    query.addQueryItem( QLatin1String("title"), url.path().mid( url.path().lastIndexOf(QStringLiteral("/")) + 1 ) );
    query.addQueryItem( QLatin1String("redirects"), QString::number(1) );
    query.addQueryItem( QLatin1String("useskin"), QLatin1String("monobook") );
    monobookUrl.setQuery( query );

    if( wikiCurrentUrl == monobookUrl )
        return;

    wikiCurrentUrl = monobookUrl;
    urls << monobookUrl;
    setMessage( QString() );
    Q_EMIT urlChanged();

    The::networkAccessManager()->getData( monobookUrl, this, &WikipediaEngine::_wikiResult );
    setBusy( true );
}

