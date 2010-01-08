/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *               2009 Mathias Panzenböck <grosser.meister.morti@gmx.net>                *
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

#include "PodcastReader.h"

#include "Debug.h"
#include "statusbar/StatusBar.h"
#include "MetaUtility.h"

#include <kio/job.h>
#include <kurl.h>
#include <KDateTime>

#include <QTextDocument>
#include <QDate>
#include <QSet>

using namespace Meta;

#define ITUNES_NS  "http://www.itunes.com/dtds/podcast-1.0.dtd"
#define RDF_NS     "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RSS10_NS   "http://purl.org/rss/1.0/"
#define RSS20_NS   ""
#define ATOM_NS    "http://www.w3.org/2005/Atom"
#define ENC_NS     "http://purl.oclc.org/net/rss_2.0/enc#"
#define CONTENT_NS "http://purl.org/rss/1.0/modules/content"
#define DC_NS      "http://purl.org/dc/elements/1.1/"

// regular expressions for linkification:
#define RE_USER   "[-+_%\\.\\w]+"
#define RE_PASSWD RE_USER
#define RE_DOMAIN "[-a-zA-Z0-9]+(?:\\.[-a-zA-Z0-9]+)*"
#define RE_PROT   "[a-zA-Z]+://"
#define RE_URL    RE_PROT "(?:" RE_USER "(?::" RE_PASSWD ")?@)?" RE_DOMAIN \
    "(?::\\d+)?(?:/[-\\w\\?&=%+.,;:_#~/!@]*)?"
#define RE_MAIL   RE_USER "@" RE_DOMAIN

const PodcastReader::StaticData PodcastReader::sd;

PodcastReader::PodcastReader( PodcastProvider *podcastProvider )
        : QXmlStreamReader()
        , m_podcastProvider( podcastProvider )
        , m_transferJob( 0 )
        , m_channel( 0 )
        , m_actionStack()
        , m_contentType( TextContent )
        , m_buffer()
        , m_current( 0 )
{}

void
PodcastReader::Action::begin( PodcastReader *podcastReader ) const
{
    if( m_begin )
        (( *podcastReader ).*m_begin )();
}

void
PodcastReader::Action::end( PodcastReader *podcastReader ) const
{
    if( m_end )
        (( *podcastReader ).*m_end )();
}

void
PodcastReader::Action::characters( PodcastReader *podcastReader ) const
{
    if( m_characters )
        (( *podcastReader ).*m_characters )();
}

// initialization of the feed parser automata:
PodcastReader::StaticData::StaticData()
        : removeScripts( "<script[^<]*</script>|<script[^>]*>", Qt::CaseInsensitive )
        , mightBeHtml( "<\\?xml[^>]*\\?>|<br[^>]*>|<p[^>]*>|&lt;|&gt;|&amp;|&quot;|"
                       "<([-:\\w\\d]+)[^>]*>.*</\\1>|<hr[>]*>|&#\\d+;|&#x[a-fA-F\\d]+;", Qt::CaseInsensitive )
        , linkify( "\\b(" RE_URL ")|\\b(" RE_MAIL ")|(\n)" )

        , startAction( rootMap )

        , docAction(
            docMap,
            0,
            &PodcastReader::endDocument )
        , xmlAction(
            xmlMap,
            &PodcastReader::beginXml,
            &PodcastReader::endXml,
            &PodcastReader::readEscapedCharacters )
        , skipAction( skipMap )
        , noContentAction(
            noContentMap,
            &PodcastReader::beginNoElement,
            0,
            &PodcastReader::readNoCharacters )

        , rdfAction(
            rdfMap,
            &PodcastReader::beginRdf )
        , rssAction(
            rssMap,
            &PodcastReader::beginRss )
        , feedAction(
            feedMap,
            &PodcastReader::beginFeed )
        , htmlAction(
            skipMap,
            &PodcastReader::beginHtml )
        , unknownFeedTypeAction(
            skipMap,
            &PodcastReader::beginUnknownFeedType )

        // RSS 1.0+2.0
        , rss10ChannelAction(
            rss10ChannelMap,
            &PodcastReader::beginChannel )
        , rss20ChannelAction(
            rss20ChannelMap,
            &PodcastReader::beginChannel )

        , titleAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endTitle,
            &PodcastReader::readCharacters )
        , subtitleAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endSubtitle,
            &PodcastReader::readCharacters )
        , descriptionAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endDescription,
            &PodcastReader::readCharacters )
        , encodedAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endEncoded,
            &PodcastReader::readCharacters )
        , bodyAction(
            xmlMap,
            &PodcastReader::beginText,
            &PodcastReader::endBody,
            &PodcastReader::readEscapedCharacters )
        , linkAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endLink,
            &PodcastReader::readCharacters )
        , imageAction( imageMap,
                       &PodcastReader::beginImage )
        , itemAction(
            itemMap,
            &PodcastReader::beginItem,
            &PodcastReader::endItem )
        , urlAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endImageUrl,
            &PodcastReader::readCharacters )
        , authorAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endAuthor,
            &PodcastReader::readCharacters )
        , creatorAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endCreator,
            &PodcastReader::readCharacters )
        , enclosureAction(
            noContentMap,
            &PodcastReader::beginEnclosure )
        , guidAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endGuid,
            &PodcastReader::readCharacters )
        , pubDateAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endPubDate,
            &PodcastReader::readCharacters )
        , keywordsAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endKeywords,
            &PodcastReader::readCharacters )
        , newFeedUrlAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endNewFeedUrl,
            &PodcastReader::readCharacters )

        // Atom
        , atomLogoAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endImageUrl,
            &PodcastReader::readCharacters )
        , atomIconAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endAtomIcon,
            &PodcastReader::readCharacters )
        , atomEntryAction(
            atomEntryMap,
            &PodcastReader::beginItem,
            &PodcastReader::endItem )
        , atomTitleAction(
            atomTextMap,
            &PodcastReader::beginAtomText,
            &PodcastReader::endAtomTitle,
            &PodcastReader::readAtomTextCharacters )
        , atomSubtitleAction(
            atomTextMap,
            &PodcastReader::beginAtomText,
            &PodcastReader::endAtomSubtitle,
            &PodcastReader::readAtomTextCharacters )
        , atomAuthorAction(
            atomAuthorMap )
        , atomFeedLinkAction(
            noContentMap,
            &PodcastReader::beginAtomFeedLink,
            0,
            &PodcastReader::readNoCharacters )
        , atomEntryLinkAction(
            noContentMap,
            &PodcastReader::beginAtomEntryLink,
            0,
            &PodcastReader::readNoCharacters )
        , atomIdAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endGuid,
            &PodcastReader::readCharacters )
        , atomPublishedAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endAtomPublished,
            &PodcastReader::readCharacters )
        , atomUpdatedAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endAtomUpdated,
            &PodcastReader::readCharacters )
        , atomSummaryAction(
            atomTextMap,
            &PodcastReader::beginAtomText,
            &PodcastReader::endAtomSummary,
            &PodcastReader::readAtomTextCharacters )
        , atomContentAction(
            atomTextMap,
            &PodcastReader::beginAtomText,
            &PodcastReader::endAtomContent,
            &PodcastReader::readAtomTextCharacters )
        , atomTextAction(
            atomTextMap,
            &PodcastReader::beginAtomTextChild,
            &PodcastReader::endAtomTextChild,
            &PodcastReader::readAtomTextCharacters )
{
    // known elements:
    knownElements[ "rss"          ] = Rss;
    knownElements[ "RDF"          ] = Rdf;
    knownElements[ "feed"         ] = Feed;
    knownElements[ "channel"      ] = Channel;
    knownElements[ "item"         ] = Item;
    knownElements[ "image"        ] = Image;
    knownElements[ "link"         ] = Link;
    knownElements[ "url"          ] = Url;
    knownElements[ "title"        ] = Title;
    knownElements[ "author"       ] = Author;
    knownElements[ "enclosure"    ] = EnclosureElement;
    knownElements[ "guid"         ] = Guid;
    knownElements[ "pubDate"      ] = PubDate;
    knownElements[ "description"  ] = Description;
    knownElements[ "summary"      ] = Summary;
    knownElements[ "body"         ] = Body;
    knownElements[ "entry"        ] = Entry;
    knownElements[ "content"      ] = Content;
    knownElements[ "name"         ] = Name;
    knownElements[ "id"           ] = Id;
    knownElements[ "subtitle"     ] = Subtitle;
    knownElements[ "updated"      ] = Updated;
    knownElements[ "published"    ] = Published;
    knownElements[ "logo"         ] = Logo;
    knownElements[ "icon"         ] = Icon;
    knownElements[ "encoded"      ] = Encoded;
    knownElements[ "creator"      ] = Creator;
    knownElements[ "keywords"     ] = Keywords;
    knownElements[ "new-feed-url" ] = NewFeedUrl;
    knownElements[ "html"         ] = Html;
    knownElements[ "HTML"         ] = Html;

    // before start document/after end document
    rootMap.insert( Document, &docAction );

    // parse document
    docMap.insert( Rss, &rssAction );
    docMap.insert( Html, &htmlAction );
    docMap.insert( Rdf, &rdfAction );
    docMap.insert( Feed, &feedAction );
    docMap.insert( Any, &unknownFeedTypeAction );

    // parse <rss> "RSS 2.0"
    rssMap.insert( Channel, &rss20ChannelAction );

    // parse <RDF> "RSS 1.0"
    rdfMap.insert( Channel, &rss10ChannelAction );
    rdfMap.insert( Item, &itemAction );

    // parse <channel> "RSS 2.0"
    rss20ChannelMap.insert( Title, &titleAction );
    rss20ChannelMap.insert( ItunesSubtitle, &subtitleAction );
    rss20ChannelMap.insert( ItunesAuthor, &authorAction );
    rss20ChannelMap.insert( Creator, &creatorAction );
    rss20ChannelMap.insert( Description, &descriptionAction );
    rss20ChannelMap.insert( Encoded, &encodedAction );
    rss20ChannelMap.insert( ItunesSummary, &descriptionAction );
    rss20ChannelMap.insert( Body, &bodyAction );
    rss20ChannelMap.insert( Link, &linkAction );
    rss20ChannelMap.insert( Image, &imageAction );
    rss20ChannelMap.insert( ItunesKeywords, &keywordsAction );
    rss20ChannelMap.insert( NewFeedUrl, &newFeedUrlAction );
    rss20ChannelMap.insert( Item, &itemAction );

    // parse <channel> "RSS 1.0"
    rss10ChannelMap.insert( Title, &titleAction );
    rss10ChannelMap.insert( ItunesSubtitle, &subtitleAction );
    rss10ChannelMap.insert( ItunesAuthor, &authorAction );
    rss10ChannelMap.insert( Creator, &creatorAction );
    rss10ChannelMap.insert( Description, &descriptionAction );
    rss10ChannelMap.insert( Encoded, &encodedAction );
    rss10ChannelMap.insert( ItunesSummary, &descriptionAction );
    rss10ChannelMap.insert( Body, &bodyAction );
    rss10ChannelMap.insert( Link, &linkAction );
    rss10ChannelMap.insert( Image, &imageAction );
    rss10ChannelMap.insert( ItunesKeywords, &keywordsAction );
    rss10ChannelMap.insert( NewFeedUrl, &newFeedUrlAction );

    // parse <image>
    imageMap.insert( Title, &skipAction );
    imageMap.insert( Link, &skipAction );
    imageMap.insert( Url, &urlAction );

    // parse <item>
    itemMap.insert( Title, &titleAction );
    itemMap.insert( ItunesSubtitle, &subtitleAction );
    itemMap.insert( Author, &authorAction );
    itemMap.insert( ItunesAuthor, &authorAction );
    itemMap.insert( Creator, &creatorAction );
    itemMap.insert( Description, &descriptionAction );
    itemMap.insert( Encoded, &encodedAction );
    itemMap.insert( ItunesSummary, &descriptionAction );
    itemMap.insert( Body, &bodyAction );
    itemMap.insert( EnclosureElement, &enclosureAction );
    itemMap.insert( Guid, &guidAction );
    itemMap.insert( PubDate, &pubDateAction );
    itemMap.insert( ItunesKeywords, &keywordsAction );
    // TODO: move the link field from PodcastChannel to PodcastMetaCommon
    // itemMap.insert( Link, &linkAction );

    // parse <feed> "Atom"
    feedMap.insert( Title, &atomTitleAction );
    feedMap.insert( Subtitle, &atomSubtitleAction );
    feedMap.insert( Icon, &atomIconAction );
    feedMap.insert( Logo, &atomLogoAction );
    feedMap.insert( Author, &atomAuthorAction );
    feedMap.insert( Link, &atomFeedLinkAction );
    feedMap.insert( Entry, &atomEntryAction );

    // parse <entry> "Atom"
    atomEntryMap.insert( Title, &atomTitleAction );
    atomEntryMap.insert( Subtitle, &atomSubtitleAction );
    atomEntryMap.insert( Author, &atomAuthorAction );
    atomEntryMap.insert( Id, &atomIdAction );
    atomEntryMap.insert( Published, &atomPublishedAction );
    atomEntryMap.insert( Updated, &atomUpdatedAction );
    atomEntryMap.insert( Summary, &atomSummaryAction );
    atomEntryMap.insert( Link, &atomEntryLinkAction );
    atomEntryMap.insert( SupportedContent, &atomContentAction );

    // parse <author> "Atom"
    atomAuthorMap.insert( Name, &authorAction );

    // parse atom text
    atomTextMap.insert( Any, &atomTextAction );

    // parse arbitrary xml
    xmlMap.insert( Any, &xmlAction );

    // skip elements
    skipMap.insert( Any, &skipAction );
}

PodcastReader::~PodcastReader()
{
    DEBUG_BLOCK
}

bool
PodcastReader::mightBeHtml( const QString& text ) //Static
{
    return sd.mightBeHtml.indexIn( text ) != -1;
}

bool PodcastReader::read( QIODevice *device )
{
    DEBUG_BLOCK

    setDevice( device );
    return read();
}

bool
PodcastReader::read( const KUrl &url )
{
    DEBUG_BLOCK

    m_url = url;

    m_transferJob = KIO::get( m_url, KIO::Reload, KIO::HideProgressInfo );

    connect( m_transferJob, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             SLOT( slotAddData( KIO::Job *, const QByteArray & ) ) );

    connect( m_transferJob, SIGNAL( result( KJob * ) ),
             SLOT( downloadResult( KJob * ) ) );

    connect( m_transferJob, SIGNAL( redirection( KIO::Job *, const KUrl & ) ),
             SLOT( slotRedirection( KIO::Job *, const KUrl & ) ) );

    connect( m_transferJob, SIGNAL( permanentRedirection( KIO::Job *,
                                    const KUrl &, const KUrl & ) ),
             SLOT( slotPermanentRedirection( KIO::Job *, const KUrl &,
                                             const KUrl & ) ) );

    QString description = i18n( "Importing podcast channel from %1", url.url() );
    if( m_channel )
    {
        description = m_channel->title().isEmpty()
                      ? i18n( "Updating podcast channel" )
                      : i18n( "Updating \"%1\"", m_channel->title() );
    }

    The::statusBar()->newProgressOperation( m_transferJob, description )
        ->setAbortSlot( this, SLOT( slotAbort() ) );

    // parse data
    return read();
}

void
PodcastReader::slotAbort()
{
    DEBUG_BLOCK
}

bool
PodcastReader::update( PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    m_channel = channel;

    return read( m_channel->url() );
}

void
PodcastReader::slotAddData( KIO::Job *job, const QByteArray &data )
{
    DEBUG_BLOCK
    Q_UNUSED( job )

    QXmlStreamReader::addData( data );

    // parse more data
    continueRead();
}

void
PodcastReader::downloadResult( KJob * job )
{
    DEBUG_BLOCK

    // parse more data
    continueRead();

    KIO::TransferJob *transferJob = dynamic_cast<KIO::TransferJob *>( job );
    if( transferJob && transferJob->isErrorPage() )
    {
        QString errorMessage =
            i18n( "Importing podcast from %1 failed with error:\n", m_url.url() );
        if( m_channel )
        {
            errorMessage = m_channel->title().isEmpty()
                           ? i18n( "Updating podcast from %1 failed with error:\n", m_url.url() )
                           : i18n( "Updating \"%1\" failed with error:\n", m_channel->title() );
        }
        errorMessage = errorMessage.append( job->errorString() );

        The::statusBar()->longMessage( errorMessage, StatusBar::Sorry );
    }
    else if( job->error() )
    {
        QString errorMessage =
            i18n( "Importing podcast from %1 failed with error:\n", m_url.url() );
        if( m_channel )
        {
            errorMessage = m_channel->title().isEmpty()
                           ? i18n( "Updating podcast from %1 failed with error:\n", m_url.url() )
                           : i18n( "Updating \"%1\" failed with error:\n", m_channel->title() );
        }
        errorMessage = errorMessage.append( job->errorString() );

        The::statusBar()->longMessage( errorMessage, StatusBar::Sorry );
    }

    m_transferJob = 0;
}

PodcastReader::ElementType
PodcastReader::elementType() const
{
    if( isEndDocument() || isStartDocument() )
        return Document;

    if( isCDATA() || isCharacters() )
        return CharacterData;

    ElementType elementType = sd.knownElements[ QXmlStreamReader::name().toString()];

    // This is a bit hacky because my automata does not support conditions.
    // Therefore I put the decision logic in here and declare some pseudo elements.
    // I don't think it is worth it to extend the automata to support such conditions.
    switch( elementType )
    {
        case Summary:
            if( namespaceUri() == ITUNES_NS )
            {
                elementType = ItunesSummary;
            }
            break;

        case Subtitle:
            if( namespaceUri() == ITUNES_NS )
            {
                elementType = ItunesSubtitle;
            }
            break;

        case Author:
            if( namespaceUri() == ITUNES_NS )
            {
                elementType = ItunesAuthor;
            }
            break;

        case Keywords:
            if( namespaceUri() == ITUNES_NS )
            {
                elementType = ItunesKeywords;
            }
            break;

        case Content:
            if( namespaceUri() == ATOM_NS &&
                    // ignore atom:content elements that do not
                    // have content but only refer to some url:
                    !hasAttribute( ATOM_NS, "src" ) )
            {
                // Atom supports arbitrary Base64 encoded content.
                // Because we can only something with text/html/xhtml I ignore
                // anything else.
                // See:
                //    http://tools.ietf.org/html/rfc4287#section-4.1.3
                if( hasAttribute( ATOM_NS, "type" ) )
                {
                    QStringRef type( attribute( ATOM_NS, "type" ) );

                    if( type == "text" || type == "html" || type == "xhtml" )
                    {
                        elementType = SupportedContent;
                    }
                }
                else
                {
                    elementType = SupportedContent;
                }
            }
            break;

        default:
            break;
    }

    return elementType;
}

bool
PodcastReader::read()
{
    DEBUG_BLOCK

    m_current = 0;
    m_item    = 0;
    m_contentType = TextContent;
    m_buffer.clear();
    m_actionStack.clear();
    m_actionStack.push( &( PodcastReader::sd.startAction ) );
    setNamespaceProcessing( true );

    return continueRead();
}

bool
PodcastReader::continueRead()
{
    // this is some kind of pushdown automata
    // with this it should be possible to parse feeds in parallel
    // woithout using threads
    DEBUG_BLOCK

    while( !atEnd() && error() != CustomError )
    {
        TokenType token = readNext();

        if( error() == PrematureEndOfDocumentError && m_transferJob )
        {
            return true;
        }

        if( hasError() )
        {
            emit finished( this );
            return false;
        }

        if( m_actionStack.isEmpty() )
        {
            debug() << "expected element on stack!";
            return false;
        }

        const Action* action = m_actionStack.top();
        const Action* subAction = 0;

        switch( token )
        {
            case Invalid:
                return false;

            case StartDocument:
            case StartElement:
                subAction = action->actionMap()[ elementType()];

                if( !subAction )
                    subAction = action->actionMap()[ Any ];

                if( !subAction )
                    subAction = &( PodcastReader::sd.skipAction );

                m_actionStack.push( subAction );

                subAction->begin( this );
                break;

            case EndDocument:
            case EndElement:
                action->end( this );

                if( m_actionStack.pop() != action )
                {
                    debug() << "popped other element than expected!";
                }
                break;

            case Characters:
                if( !isWhitespace() || isCDATA() )
                {
                    action->characters( this );
                }

                // ignoreable whitespaces
            case Comment:
            case EntityReference:
            case ProcessingInstruction:
            case DTD:
            case NoToken:
                // ignore
                break;
        }
    }

    return !hasError();
}

void
PodcastReader::stopWithError( const QString &message )
{
    raiseError( message );

    if( m_transferJob )
    {
        m_transferJob->kill(KJob::EmitResult);
        m_transferJob = 0;
    }

    emit finished( this );
}

void
PodcastReader::beginText()
{
    m_buffer.clear();
}

void
PodcastReader::endTitle()
{
    m_current->setTitle( m_buffer.trimmed() );
}

void
PodcastReader::endSubtitle()
{
    m_current->setSubtitle( m_buffer.trimmed() );
}

QString
PodcastReader::atomTextAsText()
{
    switch( m_contentType )
    {
        case HtmlContent:
        case XHtmlContent:
            // TODO: strip tags (there should not be any non-xml entities here)
            return unescape( m_buffer );

        case TextContent:
        default:
            return m_buffer;
    }
}

QString
PodcastReader::atomTextAsHtml()
{
    switch( m_contentType )
    {
        case HtmlContent:
        case XHtmlContent:
            // strip <script> elements
            // This will work because there aren't <![CDATA[ ]]> sections
            // in m_buffer, because we have (re)escape the code manually.
            // But it does not remove event handlers lice onclick="..."
            // and JavaScript links like href="javascript:..."
            return m_buffer.replace( sd.removeScripts, "" );

        case TextContent:
        default:
            return textToHtml( m_buffer );
    }
}

QString
PodcastReader::unescape( const QString &text )
{
    // TODO: resolve predefined html entities
    QString buf;

    for ( int i = 0; i < text.size(); ++ i )
    {
        QChar c( text[ i ] );

        if( c == '&' )
        {
            int endIndex = text.indexOf( ';', i );

            if( endIndex == -1 )
            {
                // fix invalid input
                buf += c;
            }
            else if( text[ i + 1 ] == '#' )
            {
                int num = 0;
                bool ok = false;
                if( text[ i + 2 ] == 'x' )
                {
                    QString entity( text.mid( i + 3, endIndex - i - 3 ) );
                    num = entity.toInt( &ok, 16 );
                }
                else
                {
                    QString entity( text.mid( i + 2, endIndex - i - 2 ) );
                    num = entity.toInt( &ok, 10 );
                }

                if( !ok || num < 0 )
                {
                    // fix invalid input
                    buf += c;
                }
                else
                {
                    buf += QChar( num );
                    i = endIndex;
                }
            }
            else
            {
                QString entity( text.mid( i + 1, endIndex - i - 1 ) );

                if( entity == "lt" )
                {
                    buf += '<';
                    i = endIndex;
                }
                else if( entity == "gt" )
                {
                    buf += '>';
                    i = endIndex;
                }
                else if( entity == "amp" )
                {
                    buf += '&';
                    i = endIndex;
                }
                else if( entity == "apos" )
                {
                    buf += '\'';
                    i = endIndex;
                }
                else if( entity == "quot" )
                {
                    buf += '"';
                    i = endIndex;
                }
                else
                {
                    // fix invalid input
                    buf += c;
                }
            }
        }
        else
        {
            buf += c;
        }
    }

    return buf;
}

void
PodcastReader::setSummary( const QString &description )
{
    if( m_current->summary().size() < description.size() )
    {
        m_current->setSummary( description );
    }
}

void
PodcastReader::setDescription( const QString &description )
{
    // The content of the <description>, <itunes:summary> or <body>
    // elements might be assigned to the field description, unless
    // there is already longer data in it. Then it will be assigned
    // to summary, unless summary depending on whether there
    // already is some (longer) information in the description
    // field.
    // If there is already data in the description field, instead of
    // overwriting, it will be moved to the summary field, unless
    // there is already longer data there.
    if( m_current->description().size() < description.size() )
    {
        setSummary( m_current->description() );
        m_current->setDescription( description );
    }
    else
    {
        setSummary( description );
    }
}

void
PodcastReader::endDescription()
{
    QString description( m_buffer.trimmed() );

    if( !mightBeHtml( description ) )
    {
        // content type is plain text
        description = textToHtml( description );
    }
    // else: content type is html
    setDescription( description );
}

QString
PodcastReader::textToHtml( const QString &text )
{
    QString buf;
    QRegExp re( sd.linkify );
    int index = 0;

    for(;;)
    {
        int next = re.indexIn( text, index );

        if( next == -1 )
            break;

        if( next != index )
        {
            buf += Qt::escape( text.mid( index, next - index ) );
        }

        QString s;

        if( !(s = re.cap( 1 )).isEmpty() )
        {
            if( s.startsWith( QLatin1String( "javascript:" ), Qt::CaseInsensitive ) ||
                s.startsWith( QLatin1String( "exec:" ), Qt::CaseInsensitive ) )
            {
                buf += Qt::escape( s );
            }
            else
            {
                buf += QString( "<a href=\"%1\">%1</a>" )
                    .arg( Qt::escape( s ) );
            }
        }
        else if( !(s = re.cap( 2 )).isEmpty() )
        {
            buf += QString( "<a href=\"mailto:%1\">%1</a>" )
                .arg( Qt::escape( s ) );
        }
        else if( !re.cap( 3 ).isEmpty() )
        {
            buf += "<br/>\n";
        }

        index = re.pos() + re.matchedLength();
    }

    buf += Qt::escape( text.mid( index ) );

    return buf;
}

void
PodcastReader::endEncoded()
{
    // content type is html
    setDescription( m_buffer.trimmed() );
}

void
PodcastReader::endBody()
{
    // content type is xhtml
    // always prefer <body>, because it's likely to
    // contain nice html formatted infos
    setSummary( m_current->description() );
    m_current->setDescription( m_buffer.trimmed() );
}

void
PodcastReader::endLink()
{
    // TODO: change to m_current->... when the field
    //       is moved to the PodcastMetaCommon class.
    m_channel->setWebLink( KUrl( m_buffer ) );
}

void
PodcastReader::beginHtml()
{
    stopWithError( i18n( "While parsing %1, a feed was expected but an HTML page was received."
                         "\nDid you enter the correct URL?" ).arg( m_url.url() ) );
}

void
PodcastReader::beginUnknownFeedType()
{
    stopWithError( i18n( "Feed has an unknown type: %1", m_url.url() ) );
}

void
PodcastReader::beginRss()
{
    if( attributes().value( "version" ) != "2.0" )
    {
        // TODO: change this string once we support more
        stopWithError( i18n( "%1 is not an RSS version 2.0 feed.", m_url.url() ) );
    }
}

void
PodcastReader::beginRdf()
{
    bool ok = true;
    if( namespaceUri() != RDF_NS )
    {
        ok = false;
    }

    if( ok )
    {
        bool found = false;
        foreach( const QXmlStreamNamespaceDeclaration &nsdecl, namespaceDeclarations() )
        {
            if( nsdecl.namespaceUri() == RSS10_NS )
            {
                found = true;
                break;
            }
        }

        if( !found )
            ok = false;
    }

    if( !ok )
        stopWithError( i18n( "%1 is not a valid RSS version 1.0 feed.", m_url.url() ) );
}

void
PodcastReader::beginFeed()
{
    if( namespaceUri() != ATOM_NS )
    {
        stopWithError( i18n( "%1 is not a valid Atom feed.", m_url.url() ) );
    }
    else
    {
        beginChannel();
    }
}

void
PodcastReader::endDocument()
{
    debug() << "successfuly parsed feed: " << m_url.url();
    emit finished( this );
}

void
PodcastReader::createChannel()
{
    if( !m_channel )
    {
        debug() << "new channel";

        Meta::PodcastChannelPtr channel( new Meta::PodcastChannel() );
        channel->setUrl( m_url );
        channel->setSubscribeDate( QDate::currentDate() );
        /* add this new channel to the provider, we get a pointer to a
         * PodcastChannelPtr of the correct type which we will use from now on.
         */
        m_channel = m_podcastProvider->addChannel( channel );
    }
}

void
PodcastReader::beginChannel()
{
    createChannel();

    m_current = m_channel.data();

    // Because the summary and description fields are read from several elements
    // they only get changed when longer information is read as there is stored in
    // the apropriate field already. In order to still be able to correctly update
    // the feed's description/summary I set it here to the empty string:
    m_channel->setDescription( "" );
    m_channel->setSummary( "" );
    m_channel->setKeywords( QStringList() );
}

void
PodcastReader::beginItem()
{
    // theoretically it is possible that an ugly RSS 1.0 feed has
    // first the <item> elements followed by the <channel> element:
    createChannel();

    m_item = new Meta::PodcastEpisode( m_channel );
    m_current = m_item.data();

    m_enclosures.clear();
}

void
PodcastReader::endItem()
{
    // TODO: change superclass of PodcastEpisode to MultiTrack

    /*  some feeds contain normal blogposts without
        enclosures alongside of podcasts */

    if( !m_enclosures.isEmpty() )
    {
        // just take the first enclosure on multi
        m_item->setUidUrl( m_enclosures[ 0 ].url() );
        m_item->setFilesize( m_enclosures[ 0 ].fileSize() );
        m_item->setMimeType( m_enclosures[ 0 ].mimeType() );

        m_enclosures.removeAt( 0 );

        // append alternative enclosures to description
        if( !m_enclosures.isEmpty() )
        {
            QString description( m_item->description() );
            description += "\n<p><b>";
            description += i18n( "Alternative Enclosures:" );
            description += "</b><br/>\n<ul>";

            foreach( const Enclosure& enclosure, m_enclosures )
            {
                description += QString( "<li><a href=\"%1\">%2</a> (%3, %4)</li>" )
                               .arg( Qt::escape( enclosure.url().url() ) )
                               .arg( Qt::escape( enclosure.url().fileName() ) )
                               .arg( Meta::prettyFilesize( enclosure.fileSize() ) )
                               .arg( enclosure.mimeType().isEmpty() ?
                                     i18n( "unknown type" ) :
                                     Qt::escape( enclosure.mimeType() ) );
            }

            description += "</ul></p>";
            m_item->setDescription( description );
        }

        const KUrl trackId( m_item->guid().isEmpty() ? m_item->uidUrl() : m_item->guid() );
        Meta::PodcastEpisodePtr episode = Meta::PodcastEpisodePtr::dynamicCast(
                                              m_podcastProvider->trackForUrl( trackId )
                                          );

        if( !episode.isNull() )
        {
            debug() << "updating episode: " << m_item->title();

            episode->setTitle( m_item->title() );
            episode->setSubtitle( m_item->subtitle() );
            episode->setSummary( m_item->summary() );
            episode->setDescription( m_item->description() );
            episode->setAuthor( m_item->author() );
            episode->setUidUrl( m_item->uidUrl() );
            episode->setFilesize( m_item->filesize() );
            episode->setMimeType( m_item->mimeType() );
            episode->setPubDate( m_item->pubDate() );
            episode->setKeywords( m_item->keywords() );

            // set the guid in case it was empty (for some buggy reason):
            episode->setGuid( m_item->guid() );
        }
        else
        {
            debug() << "new episode: " << m_item->title();

            episode = m_channel->addEpisode( m_item );
            // also let the provider know an episode has been added
            // TODO: change into a signal
            m_podcastProvider->addEpisode( episode );
        }
    }

    m_current = m_channel.data();
    m_item = 0;
}

void
PodcastReader::beginEnclosure()
{
    // This should read both, RSS 2.0 and RSS 1.0 with mod_enclosure
    // <enclosure> elements.
    // See:
    //    http://www.rssboard.org/rss-specification
    //    http://www.xs4all.nl/~foz/mod_enclosure.html
    QStringRef str;

    str = attributes().value( "url" );

    if( str.isEmpty() )
        str = attribute( RDF_NS, "about" );

    if( str.isEmpty() )
    {
        debug() << "invalid enclosure containing no/empty url";
        return;
    }

    KUrl url( str.toString() );

    str = attributes().value( "length" );

    if( str.isEmpty() )
        str = attribute( ENC_NS, "length" );

    int length = str.toString().toInt();

    str = attributes().value( "type" );

    if( str.isEmpty() )
        str = attribute( ENC_NS, "type" );

    QString mimeType( str.toString().trimmed() );

    m_enclosures.append( Enclosure( url, length, mimeType ) );
}

void
PodcastReader::endGuid()
{
    m_item->setGuid( m_buffer );
}

void
PodcastReader::endPubDate()
{
    QDateTime pubDate( parsePubDate( m_buffer ) );

    if( !pubDate.isValid() )
    {
        debug() << "invalid podcast episode pubDate: " << m_buffer;
        return;
    }

    m_item->setPubDate( pubDate );
}

void
PodcastReader::beginImage()
{
    if( namespaceUri() == ITUNES_NS )
    {
        m_channel->setImageUrl( KUrl( attributes().value( "href" ).toString() ) );
    }
}

void
PodcastReader::endImageUrl()
{
    // TODO save image data
    m_channel->setImageUrl( KUrl( m_buffer ) );
}

void
PodcastReader::endKeywords()
{
    QList<QString> keywords( m_current->keywords() );

    foreach( const QString &keyword, m_buffer.split( ',' ) )
    {
        QString kwd( keyword.simplified() );
        if( !kwd.isEmpty() && !keywords.contains( kwd ) )
            keywords.append( kwd );
    }

    qSort( keywords );
    m_current->setKeywords( keywords );

}

void
PodcastReader::endNewFeedUrl()
{
    if( namespaceUri() == ITUNES_NS )
    {
        m_url = KUrl( m_buffer.trimmed() );

        if( m_channel && m_channel->url() != m_url )
        {
            debug() << "feed url changed to: " << m_url.url();
            m_channel->setUrl( m_url );
        }
    }
}

void
PodcastReader::endAuthor()
{
    m_current->setAuthor( m_buffer.trimmed() );
}

void
PodcastReader::endCreator()
{
    // there are funny people that do not use <author> but <dc:creator>
    if( namespaceUri() == DC_NS )
    {
        endAuthor();
    }
}

void
PodcastReader::beginXml()
{
    m_buffer += '<';
    m_buffer += QXmlStreamReader::name().toString();

    foreach( const QXmlStreamAttribute &attr, attributes() )
    {
        m_buffer += QString( " %1=\"%2\"" )
                    .arg( attr.name().toString() )
                    .arg( Qt::escape( attr.value().toString() ) );
    }

    m_buffer += '>';
}

void
PodcastReader::beginNoElement()
{
    DEBUG_BLOCK
    debug() << "no element expected here, but got element: "
    << QXmlStreamReader::name();
}

void
PodcastReader::beginAtomText()
{
    if( hasAttribute( ATOM_NS, "type" ) )
    {
        QStringRef type( attribute( ATOM_NS, "type" ) );

        if( type == "text" )
        {
            m_contentType = TextContent;
        }
        else if( type == "html" )
        {
            m_contentType = HtmlContent;
        }
        else if( type == "xhtml" )
        {
            m_contentType = XHtmlContent;
        }
        else
        {
            // this should not happen, see elementType()
            debug() << "unsupported atom:content type: " << type.toString();
            m_contentType = TextContent;
        }
    }
    else
    {
        m_contentType = TextContent;
    }

    m_buffer.clear();
}

void
PodcastReader::beginAtomTextChild()
{
    switch( m_contentType )
    {
        case XHtmlContent:
            beginXml();
            break;

        case HtmlContent:
        case TextContent:
            // stripping illegal tags
            debug() << "read unexpected open tag in atom text: " << QXmlStreamReader::name();

        default:
            break;
    }
}

void
PodcastReader::endAtomTextChild()
{
    switch( m_contentType )
    {
        case XHtmlContent:
            endXml();
            break;

        case HtmlContent:
        case TextContent:
            // stripping illegal tags
            debug() << "read unexpected close tag in atom text: " << QXmlStreamReader::name();

        default:
            break;
    }
}

void
PodcastReader::readAtomTextCharacters()
{
    switch( m_contentType )
    {
    case XHtmlContent:
        m_buffer += Qt::escape( text().toString() );
        break;

    case HtmlContent:
        m_buffer += text();
        break;

    case TextContent:
        m_buffer += text();

    default:
        break;
    }
}

void
PodcastReader::beginAtomFeedLink()
{
    if( !hasAttribute( ATOM_NS, "rel" ) ||
            attribute( ATOM_NS, "rel" ) == "alternate" )
    {
        m_channel->setWebLink( KUrl( attribute( ATOM_NS, "href" ).toString() ) );
    }
    else if( attribute( ATOM_NS, "rel" ) == "self" )
    {
        m_url = KUrl( attribute( ATOM_NS, "href" ).toString() );

        if( m_channel && m_channel->url() != m_url )
        {
            debug() << "feed url changed to: " << m_url.url();
            m_channel->setUrl( m_url );
        }
    }
}

void
PodcastReader::beginAtomEntryLink()
{
    if( attribute( ATOM_NS, "rel" ) == "enclosure" )
    {
        KUrl url( attribute( ATOM_NS, "href" ).toString() );
        int filesize = 0;
        QString mimeType;

        if( hasAttribute( ATOM_NS, "length" ) )
        {
            filesize = attribute( ATOM_NS, "length" ).toString().toInt();
        }

        if( hasAttribute( ATOM_NS, "type" ) )
        {
            mimeType = attribute( ATOM_NS, "type" ).toString();
        }

        m_enclosures.append( Enclosure( url, filesize, mimeType ) );
    }
}

void
PodcastReader::endAtomIcon()
{
    if( !m_channel->hasImage() )
    {
        endImageUrl();
    }
}

void
PodcastReader::endAtomTitle()
{
    // TODO: don't convert text but store m_contentType
    m_current->setTitle( atomTextAsText().trimmed() );
}

void
PodcastReader::endAtomSubtitle()
{
    // TODO: don't convert text but store m_contentType
    m_current->setSubtitle( atomTextAsText().trimmed() );
}

void
PodcastReader::endAtomSummary()
{
    // TODO: don't convert text but store m_contentType
    m_current->setSummary( atomTextAsHtml().trimmed() );
}

void
PodcastReader::endAtomContent()
{
    // TODO: don't convert text but store m_contentType
    m_current->setDescription( atomTextAsHtml() );
}

void
PodcastReader::endAtomPublished()
{
    QDateTime date( KDateTime::fromString( m_buffer, KDateTime::ISODate ).dateTime() );

    if( !date.isValid() )
    {
        debug() << "invalid podcast episode atom:published date: " << m_buffer;
        return;
    }

    if( !m_item->pubDate().isValid() || m_item->pubDate() < date )
    {
        m_item->setPubDate( date );
    }
}

void
PodcastReader::endAtomUpdated()
{
    QDateTime date( KDateTime::fromString( m_buffer, KDateTime::ISODate ).dateTime() );

    if( !date.isValid() )
    {
        debug() << "invalid podcast episode atom:updated date: " << m_buffer;
        return;
    }

    if( !m_item->pubDate().isValid() || m_item->pubDate() < date )
    {
        // TODO: add field updatedDate and use this (throughout amarok)
        m_item->setPubDate( date );
    }
}

void
PodcastReader::readNoCharacters()
{
    DEBUG_BLOCK
    debug() << "no characters expected here";
}

void
PodcastReader::endXml()
{
    m_buffer += "</";
    m_buffer += QXmlStreamReader::name().toString();
    m_buffer += '>';
}

void
PodcastReader::readCharacters()
{
    m_buffer += text();
}

void
PodcastReader::readEscapedCharacters()
{
    m_buffer += Qt::escape( text().toString() );
}

QStringRef
PodcastReader::attribute( const char *namespaceUri, const char *name ) const
{
    // workaround, because Qt seems to have a bug:
    // when the default namespace is used attributes
    // aren't inside this namespace for some reason
    if( attributes().hasAttribute( namespaceUri, name ) )
        return attributes().value( namespaceUri, name );
    else
        return attributes().value( NULL, name );
}

bool
PodcastReader::hasAttribute( const char *namespaceUri, const char *name ) const
{
    // see PodcastReader::attribute()
    if( attributes().hasAttribute( namespaceUri, name ) )
        return true;
    else
        return attributes().hasAttribute( NULL, name );
}

QDateTime
PodcastReader::parsePubDate( const QString &dateString )
{
    DEBUG_BLOCK
    QString parseInput = dateString;
    debug() << "Parsing pubdate: " << parseInput;

    QRegExp rfcDateDayRegex("^[A-Z]{1}[a-z]{2}\\s*,\\s*(.*)");
    if ( rfcDateDayRegex.indexIn(parseInput) != -1 )
    {
        parseInput = rfcDateDayRegex.cap(1);
    }
    QDateTime pubDate = KDateTime::fromString( parseInput, KDateTime::RFCDate ).dateTime();

    debug() << "result: " << pubDate.toString();
    return pubDate;
}

void
PodcastReader::slotRedirection( KIO::Job * job, const KUrl & url )
{
    DEBUG_BLOCK
    Q_UNUSED( job );
    debug() << "redirected to: " << url.url();
}

void
PodcastReader::slotPermanentRedirection( KIO::Job * job, const KUrl & fromUrl,
        const KUrl & toUrl )
{
    DEBUG_BLOCK
    Q_UNUSED( job );
    Q_UNUSED( fromUrl );
    debug() << "permanently redirected to: " << toUrl.url();
    m_url = toUrl;
    /* change the url for existing feeds as well. Permanent redirection means the old one
    might dissapear soon. */
    if( m_channel )
        m_channel->setUrl( m_url );
}

Meta::PodcastEpisodePtr
PodcastReader::podcastEpisodeCheck( Meta::PodcastEpisodePtr episode )
{
//     DEBUG_BLOCK
    Meta::PodcastEpisodePtr episodeMatch = episode;
    Meta::PodcastEpisodeList episodes = m_channel->episodes();

//     debug() << "episode title: " << episode->title();
//     debug() << "episode url: " << episode->prettyUrl();
//     debug() << "episode guid: " << episode->guid();

    foreach( PodcastEpisodePtr match, episodes )
    {
//         debug() << "match title: " << match->title();
//         debug() << "match url: " << match->prettyUrl();
//         debug() << "match guid: " << match->guid();

        int score = 0;
        if( !episode->title().isEmpty() && episode->title() == match->title() )
            score += 1;
        if( !episode->prettyUrl().isEmpty() && episode->prettyUrl() == match->prettyUrl() )
            score += 3;
        if( !episode->guid().isEmpty() && episode->guid() == match->guid() )
            score += 3;

//         debug() << "score: " << score;
        if( score >= 3 )
        {
            episodeMatch = match;
            break;
        }
    }

    return episodeMatch;
}

#include "PodcastReader.moc"
