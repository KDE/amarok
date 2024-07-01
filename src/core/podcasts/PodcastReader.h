/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *               2009 Mathias Panzenböck <grosser.meister.morti@gmx.net>                *
 *               2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "core/podcasts/PodcastProvider.h"
#include "core/podcasts/PodcastMeta.h"

#include <QDateTime>
#include <QXmlStreamReader>
#include <QObject>
#include <QRegularExpression>
#include <QStack>

#include <KIO/TransferJob>

class QUrl;
class KJob;

namespace Podcasts {

/** Class that parses a podcast xml file and provides the results to a PodcastProvider.

    @author Bart Cerneels <bart.cerneels@kde.org>
            Mathias Panzenböck <grooser.meister.morti@gmx.net>
*/
class AMAROKCORE_EXPORT PodcastReader : public QObject
{
    Q_OBJECT
    public:
        /** Create a new PodcastReader that delivers the result to the podcastProvider.
            Note: the PodcastProvider pointer is not owned by the PodcastReader and
                  must remain valid throughout the lifetime of this object.
        */
        explicit PodcastReader( PodcastProvider *podcastProvider, QObject *parent = nullptr );
        ~PodcastReader() override;

        bool read( QIODevice *device );
        bool read( const QUrl &url );
        bool update(const PodcastChannelPtr &channel );
        QUrl & url() { return m_url; }

        Podcasts::PodcastChannelPtr channel() { return m_channel; }

        QXmlStreamReader::Error error () const { return m_xmlReader.error(); }
        QString errorString () const { return m_xmlReader.errorString(); }

    Q_SIGNALS:
        void finished( PodcastReader *podcastReader );
        void statusBarErrorMessage( const QString &message );
        void statusBarNewProgressOperation( KIO::TransferJob *, const QString &, Podcasts::PodcastReader* );

    public Q_SLOTS:
        virtual void slotAbort();

    private Q_SLOTS:
        void slotRedirection( KIO::Job *job, const QUrl &url );
        void slotPermanentRedirection ( KIO::Job * job, const QUrl &fromUrl,
                const QUrl &toUrl );
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
            NewFeedUrl,
            Image,
            Link,
            Author,
            ItunesAuthor,
            Url,
            Title,
            EnclosureElement,
            Guid,
            PubDate,
            Description,
            Body,
            Html,
            Entry,
            Subtitle,
            ItunesSubtitle,
            Updated,
            Published,
            Summary,
            ItunesSummary,
            Keywords,
            ItunesKeywords,
            Content,
            SupportedContent,
            Name,
            Id,
            Logo,
            Icon,
            Creator,
            Encoded
        };

        class Action;
        typedef void (PodcastReader::*ActionCallback)();
        typedef QHash<ElementType, Action*> ActionMap;

        class Action
        {
            public:
                explicit Action( ActionMap &actionMap )
                    : m_actionMap( actionMap )
                    , m_begin( nullptr )
                    , m_end( nullptr )
                    , m_characters( nullptr ) {}

                Action(ActionMap &actionMap, ActionCallback begin)
                    : m_actionMap( actionMap )
                    , m_begin( begin )
                    , m_end( nullptr )
                    , m_characters( nullptr ) {}

                Action(ActionMap &actionMap, ActionCallback begin, ActionCallback end)
                    : m_actionMap( actionMap )
                    , m_begin( begin )
                    , m_end( end )
                    , m_characters( nullptr ) {}

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
                ActionMap        &m_actionMap;
                ActionCallback    m_begin;
                ActionCallback    m_end;
                ActionCallback    m_characters;
        };

        static bool mightBeHtml( const QString& text );

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
        void beginImage();
        void beginXml();
        void beginNoElement();
        void beginAtomText();
        void beginAtomFeedLink();
        void beginAtomEntryLink();
        void beginAtomTextChild();

        void endDocument();
        void endTitle();
        void endSubtitle();
        void endDescription();
        void endEncoded();
        void endBody();
        void endLink();
        void endGuid();
        void endPubDate();
        void endItem();
        void endImageUrl();
        void endKeywords();
        void endNewFeedUrl();
        void endAuthor();
        void endCreator();
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

        QDateTime parsePubDate( const QString &datestring );

        void stopWithError(const QString &message);

        static QString unescape( const QString &text );
        static QString textToHtml( const QString &text );

        QString atomTextAsText();
        QString atomTextAsHtml();

        QStringView attribute(const char *namespaceUri, const char *name) const;
        bool hasAttribute(const char *namespaceUri, const char *name) const;

        void setDescription(const QString &description);
        void setSummary(const QString &description);

        /** podcastEpisodeCheck
        * Check if this PodcastEpisode has been fetched before. Uses a scoring algorithm.
        * @return A pointer to a PodcastEpisode that has been fetched before or the \
        *   same pointer as the argument.
        */
        Podcasts::PodcastEpisodePtr podcastEpisodeCheck( Podcasts::PodcastEpisodePtr episode );

        // TODO: move this to PodcastMeta and add a field
        //       descriptionType to PodcastCommonMeta.
        enum ContentType
        {
            TextContent,
            HtmlContent,
            XHtmlContent
        };

        class Enclosure
        {
            public:
                Enclosure(const QUrl &url, int filesize, const QString& mimeType)
                    : m_url( url ), m_filesize( filesize ), m_mimeType( mimeType ) {}

                const QUrl &url() const { return m_url; }
                int fileSize() const { return m_filesize; }
                const QString &mimeType() const { return m_mimeType; }

            private:
                QUrl    m_url;
                int     m_filesize;
                QString m_mimeType;
        };

        class StaticData {
            public:
                StaticData();

                // This here basically builds an automata.
                // This way feed parsing can be paused after any token,
                // thus enabling paralell download and parsing of multiple
                // feeds without the need for threads.

                QHash<QString, ElementType> knownElements;
                QRegularExpression removeScripts;
                QRegularExpression mightBeHtml;
                QRegularExpression linkify;
                
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
                Action subtitleAction;
                Action descriptionAction;
                Action encodedAction;
                Action bodyAction;
                Action linkAction;
                Action imageAction;
                Action itemAction;
                Action urlAction;
                Action authorAction;
                Action creatorAction;
                Action enclosureAction;
                Action guidAction;
                Action pubDateAction;
                Action keywordsAction;
                Action newFeedUrlAction;

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

        QXmlStreamReader m_xmlReader;

        QUrl m_url;
        PodcastProvider *m_podcastProvider;
        KIO::TransferJob *m_transferJob;
        Podcasts::PodcastChannelPtr m_channel;
        Podcasts::PodcastEpisodePtr m_item;

        /** This points to the data of the current channel or (if parsing an item) 
            the data of the current item */
        Podcasts::PodcastMetaCommon *m_current;

        // this somewhat emulates a callstack (without local variables):
        QStack<const Action*> m_actionStack;

        ContentType m_contentType;
        QString m_buffer;
        QList<Enclosure> m_enclosures;

};

} //namespace Podcasts

#endif
