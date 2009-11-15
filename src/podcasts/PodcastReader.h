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

#ifndef PODCASTREADER_H
#define PODCASTREADER_H

#include "PodcastProvider.h"
#include "PodcastMeta.h"

#include <QDateTime>
#include <QXmlStreamReader>
#include <QObject>


namespace KIO
{
    class Job;
    class TransferJob;
}

class KUrl;

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PodcastReader : public QObject, public QXmlStreamReader
{
    Q_OBJECT
    public:
        PodcastReader( PodcastProvider * podcastProvider );
        ~PodcastReader();

        bool read( QIODevice *device );
        bool read( const KUrl &url );
        bool read();
        bool update( Meta::PodcastChannelPtr channel );
        KUrl & url() { return m_url; }

        Meta::PodcastChannelPtr channel() { return m_channel; }

    signals:
        void finished( PodcastReader *podcastReader );

    private slots:
        void slotRedirection( KIO::Job *job, const KUrl & url );
        void slotPermanentRedirection ( KIO::Job * job, const KUrl & fromUrl,
                const KUrl & toUrl );
        void slotAbort();
        void slotAddData( KIO::Job *, const QByteArray & data );

        void downloadResult( KJob * );

    private:
		typedef QXmlStreamReader qxml;

		/** internally used exception class */
		class ParseError {
		public:
			ParseError(const QString& message)
				: m_message(message) {}

			const QString& message() const { return m_message; }

		private:
			QString m_message;
		};

		class XmlParseError : public ParseError {
		public:
			XmlParseError(const QString& message)
				: ParseError(message) {}
		};

		/** This method wraps readNext() and tries to fix the PrematureEndOfDocumentError. */
		TokenType nextRawToken();

		/** Like nextRawToken() but skips Comments, DTDs, EntityReferences,
		 * ProcessingInstructions and ignorable whitespaces. */
		TokenType nextToken();

		static void expect(TokenType expected, TokenType got);

		void expect(TokenType token);
		void expectName(const QString& name);
		void expectStart(const QString& name);
		void expectEnd(const QString& name);

		/** Read the inner xml of an element as a string. This is used to read the
		 * contents of a &lt;body&gt; element, which contains xhtml data. */
		QString readInnerXml();

		/** Read text content of an element. Raises error if non text content (elements) is read. */
		QString readTextContent();

		void readChannel();
		Meta::PodcastEpisodePtr readItem();
		KUrl readImage();

		/** There usually are 3 kinds of descriptions. Usually a &lt;body&gt; element contains
		 * the most detailed description followed by &lt;itunes:summary&gt; and the standard
		 * &lt;description&gt;. */
		enum DescriptionType { NoDescription = 0, RssDescription = 1, ItunesSummary = 2, HtmlBody = 3 };
        enum FeedType { UnknownFeedType, ErrorPageType, Rss20FeedType };

        FeedType m_feedType;
        KUrl m_url;
        PodcastProvider * m_podcastProvider;
        KIO::TransferJob *m_transferJob;
        Meta::PodcastChannelPtr m_channel;

        void skipElement();

        QDateTime parsePubDate( const QString &datestring );

        /** podcastEpisodeCheck
        * Check if this PodcastEpisode has been fetched before. Uses a scoring algorithm.
        * @return A pointer to a PodcastEpisode that has been fetched before or the \
        *   same pointer as the argument.
        */
        Meta::PodcastEpisodePtr podcastEpisodeCheck( Meta::PodcastEpisodePtr episode );
};

#endif
