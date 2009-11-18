/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include <kio/job.h>
#include <kurl.h>
#include <KDateTime>

#include <QTextDocument>
#include <QDate>
#include <time.h>

using namespace Meta;

const PodcastReader::StaticData PodcastReader::sd;

PodcastReader::PodcastReader( PodcastProvider * podcastProvider )
        : QXmlStreamReader()
        , m_podcastProvider( podcastProvider )
        , m_transferJob( 0 )
        , m_channel( 0 )
        , m_actionStack()
        , m_descriptionType( NoDescription )
        , m_channelDescriptionType( NoDescription )
        , m_buffer()
        , m_current( 0 )
{}

void
PodcastReader::Action::begin(PodcastReader *podcastReader) const
{
    if( m_begin )
        ((*podcastReader).*m_begin)();
}

void
PodcastReader::Action::end(PodcastReader *podcastReader) const
{
    if( m_end )
        ((*podcastReader).*m_end)();
}

void
PodcastReader::Action::characters(PodcastReader *podcastReader) const
{
    if( m_characters )
        ((*podcastReader).*m_characters)();
}

// initialization of the feed parser automata:
PodcastReader::StaticData::StaticData()
        : startAction( rootMap )
        , titleAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endTitle,
            &PodcastReader::readCharacters )
        , descriptionAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endDescription,
            &PodcastReader::readCharacters )
        , summaryAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endItunesSummary,
            &PodcastReader::readCharacters )
        , bodyAction(
            xmlMap,
            &PodcastReader::beginText,
            &PodcastReader::endBody,
            &PodcastReader::readCharacters )
        , linkAction(
            textMap,
            &PodcastReader::beginText,
            &PodcastReader::endLink,
            &PodcastReader::readCharacters )
        , skipAction( skipMap )
        , docAction( docMap )
        , rssAction(
            rssMap,
            &PodcastReader::beginRss,
            &PodcastReader::endRss )
        , htmlAction(
            skipMap,
            &PodcastReader::beginHtml )
        , unknownFeedTypeAction(
            skipMap,
            &PodcastReader::beginUnknownFeedType )
        , channelAction(
            channelMap,
            &PodcastReader::beginChannel)
        , imageAction( imageMap )
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
        , enclosureAction(
            skipMap,
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
        , xmlAction(
            xmlMap,
            &PodcastReader::beginXml,
            &PodcastReader::endXml,
            &PodcastReader::readCharacters )
{

    // before start document/after end document
    rootMap.insert( Document, &docAction );

    // parse document
    docMap.insert( Rss, &rssAction );
    docMap.insert( Html, &htmlAction );
    docMap.insert( Any, &unknownFeedTypeAction );

    // parse <rss>
    rssMap.insert( Channel, &channelAction );

    // parse <channel>
    channelMap.insert( Title, &titleAction );
    channelMap.insert( Description, &descriptionAction );
    channelMap.insert( Summary, &summaryAction );
    channelMap.insert( Body, &bodyAction );
    channelMap.insert( Link, &linkAction );
    channelMap.insert( Image, &imageAction );
    channelMap.insert( Item, &itemAction );
    
    // parse <image>
    imageMap.insert( Title, &skipAction );
    imageMap.insert( Link, &skipAction );
    imageMap.insert( Url, &urlAction );

    // parse <item>
    itemMap.insert( Title, &titleAction );
    itemMap.insert( Author, &authorAction );
    itemMap.insert( Description, &descriptionAction );
    itemMap.insert( Summary, &summaryAction );
    itemMap.insert( Body, &bodyAction );
    itemMap.insert( Enclosure, &enclosureAction );
    itemMap.insert( Guid, &guidAction );
    itemMap.insert( PubDate, &pubDateAction );
    // TODO: move the link field from PodcastChannel to PodcastMetaCommon
    // itemMap.insert( Link, &linkAction );

    // parse arbitrary xml
    xmlMap.insert( Any, &xmlAction );

    // skip elements
    skipMap.insert( Any, &skipAction );
}

PodcastReader::~PodcastReader()
{
    DEBUG_BLOCK
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
             const KUrl &, const KUrl &) ),
             SLOT( slotPermanentRedirection( KIO::Job *, const KUrl &,
             const KUrl &) ) );

    QString description = i18n("Importing podcast channel from %1", url.url());
    if( m_channel )
    {
        description = m_channel->title().isEmpty()
            ? i18n("Updating podcast channel")
            : i18n("Updating \"%1\"", m_channel->title());
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
        return TextContent;

    const QStringRef name = QXmlStreamReader::name();

    if( name == "rss" )
    {
        return Rss;
    }
    else if( name == "channel" )
    {
        return Channel;
    }
    else if( name == "item" )
    {
        return Item;
    }
    else if( name == "image" )
    {
        return Image;
    }
    else if( name == "link" )
    {
        return Link;
    }
    else if( name == "url" )
    {
        return Url;
    }
    else if( name == "title" )
    {
        return Title;
    }
    else if( name == "author" )
    {
        return Author;
    }
    else if( name == "enclosure" )
    {
        return Enclosure;
    }
    else if( name == "guid" )
    {
        return Guid;
    }
    else if( name == "pubDate" )
    {
        return PubDate;
    }
    else if( name == "description" )
    {
        return Description;
    }
    else if( name == "summary" &&
        namespaceUri() == "http://www.itunes.com/dtds/podcast-1.0.dtd" )
    {
        return Summary;
    }
    else if( name == "body" )
    {
        return Body;
    }
    else if( name.toString().toLower() == "html" )
    {
        return Html;
    }
    else
    {
        return Unknown;
    }
}

bool
PodcastReader::read()
{
    DEBUG_BLOCK

    m_current = 0;
    m_item    = 0;
    m_descriptionType        = NoDescription;
    m_channelDescriptionType = NoDescription;
    m_buffer.clear();
    m_actionStack.clear();
    m_actionStack.push( &( PodcastReader::sd.startAction ) );

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
                subAction = action->actionMap()[ elementType() ];

                if( !subAction )
                    subAction = action->actionMap()[ Any ];

                if( !subAction )
                    subAction = PodcastReader::sd.skipMap[ Any ];
                
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
        m_transferJob->kill();
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
PodcastReader::endDescription()
{
    if( m_descriptionType <= RssDescription )
    {
        m_current->setDescription( m_buffer.trimmed() );
        m_descriptionType = RssDescription;
    }
}

void
PodcastReader::endItunesSummary()
{
    if( m_descriptionType <= ItunesSummary )
    {
        m_current->setDescription( m_buffer.trimmed() );
        m_descriptionType = ItunesSummary;
    }
}

void
PodcastReader::endBody()
{
    m_current->setDescription( m_buffer.trimmed() );
    m_descriptionType = HtmlBody;
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
    stopWithError( i18n( "A HTML page was received. Expected an RSS 2.0 feed" ) );
}

void
PodcastReader::beginUnknownFeedType()
{
    stopWithError( i18n( "Feed has unknown type: %1", m_url.url() ) );
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
PodcastReader::endRss()
{
    debug() << "successfuly parsed feed: " << m_url.url();
    emit finished( this );
}

void
PodcastReader::beginChannel()
{
    if( !m_channel )
    {
        debug() << "new channel";

        m_channel = new Meta::PodcastChannel();
           m_channel->setUrl( m_url );
        m_channel->setSubscribeDate( QDate::currentDate() );
        /* add this new channel to the provider, we get a pointer to a
         * PodcastChannelPtr of the correct type which we will use from now on.
         */
        m_channel = m_podcastProvider->addChannel( m_channel );
    }

    m_descriptionType = m_channelDescriptionType = NoDescription;
    m_current = m_channel.data();
}

void 
PodcastReader::beginItem()
{
    m_item = new Meta::PodcastEpisode( m_channel );
    m_current = m_item.data();
    m_channelDescriptionType = m_descriptionType;
    m_descriptionType = NoDescription;
}

void 
PodcastReader::endItem()
{
    if( !m_podcastProvider->possiblyContainsTrack( m_item->uidUrl() ) &&
        // some feeds contain normal blogposts without
        // enclosures alongside of podcasts:
        !m_item->uidUrl().isEmpty() )
    {
        debug() << "new episode: " << m_item->title();

        Meta::PodcastEpisodePtr episode = m_channel->addEpisode( m_item );
        // also let the provider know an episode has been added
        // TODO: change into a signal
        m_podcastProvider->addEpisode( episode );
    }

    m_descriptionType = m_channelDescriptionType;
    m_current = m_channel.data();
    m_item = 0;
}

void
PodcastReader::beginEnclosure()
{
    m_item->setUidUrl( KUrl( attributes().value( "url" ).toString() ) );
    m_item->setFilesize( attributes().value( "length" ).toString().toInt() );
    m_item->setMimeType( attributes().value( "type" ).toString().trimmed() );
}

void
PodcastReader::endGuid()
{
    m_item->setGuid( m_buffer );
}

void
PodcastReader::endPubDate()
{
    m_item->setPubDate( parsePubDate( m_buffer ) );
}

void
PodcastReader::endImageUrl()
{
    // TODO save image data
    m_channel->setImageUrl( KUrl( m_buffer ) );
}

void
PodcastReader::endAuthor()
{
    m_current->setAuthor( m_buffer.trimmed() );
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

const char*
PodcastReader::tokenToString(TokenType token)
{
    switch (token)
    {
        case NoToken: return "NoToken";
        case Invalid: return "Invalid";
        case StartDocument: return "StartDocument";
        case EndDocument: return "EndDocument";
        case StartElement: return "StartElement";
        case EndElement: return "EndElement";
        case Characters: return "Characters";
        case Comment: return "Comment";
        case DTD: return "DTD";
        case EntityReference: return "EntityReference";
        case ProcessingInstruction: return "ProcessingInstruction";
    }

    return "<Invalid-Enum-Value>";
}

QDateTime
PodcastReader::parsePubDate( const QString &dateString )
{
    DEBUG_BLOCK
    QDateTime pubDate;

    debug() << "Parsing pubdate: " << dateString;
    if( dateString.contains( ',' ) )
        pubDate = KDateTime::fromString( dateString, KDateTime::RFCDateDay ).dateTime();
    else
        pubDate = KDateTime::fromString( dateString, KDateTime::RFCDate ).dateTime();

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
    Q_UNUSED( job ); Q_UNUSED( fromUrl );
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
