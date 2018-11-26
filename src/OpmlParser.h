/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef OPMLPARSER_H
#define OPMLPARSER_H

#include "amarok_export.h"
#include "OpmlOutline.h"

#include <QMap>
#include <QStack>
#include <QStringList>
#include <QUrl>
#include <QXmlStreamReader>

#include <KJob>
#include <ThreadWeaver/Job>

namespace KIO
{
    class Job;
    class TransferJob;
}

/**
* Parser for OPML files.
*/
class AMAROK_EXPORT OpmlParser : public QObject, public ThreadWeaver::Job, public QXmlStreamReader
{
    Q_OBJECT

public:
    static const QString OPML_MIME;
    /**
     * Constructor
     * @param url The address to parse 
     * @return Pointer to new object
     */
    explicit OpmlParser( const QUrl &url );

    /**
     * Destructor
     * @return none
     */
    ~OpmlParser();

    /**
    * The function that starts the actual work. Inherited from ThreadWeaver::Job
    * Note the work is performed in a separate thread
    * @return Returns true on success and false on failure
    */
    void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    bool read( const QUrl &url );
    bool read( QIODevice *device );

    /** @return the URL of the OPML being parsed.
    */
    QUrl url() const { return m_url; }

    QMap<QString,QString> headerData() { return m_headerData; }

    /**
     * Get the result of the parsing as a list of OpmlOutlines.
     * This list contains only root outlines that can be found in the <body> of the OPML.
     * The rest are children of these root items.
     *
     * The user is responsible for deleting the results.
     */
    QList<OpmlOutline *> results() const { return m_outlines; }

protected:
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

Q_SIGNALS:

    /**
     * Emitted when <head> has been completely parsed.
     */
    void headerDone();

    /**
     * Signal emmited when parsing is complete.
     * The data is complete now and accessible via results().
     * Children of all the outlines are available via OpmlOutline::children().
     */
    void doneParsing();

    /**
     * Emitted when a new outline item is available.
     * Emitted after the attributes have been read but before any of the children are available.
     * Each child will be reported in a separate signal.
     */
    void outlineParsed( OpmlOutline *outline );

    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

public Q_SLOTS:
    virtual void slotAbort();

private Q_SLOTS:
    void slotAddData( KIO::Job *, const QByteArray &data );

    void downloadResult( KJob * );

private:
    enum ElementType
    {
        Unknown = 0,
        Any,
        Document,
        CharacterData,
        Opml,
        Html,
        Head,
        Title,
        DateCreated,
        DateModified,
        OwnerName,
        OwnerEmail,
        OwnerId,
        Docs,
        ExpansionState,
        VertScrollState,
        WindowTop,
        WindowLeft,
        WindowBottom,
        WindowRight,
        Body,
        Outline
    };

    class Action;
    typedef void (OpmlParser::*ActionCallback)();
    typedef QHash<ElementType, Action*> ActionMap;

    class Action
    {
        public:
            explicit Action( ActionMap &actionMap )
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

            void begin( OpmlParser *opmlParser ) const;
            void end( OpmlParser *opmlParser ) const;
            void characters( OpmlParser *opmlParser ) const;

            const ActionMap &actionMap() const { return m_actionMap; }

        private:
            ActionMap &m_actionMap;
            ActionCallback m_begin;
            ActionCallback m_end;
            ActionCallback m_characters;
    };

    ElementType elementType() const;
    bool read();
    bool continueRead();

    // callback methods for parsing
    void beginOpml();
    void beginText();
    void beginOutline();
    void beginNoElement();

    void endDocument();
    void endHead();
    void endTitle();
    void endOutline();

    void readCharacters();
    void readNoCharacters();

    void stopWithError( const QString &message );

    class StaticData {
        public:
            StaticData();

            // This here basically builds an automata.
            // This way feed parsing can be paused after any token,
            // thus enabling parallel download and parsing of multiple
            // feeds without the need for threads.

            QHash<QString, ElementType> knownElements;

            //Actions
            Action startAction;

            Action docAction;
            Action skipAction;
            Action noContentAction;

            Action opmlAction;

            Action headAction;
            Action titleAction;
//            Action dateCreatedAction;
//            Action dateModifiedAction;
//            Action ownerNameAction;
//            Action ownerEmailAction;
//            Action ownerIdAction;
//            Action docsAction;
//            Action expansionStateAction;
            Action bodyAction;
            Action outlineAction;

            ActionMap rootMap;
            ActionMap skipMap;
            ActionMap noContentMap;
            ActionMap xmlMap;

            ActionMap docMap;
            ActionMap opmlMap;
            ActionMap headMap;
            ActionMap bodyMap;
            ActionMap outlineMap;
            ActionMap textMap;
    };

    static const StaticData sd;

    QStack<const Action*> m_actionStack;

    QString m_buffer;

    QMap<QString,QString> m_headerData;
    // the top level outlines of <body>.
    QList<OpmlOutline *> m_outlines;

    // currently processing outlines so we can do nested outlines.
    QStack<OpmlOutline *> m_outlineStack;

    QUrl m_url;
    KIO::TransferJob *m_transferJob;
};

#endif
