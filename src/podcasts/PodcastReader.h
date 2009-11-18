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
        typedef enum {
            Document,
            TextContent,
            Rss,
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
            Summary,
            Body,
            Html,
            Unknown,
            Any
        } ElementType;

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

                virtual ~Action() {}

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
        
        // callback methods for feed parsing:
        void beginRss();
        void beginHtml();
        void beginUnknownFeedType();
        void beginEnclosure();
        void beginText();
        void beginChannel();
        void beginItem();
        void beginXml();

        void endRss();
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

        void readCharacters();

        void stopWithError(const QString &message);
        static const char* tokenToString(TokenType token);

        /** There usually are 3 kinds of descriptions. Usually a &lt;body&gt; element contains
         * the most detailed description followed by &lt;itunes:summary&gt; and the standard
         * &lt;description&gt;. */
        enum DescriptionType {
            NoDescription  = 0,
            RssDescription = 1,
            ItunesSummary  = 2,
            HtmlBody       = 3
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
        QString m_buffer;
        Meta::PodcastMetaCommon *m_current;

        class StaticData {
            public:
                StaticData();

                // This here basically builds a automata.
                // This way feed paraing can be paused after any token,
                // thus enabling paralell download and parsing of multiple
                // feeds without the need for threads.

                Action startAction;
                Action titleAction;
                Action descriptionAction;
                Action summaryAction;
                Action bodyAction;
                Action linkAction;
                Action skipAction;
                Action docAction;
                Action rssAction;
                Action htmlAction;
                Action unknownFeedTypeAction;
                Action channelAction;
                Action imageAction;
                Action itemAction;
                Action urlAction;
                Action authorAction;
                Action enclosureAction;
                Action guidAction;
                Action pubDateAction;
                Action xmlAction;

                ActionMap rootMap;
                ActionMap docMap;
                ActionMap rssMap;
                ActionMap channelMap;
                ActionMap imageMap;
                ActionMap itemMap;
                ActionMap skipMap;
                ActionMap xmlMap;
                ActionMap textMap;
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
