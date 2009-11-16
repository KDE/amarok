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

PodcastReader::PodcastReader( PodcastProvider * podcastProvider )
        : QXmlStreamReader()
        , m_feedType( UnknownFeedType )
        , m_podcastProvider( podcastProvider )
        , m_transferJob( 0 )
{}

PodcastReader::~PodcastReader()
{
    DEBUG_BLOCK
}

bool PodcastReader::read ( QIODevice *device )
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

    KIO::TransferJob *m_transferJob = KIO::get( m_url, KIO::Reload, KIO::HideProgressInfo );

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

    return true;
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

    qxml::addData( data );
}

void
PodcastReader::downloadResult( KJob * job )
{
    DEBUG_BLOCK

    KIO::TransferJob *transferJob = dynamic_cast<KIO::TransferJob *>( job );
    if ( transferJob && transferJob->isErrorPage() )
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

    // parse data
    read();
}

QString
PodcastReader::readInnerXml()
{
	QString xml;
	int level = 1;

    while ( !atEnd() )
    {
		switch ( nextToken() ) {
		case Invalid:
			// this should not happen
			debug() << "*** Invalid *** " << errorString();
			return xml;

		case StartElement:
			++ level;
			xml += QString("<%1").arg(qxml::name().toString());
			
			foreach( const QXmlStreamAttribute& attr, attributes() ) {
				xml += QString(" %1=\"%2\"")
					.arg(attr.name().toString())
					.arg(Qt::escape(attr.value().toString()));
			}
			xml += '>';
			break;

		case EndElement:
			-- level;
			if (level == 0)
				return xml;
			xml += QString("</%1>").arg(qxml::name().toString());
			break;

		case Characters:
			xml += text();

		default:
			break;
		}
	}

	return xml;
}

QXmlStreamReader::TokenType
PodcastReader::nextRawToken()
{
	TokenType token = NoToken;
	
	for (int repeat = 0; repeat < 3; ++ repeat) {
		token = readNext();

		if ( error() != PrematureEndOfDocumentError )
			break;

		debug() << "recovering from PrematureEndOfDocumentError for "
                << qxml::name().toString() << " at "
                << qxml::lineNumber();
	}

	if( error() ) {
		throw XmlParseError( errorString() );
	}

	return token;
}

QXmlStreamReader::TokenType
PodcastReader::nextToken()
{
	TokenType token = NoToken;

	do {
		token = nextRawToken();

		switch ( token ) {
		case Invalid:
		case StartDocument:
		case EndDocument:
		case StartElement:
		case EndElement:
			return token;

		case Characters:
			if ( !isWhitespace() )
				return token;
			break;
		
		case NoToken:
		case Comment:
		case DTD:
		case EntityReference:
		case ProcessingInstruction:
			// ignore
			break;
		}
	} while ( !atEnd() );

	return token;
}

void
PodcastReader::expect(TokenType expected, TokenType got) {
	if ( got != expected )
		throw ParseError( i18n( "expected token %1, but got token %2", expected, got ) );
}

void
PodcastReader::expect(TokenType expected) {
	expect( expected, nextToken() );
}

void
PodcastReader::expectName(const QString& name) {
	if ( qxml::name() != name )
		throw ParseError( i18n( "expected element name %1, but got element name %2",
			name, qxml::name().toString() ) );
}

void
PodcastReader::expectStart(const QString& name) {
	expect(StartElement);
	expectName(name);
}

void
PodcastReader::expectEnd(const QString& name) {
	expect(EndElement);
	expectName(name);
}

QString
PodcastReader::readTextContent() {
	QString text = readElementText();

	if ( error() )
		throw ParseError( errorString() );
	
	return text;
}

bool
PodcastReader::read()
{
	DEBUG_BLOCK

	try {
		m_feedType = UnknownFeedType;

		expect(StartDocument);
		expect(StartElement);

		QStringRef version = attributes().value( "version" );

		debug() << "Initial StartElement: " << qxml::name().toString();
		debug() << "version: " << version.toString();

		if ( qxml::name() == "rss" && version == "2.0" ) {
			m_feedType = Rss20FeedType;
		}
		else if ( qxml::name() == "html" || qxml::name() == "HTML" ) {
			m_feedType = ErrorPageType;
			throw ParseError( i18n( "An HTML page was received. Expected an RSS 2.0 feed" ) );
		}
		else {
			// TODO: change this string once we support more
			throw ParseError( i18n( "%1 is not an RSS version 2.0 feed.", m_url.url() ) );
		}

		// rss 2.0 specifies exactly one channel element per feed
		readChannel();
		
		expect(EndElement);
		expect(EndDocument);
	}
	catch (XmlParseError& e) {
		debug() << "XML ERROR: " << error() << " at line: " << lineNumber()
		        << ": " << columnNumber()
		        << "\n\t" << errorString();
		debug() << "\tname = " << qxml::name().toString()
		        << " tokenType = " << tokenString();

		if (m_transferJob) {
			m_transferJob->kill();
			m_transferJob = NULL;
		}

		emit finished( this );
		return false;
	}
	catch (ParseError& e) {
		debug() << "error parsing podcast feed: " << e.message();

		raiseError( e.message() );

		if (m_transferJob) {
			m_transferJob->kill();
			m_transferJob = NULL;
		}

		emit finished( this );
		return false;
	}
	
	emit finished( this );
	return true;
}

KUrl
PodcastReader::readImage() {
	expectName("image");

	KUrl url;
	
	for (TokenType token = nextToken(); token != EndElement; token = nextToken()) {
		// elements: url, title, link
		expect( token, StartElement );

		if ( qxml::name() == "url" ) {
			url = KUrl( readTextContent() );
		}
		else if ( qxml::name() == "link" || qxml::name() == "title" ) {
			// not supported but well known
			skipElement();
		}
		else {
			debug() << "skipping unsupported image element: " << qxml::name().toString();
			skipElement();
		}
	}
	expectName("image");

	return url;
}

void
PodcastReader::readChannel() {
	DEBUG_BLOCK

	expectStart("channel");

	DescriptionType hasDescription = NoDescription;
	m_channel = new Meta::PodcastChannel();
   	m_channel->setUrl( m_url );
    m_channel->setSubscribeDate( QDate::currentDate() );
    /* add this new channel to the provider, we get a pointer to a
     * PodcastChannelPtr of the correct type which we will use from now on.
     */
    m_channel = m_podcastProvider->addChannel( m_channel );
	
	for (TokenType token = nextToken(); token != EndElement; token = nextToken()) {
		// required elements: title, link, description
		// optional elements: language, copyright, pubDate, category, image, ...

		expect( token, StartElement );

		if ( qxml::name() == "title" ) {
			// Remove redundant whitespace from the title.
			m_channel->setTitle( readTextContent().simplified() );
		}
		else if ( qxml::name() == "description" ) {
			if ( hasDescription <= RssDescription ) {
				m_channel->setDescription( readTextContent() );
				hasDescription = RssDescription;
			}
		}
		else if ( qxml::name() == "summary" && namespaceUri() == "http://www.itunes.com/dtds/podcast-1.0.dtd" ) {
			if ( hasDescription <= ItunesSummary ) {
				m_channel->setDescription( readTextContent() );
				hasDescription = ItunesSummary;
			}
		}
		else if ( qxml::name() == "body" ) {
			if ( hasDescription <= HtmlBody ) {
				m_channel->setDescription( readInnerXml() );
				hasDescription = HtmlBody;
			}
		}
		else if ( qxml::name() == "link" ) {
			m_channel->setWebLink( KUrl( readTextContent() ) );
		}
		else if ( qxml::name() == "image" ) {
			// TODO save image data
			m_channel->setImageUrl( readImage() );
		}
		else if ( qxml::name() == "item" ) {
			Meta::PodcastEpisodePtr item = readItem();

			if ( !m_podcastProvider->possiblyContainsTrack( item->uidUrl() ) ) {
				Meta::PodcastEpisodePtr episode = m_channel->addEpisode( item );
				// also let the provider know an episode has been added
				// TODO: change into a signal
				m_podcastProvider->addEpisode( episode );
			}
		}
		else {
			debug() << "skipping unsupported channel element: " << qxml::name().toString();
			skipElement();
		}
	}

	expectName("channel");
}

Meta::PodcastEpisodePtr
PodcastReader::readItem() {
	DEBUG_BLOCK

	expectName("item");

	Meta::PodcastEpisodePtr item( new Meta::PodcastEpisode( m_channel ) );
	DescriptionType hasDescription = NoDescription;

	for (TokenType token = nextToken(); token != EndElement; token = nextToken()) {
		// elements: title, link, description, author, category,
		//           comments, enclosure, guid, pubDate, source
		// extension elements: body, itunes:summary
		
		if ( qxml::name() == "title" ) {
			item->setTitle( readTextContent().simplified() );
		}
		else if ( qxml::name() == "description" ) {
			if ( hasDescription <= RssDescription ) {
				item->setDescription( readTextContent() );
				hasDescription = RssDescription;
			}
		}
		else if ( qxml::name() == "summary" && namespaceUri() == "http://www.itunes.com/dtds/podcast-1.0.dtd" ) {
			if ( hasDescription <= ItunesSummary ) {
				item->setDescription( readTextContent() );
				hasDescription = ItunesSummary;
			}
		}
		else if ( qxml::name() == "body" ) {
			if ( hasDescription <= HtmlBody ) {
				item->setDescription( readInnerXml() );
				hasDescription = HtmlBody;
			}
		}
		else if ( qxml::name() == "enclosure" ) {
			item->setUidUrl( KUrl( attributes().value( "url" ).toString() ) );
			skipElement();
		}
		else if ( qxml::name() == "guid" ) {
			item->setGuid( readTextContent() );
		}
		else if ( qxml::name() == "pubDate" ) {
			item->setPubDate( parsePubDate( readTextContent() ) );
		}
		else {
			debug() << "skipping unsupported item element: " << qxml::name().toString();
			skipElement();
		}
	}

	expectName("item");

	return item;
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
PodcastReader::skipElement()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() );

	int level = 1;

	while (level > 0) {
		switch ( nextToken() ) {
		case StartElement: ++ level; break;
		case EndElement:   -- level; break;
		default: break;
		}
	}
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
