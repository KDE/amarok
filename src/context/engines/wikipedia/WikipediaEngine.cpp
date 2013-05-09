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

#define DEBUG_PREFIX "WikipediaEngine"

#include "WikipediaEngine.h"

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <Plasma/DataContainer>

#include <QHashIterator>
#include <QXmlStreamReader>

using namespace Context;

class WikipediaEnginePrivate
{
private:
    WikipediaEngine *const q_ptr;
    Q_DECLARE_PUBLIC( WikipediaEngine )

public:
    WikipediaEnginePrivate( WikipediaEngine *parent )
        : q_ptr( parent )
        , currentSelection( Artist )
        , useMobileVersion( false )
        , useSSL( true )
        , dataContainer( 0 )
    {}
    ~WikipediaEnginePrivate() {}

    enum SelectionType
    {
        Artist,
        Composer,
        Album,
        Track
    };

    // functions
    void fetchWikiUrl( const QString &title, const QString &urlPrefix );
    void fetchLangLinks( const QString &title, const QString &hostLang, const QString &llcontinue = QString() );
    void fetchListing( const QString &title, const QString &hostLang );
    void reloadWikipedia();
    bool setSelection( SelectionType type ); // returns true if selection is changed
    bool setSelection( const QString &type );
    SelectionType selection() const;
    void updateEngine();
    void wikiParse( QString &page );
    QString createLanguageComboBox( const QMap<QString, QString> &languageMap );

    // data members
    SelectionType currentSelection;
    QUrl wikiCurrentUrl;
    QStringList preferredLangs;
    struct TrackMetadata
    {
        QString artist;
        QString composer;
        QString album;
        QString track;
        void clear()
        {
            artist.clear();
            composer.clear();
            album.clear();
            track.clear();
        }
    } m_previousTrackMetadata;
    bool useMobileVersion;
    bool useSSL;

    Plasma::DataContainer *dataContainer;

    QSet< QUrl > urls;

    // private slots
    void _checkRequireUpdate( Meta::TrackPtr track );
    void _dataContainerUpdated( const QString &source, const Plasma::DataEngine::Data &data );
    void _parseLangLinksResult( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void _parseListingResult( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void _wikiResult( const KUrl &url, QByteArray result, NetworkAccessManagerProxy::Error e );
    void _stopped();
};

void
WikipediaEnginePrivate::_dataContainerUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    DEBUG_BLOCK
    Q_Q( WikipediaEngine );

    if( source != QLatin1String("wikipedia") )
        return;

    if( data.isEmpty() )
    {
        debug() << "data is empty";
        return;
    }

    if( data.contains( QLatin1String("reload") ) )
    {
        if( data.value( QLatin1String("reload") ).toBool() )
        {
            debug() << QLatin1String("reloading");
            reloadWikipedia();
        }
        q->removeData( source, QLatin1String("reload") );
    }

    if( data.contains( QLatin1String("goto") ) )
    {
        QString gotoType = data.value( QLatin1String("goto") ).toString();
        debug() << "goto:" << gotoType;
        if( !gotoType.isEmpty() )
        {
            setSelection( gotoType );
            q->setData( source, QLatin1String("busy"), true );
            updateEngine();
        }
        q->removeData( source, QLatin1String("goto") );
    }

    if( data.contains( QLatin1String("clickUrl") ) )
    {
        QUrl clickUrl = data.value( QLatin1String("clickUrl") ).toUrl();
        debug() << "clickUrl:" << clickUrl;
        if( clickUrl.isValid() )
        {
            wikiCurrentUrl = clickUrl;
            if( !wikiCurrentUrl.hasQueryItem( QLatin1String("useskin") ) )
                wikiCurrentUrl.addQueryItem( QLatin1String("useskin"), QLatin1String("monobook") );
            KUrl encodedUrl( wikiCurrentUrl.toEncoded() );
            urls << encodedUrl;
            q->setData( source, QLatin1String("busy"), true );
            The::networkAccessManager()->getData( encodedUrl, q,
                 SLOT(_wikiResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
        q->removeData( source, QLatin1String("clickUrl") );
    }

    if( data.contains( QLatin1String("mobile") ) )
    {
        bool mobile = data.value( QLatin1String("mobile") ).toBool();
        if( mobile != useMobileVersion )
        {
            debug() << (mobile ? "switching to mobile wikipedia" : "switching to normal wikipedia");
            useMobileVersion = mobile;
            updateEngine();
        }
    }

    if( data.contains( QLatin1String("ssl") ) )
    {
        const bool ssl = data.value( QLatin1String("ssl") ).toBool();
        if( ssl != useSSL )
        {
            useSSL = ssl;
            updateEngine();
        }
    }

    if( data.contains( QLatin1String("lang") ) )
    {
        QStringList langList = data.value( QLatin1String("lang") ).toStringList();
        if( !langList.isEmpty() && (preferredLangs != langList) )
        {
            preferredLangs = langList;
            updateEngine();
            debug() << QLatin1String("updated preferred wikipedia languages:") << preferredLangs;
        }
        q->removeData( source, QLatin1String("lang") );
    }
}

void
WikipediaEnginePrivate::_wikiResult( const KUrl &url, QByteArray result, NetworkAccessManagerProxy::Error e )
{
    Q_Q( WikipediaEngine );
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError )
    {
        q->removeAllData( QLatin1String("wikipedia") );
        q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                    i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        q->scheduleSourcesUpdated();
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
        q->removeAllData( QLatin1String("wikipedia") );
        q->setData( QLatin1String("wikipedia"), QLatin1String("message"), i18n( "No information found..." ) );
        q->scheduleSourcesUpdated();
        return;
    }

    // We've found a page
    DataEngine::Data data;
    wikiParse( wiki );
    data[QLatin1String("page")] = wiki;
    data[QLatin1String("url")] = QUrl(url);
    q->removeData( QLatin1String("wikipedia"), QLatin1String("busy") );

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack )
        return;

    if( currentSelection == Artist ) // default, or applet told us to fetch artist
    {
        if( currentTrack && currentTrack->artist() )
        {
            data[QLatin1String("label")] =  QLatin1String("Artist");
            data[QLatin1String("title")] = currentTrack->artist()->prettyName();
        }
    }
    else if( currentSelection == Composer )
    {
        data[QLatin1String("label")] = QLatin1String("Title");
        data[QLatin1String("title")] = currentTrack->composer()->prettyName();
    }
    else if( currentSelection == Track )
    {
        data[QLatin1String("label")] = QLatin1String("Title");
        data[QLatin1String("title")] = currentTrack->prettyName();
    }
    else if( currentSelection == Album )
    {
        if( currentTrack && currentTrack->album() )
        {
            data[QLatin1String("label")] = QLatin1String("Album");
            data[QLatin1String("title")] = currentTrack->album()->prettyName();
        }
    }
    q->setData( QLatin1String("wikipedia"), data );
    q->scheduleSourcesUpdated();
}

void
WikipediaEnginePrivate::_parseLangLinksResult( const KUrl &url, QByteArray data,
                                               NetworkAccessManagerProxy::Error e )
{
    Q_Q( WikipediaEngine );
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError || data.isEmpty() )
    {
        debug() << "Parsing langlinks result failed" << e.description;
        q->removeAllData( QLatin1String("wikipedia") );
        q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                    i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        q->scheduleSourcesUpdated();
        return;
    }

    QString hostLang = url.host();
    hostLang.remove( QLatin1String(".wikipedia.org") );
    const QString &title = url.queryItemValue( QLatin1String("titles") );

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

        q->removeData( QLatin1String("wikipedia"), QLatin1String("busy") );
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
        QRegExp regex( QLatin1Char('^') + hostLang + QLatin1String(".*$") );
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
            QStringList refinePossibleLangs = preferredLangs.filter( QRegExp("^(en|fr|de|pl).*$") );
            if( refinePossibleLangs.isEmpty() )
            {
                q->removeAllData( QLatin1String("wikipedia") );
                q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                            i18n( "No information found..." ) );
                q->scheduleSourcesUpdated();
                return;
            }
            fetchListing( title, refinePossibleLangs.first().split( QLatin1Char(':') ).back() );
        }
    }
}

void
WikipediaEnginePrivate::_parseListingResult( const KUrl &url,
                                             QByteArray data,
                                             NetworkAccessManagerProxy::Error e )
{
    Q_Q( WikipediaEngine );
    if( !urls.contains( url ) )
        return;

    urls.remove( url );
    if( e.code != QNetworkReply::NoError || data.isEmpty() )
    {
        debug() << "Parsing listing result failed" << e.description;
        q->removeAllData( QLatin1String("wikipedia") );
        q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                    i18n("Unable to retrieve Wikipedia information: %1", e.description) );
        q->scheduleSourcesUpdated();
        return;
    }

    QString hostLang = url.host();
    hostLang.remove( QLatin1String(".wikipedia.org") );
    const QString &title = url.queryItemValue( QLatin1String("srsearch") );

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
        QStringList refinePossibleLangs = preferredLangs.filter( QRegExp("^(en|fr|de|pl).*$") );
        int index = refinePossibleLangs.indexOf( hostLang );
        if( (index != -1) && (index < refinePossibleLangs.count() - 1) )
            fetchListing( title, refinePossibleLangs.value( index + 1 ).split( QLatin1Char(':') ).back() );
        else
        {
            q->removeAllData( QLatin1String("wikipedia") );
            q->setData( QLatin1String("wikipedia"), QLatin1String("message"), i18n( "No information found..." ) );
        }
        return;
    }

    QString pattern;
    switch( currentSelection )
    {
    default:
    case Artist:
        pattern = i18nc("Search pattern for an artist or band", ".*\\(.*(artist|band).*\\))").toLatin1();
        break;
    case Composer:
            pattern = i18nc("Search pattern for a composer", ".*\\(.*(composer|musician).*\\))").toLatin1();
        break;
    case Album:
            pattern = i18nc("Search pattern for an album", ".*\\(.*(album|score|soundtrack).*\\)").toLatin1();
        break;

    case Track:
            pattern = i18nc("Search pattern for a song", ".*\\(.*(song|track).*\\)").toLatin1();
        break;
    }

    pattern.prepend( title );
    int patternIndex = titles.indexOf( QRegExp(pattern, Qt::CaseInsensitive) );
    const QString result = ( patternIndex != -1 ) ? titles.at( patternIndex ) : titles.first();
    fetchWikiUrl( result, hostLang ); // end of the line
}

void
WikipediaEnginePrivate::_checkRequireUpdate( Meta::TrackPtr track )
{
    if( !track )
        return;

    bool updateNeeded(false);

    switch( currentSelection )
    {
    case WikipediaEnginePrivate::Artist:
        if( track->artist() )
            updateNeeded = track->artist()->name() != m_previousTrackMetadata.artist;
        break;
    case WikipediaEnginePrivate::Composer:
        if( track->composer() )
            updateNeeded = track->composer()->name() != m_previousTrackMetadata.composer;
        break;
    case WikipediaEnginePrivate::Album:
        if( track->album() )
            updateNeeded = track->album()->name() != m_previousTrackMetadata.album;
        break;

    case WikipediaEnginePrivate::Track:
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
WikipediaEnginePrivate::_stopped()
{
    DEBUG_BLOCK
    Q_Q( WikipediaEngine );
    dataContainer->removeAllData();
    dataContainer->setData( "stopped", 1 );
    q->scheduleSourcesUpdated();
    m_previousTrackMetadata.clear();
}

void
WikipediaEnginePrivate::fetchWikiUrl( const QString &title, const QString &urlPrefix )
{
    Q_Q( WikipediaEngine );
    KUrl pageUrl;
    QString host( ".wikipedia.org" );
    pageUrl.setScheme( useSSL ? QLatin1String( "https" ) : QLatin1String( "http" ) );

    if( useMobileVersion )
    {
        host.prepend( ".m" );
        host.prepend( urlPrefix );
        pageUrl.setHost( host );
        pageUrl.setPath( QString("/wiki/%1").arg(title) );
        DataEngine::Data data;
        data[QLatin1String("sourceUrl")] = pageUrl;
        q->removeAllData( QLatin1String("wikipedia") );
        q->setData( QLatin1String("wikipedia"), data );
        q->scheduleSourcesUpdated();
        return;
    }

    // We now use:  http://en.wikipedia.org/w/index.php?title=The_Beatles&useskin=monobook
    // instead of:  http://en.wikipedia.org/wiki/The_Beatles
    // So that wikipedia skin is forced to default "monoskin", and the page can be parsed correctly (see BUG 205901 )
    host.prepend( urlPrefix );
    pageUrl.setHost( host );
    pageUrl.setPath( QLatin1String("/w/index.php") );
    pageUrl.addQueryItem( QLatin1String("title"), title );
    pageUrl.addQueryItem( QLatin1String("redirects"), QString::number(1) );
    pageUrl.addQueryItem( QLatin1String("useskin"), QLatin1String("monobook") );
    wikiCurrentUrl = pageUrl;
    urls << pageUrl;
    The::networkAccessManager()->getData( pageUrl, q,
         SLOT(_wikiResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
WikipediaEnginePrivate::fetchLangLinks( const QString &title,
                                        const QString &hostLang,
                                        const QString &llcontinue )
{
    Q_Q( WikipediaEngine );
    KUrl url;
    url.setScheme( useSSL ? QLatin1String( "https" ) : QLatin1String( "http" ) );
    url.setHost( hostLang + QLatin1String(".wikipedia.org") );
    url.setPath( QLatin1String("/w/api.php") );
    url.addQueryItem( QLatin1String("action"), QLatin1String("query") );
    url.addQueryItem( QLatin1String("prop"), QLatin1String("langlinks") );
    url.addQueryItem( QLatin1String("titles"), title );
    url.addQueryItem( QLatin1String("format"), QLatin1String("xml") );
    url.addQueryItem( QLatin1String("lllimit"), QString::number(100) );
    url.addQueryItem( QLatin1String("redirects"), QString::number(1) );
    if( !llcontinue.isEmpty() )
        url.addQueryItem( QLatin1String("llcontinue"), llcontinue );
    urls << url;
    debug() << "Fetching langlinks:" << url;
    The::networkAccessManager()->getData( url, q,
         SLOT(_parseLangLinksResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
WikipediaEnginePrivate::fetchListing( const QString &title, const QString &hostLang )
{
    Q_Q( WikipediaEngine );
    KUrl url;
    url.setScheme( useSSL ? QLatin1String( "https" ) : QLatin1String( "http" ) );
    url.setHost( hostLang + QLatin1String(".wikipedia.org") );
    url.setPath( QLatin1String("/w/api.php") );
    url.addQueryItem( QLatin1String("action"), QLatin1String("query") );
    url.addQueryItem( QLatin1String("list"), QLatin1String("search") );
    url.addQueryItem( QLatin1String("srsearch"), title );
    url.addQueryItem( QLatin1String("srprop"), QLatin1String("size") );
    url.addQueryItem( QLatin1String("srredirects"), QString::number(1) );
    url.addQueryItem( QLatin1String("srlimit"), QString::number(20) );
    url.addQueryItem( QLatin1String("format"), QLatin1String("xml") );
    urls << url;
    debug() << "Fetching listing:" << url;
    The::networkAccessManager()->getData( url, q,
         SLOT(_parseListingResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
WikipediaEnginePrivate::updateEngine()
{
    static QMap<SelectionType, qint64> typeToFieldMap;
    if( typeToFieldMap.isEmpty() )
    {
        typeToFieldMap.insert( Artist, Meta::valArtist );
        typeToFieldMap.insert( Composer, Meta::valComposer );
        typeToFieldMap.insert( Album, Meta::valAlbum );
        typeToFieldMap.insert( Track, Meta::valTitle );
    }

    Q_Q( WikipediaEngine );

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
                q->removeAllData( QLatin1String("wikipedia") );
                q->scheduleSourcesUpdated();
                q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                            notice );
                return;
            }
            if( ( currentTrack->playableUrl().protocol() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().protocol() == QLatin1String("daap") ) ||
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
                q->removeAllData( QLatin1String("wikipedia") );
                q->scheduleSourcesUpdated();
                q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                            notice );
                return;
            }
            if( ( currentTrack->playableUrl().protocol() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().protocol() == QLatin1String("daap") ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->composer()->name();
        }
        break;
    case Album:
        if( currentTrack->album() )
        {
            if( currentTrack->album()->name().isEmpty() )
            {
                q->removeAllData( QLatin1String("wikipedia") );
                q->scheduleSourcesUpdated();
                q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                            notice );
                return;
            }
            if( ( currentTrack->playableUrl().protocol() == QLatin1String("lastfm") ) ||
                ( currentTrack->playableUrl().protocol() == QLatin1String("daap") ) ||
                !The::engineController()->isStream() )
                tmpWikiStr = currentTrack->album()->name();

        }
        break;

    case Track:
        if( currentTrack->name().isEmpty() )
        {
            q->removeAllData( QLatin1String("wikipedia") );
            q->scheduleSourcesUpdated();
            q->setData( QLatin1String("wikipedia"), QLatin1String("message"),
                        notice );
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
WikipediaEnginePrivate::wikiParse( QString &wiki )
{
    //remove the new-lines and tabs(replace with spaces IS needed).
    wiki.replace( '\n', QLatin1Char(' ') );
    wiki.replace( '\t', QLatin1Char(' ') );

    // Get the available language list
    QString wikiLanguagesSection;
    QMap<QString, QString> langMap;
    int langSectionIndex = 0;
    if( (langSectionIndex = wiki.indexOf( QLatin1String("<div id=\"p-lang\" class=\"portlet\">") )) != -1 )
    {
        QStringRef sref = wiki.midRef( langSectionIndex );
        sref = wiki.midRef( sref.position(), wiki.indexOf( QLatin1String("<ul>"), sref.position() ) - sref.position() );
        sref = wiki.midRef( sref.position(), wiki.indexOf( QLatin1String("</dev>"), sref.position() ) - sref.position() );
        wikiLanguagesSection = sref.toString();
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
        QStringRef sref = wiki.midRef( copyrightIndex + copyrightMark.length() );
        sref = wiki.midRef( sref.position(), wiki.indexOf( QLatin1String("</li>"), sref.position() ) - sref.position() );
        copyright = sref.toString();
        copyright.remove( QLatin1String("<br />") );
        //only one br at the beginning
        copyright.prepend( QLatin1String("<br />") );
    }

    const int titleIndex = wiki.indexOf( QRegExp( QLatin1String("<title>[^<]*</title>") ) ) + 7;
    const int bsTitleIndex = wiki.indexOf( QLatin1String("</title>"), titleIndex ) - titleIndex;
    const QString title = wiki.mid( titleIndex, bsTitleIndex );

    // Ok lets remove the top and bottom parts of the page
    QStringRef wikiRef;
    wikiRef = wiki.midRef( wiki.indexOf( QLatin1String("<!-- start content -->") ) );
    wikiRef = wiki.midRef( wikiRef.position(), wiki.indexOf( QLatin1String("<div class=\"printfooter\">"), wikiRef.position() ) - wikiRef.position() );
    wiki = wikiRef.toString();

    auto removeTag = [&wiki] ( const QString& tagStart, const QString& tagEnd )
    {
        const int tagEndSize = tagEnd.size();
        int matchIndex = 0;
        const QStringMatcher tagMatcher( tagStart );
        while( ( matchIndex = tagMatcher.indexIn( wiki, matchIndex ) ) != -1 )
        {
            const int nToTagEnd = wiki.indexOf( tagEnd, matchIndex ) - matchIndex;
            const QStringRef tagRef = wiki.midRef( matchIndex, nToTagEnd + tagEndSize );
            wiki.remove( tagRef.toString() );
        }
    };

    // lets remove the warning box
    removeTag( "<table class=\"metadata plainlinks ambox", "</table>" );
    // remove protection policy (we don't do edits)
    removeTag( "<div><a href=\"/wiki/Wikipedia:Protection_policy", "</a></div>" );
    // lets also remove the "lock" image
    removeTag( "<div class=\"metadata topicon\" ", "</a></div>" );
    // remove <audio> tags (can lead to crashes in QtWebKit)
    removeTag( "<audio", "</audio>" );

    // Adding back style and license information
    wiki = QLatin1String("<div id=\"bodyContent\"") + wiki;
    wiki += copyright;
    wiki.append( QLatin1String("</div>") );
    wiki.remove( QRegExp( QLatin1String("<h3 id=\"siteSub\">[^<]*</h3>") ) );

    wiki.remove( QRegExp( QLatin1String("<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>") ) );
    wiki.remove( QRegExp( QLatin1String("<p><span[^>]*><[^\"]*\"#_skip_noteTA\">[^<]*<[^<]*</span></p>") ) );

    wiki.replace( QRegExp( QLatin1String("<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>") ), QLatin1String("\\1") );

    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    wiki.remove( QRegExp( QLatin1String("<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>") ) );

    // Remove hidden table rows as well
    QRegExp hidden( QLatin1String("<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>"), Qt::CaseInsensitive );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    wiki.remove( hidden );

    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    wiki.remove( QRegExp( QLatin1String("style= *\"[^\"]*\"") ) );
    // We need to leave the classes behind, otherwise styling it ourselves gets really nasty and tedious and roughly impossible to do in a sane maner
    //wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    wiki.remove( QRegExp( QLatin1String("<input[^>]*>") ) );
    wiki.remove( QRegExp( QLatin1String("<select[^>]*>") ) );
    wiki.remove( QLatin1String("</select>\n")  );
    wiki.remove( QRegExp( QLatin1String("<option[^>]*>") ) );
    wiki.remove( QLatin1String("</option>\n")  );
    wiki.remove( QRegExp( QLatin1String("<textarea[^>]*>") ) );
    wiki.remove( QLatin1String("</textarea>") );

    wiki.prepend( QLatin1String("<html>\n") );
    wiki.append( QString(QLatin1String("<head><title>%1</title></head>\n")).arg(title) );
    wiki.append( QLatin1String("<body>\n") );
    // wiki.append( createLanguageComboBox(langMap) ); // BUG:259075
    wiki.append( QLatin1String("</body></html>\n") );
}

QString
WikipediaEnginePrivate::createLanguageComboBox( const QMap<QString, QString> &languageMap )
{
    if( languageMap.isEmpty() )
        return QString();

    QString html;
    QMapIterator<QString, QString> i(languageMap);
    while( i.hasNext() )
    {
        i.next();
        html += QString( "<option value=\"%1\">%2</option>" ).arg( i.value(), i.key() );
    }
    html.prepend( QString("<form name=\"langform\"><select name=\"links\" size=\"1\">") );
    html.append( QString("/select><input type=\"button\" value=\"%1\" ").arg( i18n("Choose Language") ) );
    html.append( QString("onClick=\"mWebPage.loadWikipediaUrl(document.langform.links.options[document.langform.links.selectedIndex].value);\"></form>") );
    return html;
}

void
WikipediaEnginePrivate::reloadWikipedia()
{
    Q_Q( WikipediaEngine );
    if( !wikiCurrentUrl.isValid() )
        return;
    urls << wikiCurrentUrl;
    q->setData( QLatin1String("wikipedia"), QLatin1String("busy"), true );
    q->scheduleSourcesUpdated();
    The::networkAccessManager()->getData( wikiCurrentUrl, q,
         SLOT(_wikiResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

WikipediaEnginePrivate::SelectionType
WikipediaEnginePrivate::selection() const
{
    return currentSelection;
}

bool
WikipediaEnginePrivate::setSelection( SelectionType type )
{
    if( currentSelection != type )
    {
        currentSelection = type;
        return true;
    }
    return false;
}

bool
WikipediaEnginePrivate::setSelection( const QString &type )
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

WikipediaEngine::WikipediaEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , d_ptr( new WikipediaEnginePrivate( this ) )
{

}

WikipediaEngine::~WikipediaEngine()
{
    delete d_ptr;
}

void
WikipediaEngine::init()
{
    Q_D( WikipediaEngine );
    d->dataContainer = new Plasma::DataContainer( this );
    d->dataContainer->setObjectName( QLatin1String("wikipedia") );
    addSource( d->dataContainer );
    connect( d->dataContainer, SIGNAL(dataUpdated(QString,Plasma::DataEngine::Data)),
             this, SLOT(_dataContainerUpdated(QString,Plasma::DataEngine::Data)) );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(_checkRequireUpdate(Meta::TrackPtr)) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(_checkRequireUpdate(Meta::TrackPtr)) );
    connect( engine, SIGNAL(stopped(qint64,qint64)),
             this, SLOT(_stopped()) );
}

bool
WikipediaEngine::sourceRequestEvent( const QString &source )
{
    if( source == QLatin1String("update") )
    {
        scheduleSourcesUpdated();
    }
    else if( source == QLatin1String("wikipedia") )
    {
        Q_D( WikipediaEngine );
        d->updateEngine();
        return true;
    }
    return false;
}

#include "WikipediaEngine.moc"
