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
#include <QStack>

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
        /** these are the keys used by the automata */
        enum ElementType
        {
            Unknown = 0,
            Any,
            Document,
            CharacterData,
            Rss,
            Rdf,
            Feed,
            Channel,
            Item,
            Image,
            Link,
            Author,
            Url,
            Title,
            Enclosure,
            Guid,
            PubDate,
            Description,
            ItunesSummary,
            Body,
            Html,
            Entry,
            Subtitle,
            Updated,
            Published,
            Summary,
            Content,
            SupportedContent,
            Name,
            Id,
            Logo,
            Icon
        };

        class Action;
        typedef void (PodcastReader::*ActionCallback)();
        typedef QHash<ElementType, Action*> ActionMap;

        class Action
        {
            public:
                Action( ActionMap &actionMap )
                    : m_actionMap( actionMap )
                    , m_begin( 0 )
                    , m_end( 0 )
                    , m_characters( 0 ) {}

                Action(ActionMap &actionMap, ActionCallback begin)
                    : m_actionMap( actionMap )
                    , m_begin( begin )
                    , m_end( 0 )
                    , m_characters( 0 ) {}

                Action(ActionMap &actionMap, ActionCallback begin, ActionCallback end)
                    : m_actionMap( actionMap )
                    , m_begin( begin )
                    , m_end( end )
                    , m_characters( 0 ) {}

                Action(ActionMap &actionMap, ActionCallback begin,
                        ActionCallback end, ActionCallback characters)
                    : m_actionMap( actionMap )
                    , m_begin( begin )
                    , m_end( end )
                    , m_characters( characters ) {}

                void begin(PodcastReader *podcastReader) const;
                void end(PodcastReader *podcastReader) const;
                void characters(PodcastReader *podcastReader) const;

                const ActionMap &actionMap() const { return m_actionMap; }

            private:
                ActionMap      &m_actionMap;
                ActionCallback  m_begin;
                ActionCallback  m_end;
                ActionCallback  m_characters;
        };

        ElementType elementType() const;
        bool read();
        bool continueRead();
        void createChannel();
        
        // callback methods for feed parsing:
        void beginRss();
        void beginRdf();
        void beginFeed();
        void beginHtml();
        void beginUnknownFeedType();
        void beginEnclosure();
        void beginText();
        void beginChannel();
        void beginItem();
        void beginXml();
        void beginNoElement();
        void beginAtomText();
        void beginAtomFeedLink();
        void beginAtomEntryLink();
        void beginAtomTextChild();

        void endDocument();
        void endTitle();
        void endDescription();
        void endItunesSummary();
        void endBody();
        void endLink();
        void endGuid();
        void endPubDate();
        void endItem();
        void endImageUrl();
        void endAuthor();
        void endXml();
        void endAtomLogo();
        void endAtomIcon();
        void endAtomTitle();
        void endAtomSubtitle();
        void endAtomPublished();
        void endAtomUpdated();
        void endAtomSummary();
        void endAtomContent();
        void endAtomTextChild();

        // TODO: maybe I can remove readCharacters() and readEscapedCharacters()
        //       and use readAtomTextCharacters() plus setting m_contentType even
        //       in Rss 1.0/2.0 parsers instead.
        void readCharacters();
        void readNoCharacters();
        void readEscapedCharacters();
        void readAtomTextCharacters();

        void stopWithError(const QString &message);

        static QString unescape( const QString &text );

        QString atomTextAsText();
        QString atomTextAsHtml();

        QStringRef attribute(const char *namespaceUri, const char *name) const;
        bool hasAttribute(const char *namespaceUri, const char *name) const;

        void moveDescription(const QString &description);
        void moveItunesSummary(const QString &summary);

        /** There usually are 3 kinds of descriptions. Usually a &lt;body&gt; element contains
         * the most detailed description followed by &lt;itunes:summary&gt; and the standard
         * &lt;description&gt;. */
        enum DescriptionType
        {
            NoDescription             = 0,
            RssDescription            = 1,
            ItunesSummaryDescription  = 2,
            HtmlBodyDescription       = 3
        };

        enum ContentType
        {
            TextContent,
            HtmlContent,
            XHtmlContent
        };

        KUrl m_url;
        PodcastProvider *m_podcastProvider;
        KIO::TransferJob *m_transferJob;
        Meta::PodcastChannelPtr m_channel;
        Meta::PodcastEpisodePtr m_item;
        
        // this somewhat emulates a callstack (whithout local variables):
        QStack<const Action*> m_actionStack;
        
        DescriptionType m_descriptionType;
        DescriptionType m_channelDescriptionType;
        ContentType m_contentType;
        QString m_buffer;
        Meta::PodcastMetaCommon *m_current;

        class StaticData {
            public:
                StaticData();

                // This here basically builds an automata.
                // This way feed parsing can be paused after any token,
                // thus enabling paralell download and parsing of multiple
                // feeds without the need for threads.

                QHash<QString, ElementType> knownElements;
                
                // Actions
                Action startAction;
                
                Action docAction;
                Action xmlAction;
                Action skipAction;
                Action noContentAction;

                Action rdfAction;  // RSS 1.0
                Action rssAction;  // RSS 2.0
                Action feedAction; // Atom
                Action htmlAction;
                Action unknownFeedTypeAction;

                // RSS 1.0+2.0
                Action rss10ChannelAction;
                Action rss20ChannelAction;

                Action titleAction;
                Action descriptionAction;
                Action summaryAction;
                Action bodyAction;
                Action linkAction;
                Action imageAction;
                Action itemAction;
                Action urlAction;
                Action authorAction;
                Action enclosureAction;
                Action guidAction;
                Action pubDateAction;

                // Atom
                Action atomLogoAction;
                Action atomIconAction;
                Action atomEntryAction;
                Action atomTitleAction;
                Action atomSubtitleAction;
                Action atomAuthorAction;
                Action atomFeedLinkAction;
                Action atomEntryLinkAction;
                Action atomIdAction;
                Action atomPublishedAction;
                Action atomUpdatedAction;
                Action atomSummaryAction;
                Action atomContentAction;
                Action atomTextAction;
                
                // ActionMaps
                ActionMap rootMap;
                ActionMap skipMap;
                ActionMap noContentMap;
                ActionMap xmlMap;

                ActionMap docMap;
                ActionMap rssMap;
                ActionMap rdfMap;
                ActionMap feedMap;

                ActionMap rss10ChannelMap;
                ActionMap rss20ChannelMap;
                ActionMap imageMap;
                ActionMap itemMap;
                ActionMap textMap;

                ActionMap atomEntryMap;
                ActionMap atomAuthorMap;
                ActionMap atomTextMap;
        };

        static const StaticData sd;

        QDateTime parsePubDate( const QString &datestring );

        /** podcastEpisodeCheck
        * Check if this PodcastEpisode has been fetched before. Uses a scoring algorithm.
        * @return A pointer to a PodcastEpisode that has been fetched before or the \
        *   same pointer as the argument.
        */
        Meta::PodcastEpisodePtr podcastEpisodeCheck( Meta::PodcastEpisodePtr episode );
};

#endif
