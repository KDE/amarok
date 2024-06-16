/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "OpmlParser.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QFile>
#include <QXmlStreamReader>

#include <KLocalizedString>
#include <kio/job.h>

const QString OpmlParser::OPML_MIME = "text/x-opml+xml";

const OpmlParser::StaticData OpmlParser::sd;

OpmlParser::OpmlParser( const QUrl &url )
        : QObject()
        , ThreadWeaver::Job()
        , QXmlStreamReader()
        , m_url( url )
{
}

OpmlParser::~OpmlParser()
{
}

void
OpmlParser::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    read( m_url );
}

void
OpmlParser::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
OpmlParser::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

bool
OpmlParser::read( const QUrl &url )
{
    m_url = url;
    if( m_url.isLocalFile() )
    {
        //read directly from local file
        QFile localFile( m_url.toLocalFile() );
        if( !localFile.open( QIODevice::ReadOnly ) )
        {
            debug() << "failed to open local OPML file " << m_url.url();
            return false;
        }

        return read( &localFile );
    }

    m_transferJob = KIO::get( m_url, KIO::Reload, KIO::HideProgressInfo );

    connect( m_transferJob, &KIO::TransferJob::data,
             this, &OpmlParser::slotAddData );

    connect( m_transferJob, &KIO::TransferJob::result,
             this, &OpmlParser::downloadResult );

    // parse data
    return read();
}

bool
OpmlParser::read( QIODevice *device )
{
    setDevice( device );
    return read();
}

void
OpmlParser::slotAddData( KIO::Job *job, const QByteArray &data )
{
    Q_UNUSED( job )

    QXmlStreamReader::addData( data );

    // parse more data
    continueRead();
}

void
OpmlParser::downloadResult( KJob *job )
{
    // parse more data
    continueRead();

    KIO::TransferJob *transferJob = dynamic_cast<KIO::TransferJob *>( job );
    if( job->error() || ( transferJob && transferJob->isErrorPage() ) )
    {
        QString errorMessage =
            i18n( "Reading OPML podcast from %1 failed with error:\n", m_url.url() );
        errorMessage = errorMessage.append( job->errorString() );

//        Q_EMIT statusBarErrorMessage( errorMessage );
    }

    m_transferJob = nullptr;
}

void
OpmlParser::slotAbort()
{
    DEBUG_BLOCK
}

void
OpmlParser::Action::begin( OpmlParser *opmlParser ) const
{
    if( m_begin )
        (( *opmlParser ).*m_begin )();
}

void
OpmlParser::Action::end( OpmlParser *opmlParser ) const
{
    if( m_end )
        (( *opmlParser ).*m_end )();
}

void
OpmlParser::Action::characters( OpmlParser *opmlParser ) const
{
    if( m_characters )
        (( *opmlParser ).*m_characters )();
}

// initialization of the feed parser automata:
OpmlParser::StaticData::StaticData()
    : startAction( rootMap )
    , docAction(
        docMap,
        nullptr,
        &OpmlParser::endDocument )
    , skipAction( skipMap )
    , noContentAction(
            noContentMap,
            &OpmlParser::beginNoElement,
            nullptr,
            &OpmlParser::readNoCharacters )
    , opmlAction(
            opmlMap,
            &OpmlParser::beginOpml )
    , headAction(
            headMap,
            nullptr,
            &OpmlParser::endHead )
    , titleAction(
            textMap,
            &OpmlParser::beginText,
            &OpmlParser::endTitle,
            &OpmlParser::readCharacters )
    , bodyAction( bodyMap )
    , outlineAction(
            outlineMap,
            &OpmlParser::beginOutline,
            &OpmlParser::endOutline )
{
    // known elements:
    knownElements[ "opml" ] = Opml;
    knownElements[ "html" ] = Html;
    knownElements[ "HTML" ] = Html;
    knownElements[ "head" ] = Head;
    knownElements[ "title" ] = Title;
    knownElements[ "dateCreated" ] = DateCreated;
    knownElements[ "dateModified" ] = DateModified;
    knownElements[ "ownerName" ] = OwnerName;
    knownElements[ "ownerEmail" ] = OwnerEmail;
    knownElements[ "ownerId" ] = OwnerId;
    knownElements[ "docs" ] = Docs;
    knownElements[ "expansionState" ] = ExpansionState;
    knownElements[ "vertScrollState" ] = VertScrollState;
    knownElements[ "windowTop" ] = WindowTop;
    knownElements[ "windowLeft" ] = WindowLeft;
    knownElements[ "windowBottom" ] = WindowBottom;
    knownElements[ "windowRight" ] = WindowRight;
    knownElements[ "body" ] = Body;
    knownElements[ "outline" ] = Outline;

    // before start document/after end document
    rootMap.insert( Document, &docAction );

    // parse document
    docMap.insert( Opml, &opmlAction );
//    docMap.insert( Html, &htmlAction );

    // parse <opml>
    opmlMap.insert( Head, &headAction );
    opmlMap.insert( Body, &bodyAction );

    // parse <head>
    headMap.insert( Title, &titleAction );
    headMap.insert( DateCreated, &skipAction );
    headMap.insert( DateModified, &skipAction );
    headMap.insert( OwnerName, &skipAction );
    headMap.insert( OwnerEmail, &skipAction );
    headMap.insert( OwnerId, &skipAction );
    headMap.insert( Docs, &skipAction );
    headMap.insert( ExpansionState, &skipAction );
    headMap.insert( VertScrollState, &skipAction );
    headMap.insert( WindowTop, &skipAction );
    headMap.insert( WindowLeft, &skipAction );
    headMap.insert( WindowBottom, &skipAction );
    headMap.insert( WindowRight, &skipAction );

    // parse <body>
    bodyMap.insert( Outline, &outlineAction );

    // parse <outline> in case of sub-elements
    outlineMap.insert( Outline, &outlineAction );

    // skip elements
    skipMap.insert( Any, &skipAction );

}

OpmlParser::ElementType
OpmlParser::elementType() const
{
    if( isEndDocument() || isStartDocument() )
        return Document;

    if( isCDATA() || isCharacters() )
        return CharacterData;

    ElementType elementType = sd.knownElements[ QXmlStreamReader::name().toString()];

    return elementType;
}

bool
OpmlParser::read()
{
    m_buffer.clear();
    m_actionStack.clear();
    m_actionStack.push( &( OpmlParser::sd.startAction ) );
    setNamespaceProcessing( false );

    return continueRead();
}

bool
OpmlParser::continueRead()
{
    // this is some kind of pushdown automata
    // with this it should be possible to parse feeds in parallel
    // without using threads
    DEBUG_BLOCK

    while( !atEnd() && error() != CustomError )
    {
        TokenType token = readNext();

        if( error() == PrematureEndOfDocumentError && m_transferJob )
            return true;

        if( hasError() )
        {
            Q_EMIT doneParsing();
            return false;
        }

        if( m_actionStack.isEmpty() )
        {
            debug() << "expected element on stack!";
            return false;
        }

        const Action* action = m_actionStack.top();
        const Action* subAction = nullptr;

        switch( token )
        {
            case Invalid:
            {
                debug() << "invalid token received at line " << lineNumber();
                debug() << "Error:\n" << errorString();
                return false;
            }

            case StartDocument:
            case StartElement:
                subAction = action->actionMap()[ elementType() ];

                if( !subAction )
                    subAction = action->actionMap()[ Any ];

                if( !subAction )
                    subAction = &( OpmlParser::sd.skipAction );

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

                // ignorable whitespaces
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
OpmlParser::stopWithError( const QString &message )
{
    raiseError( message );

    if( m_transferJob )
    {
        m_transferJob->kill( KJob::EmitResult );
        m_transferJob = nullptr;
    }

    Q_EMIT doneParsing();
}

void
OpmlParser::beginOpml()
{
    m_outlineStack.clear();
}

void
OpmlParser::beginText()
{
    m_buffer.clear();
}

void
OpmlParser::beginOutline()
{
    OpmlOutline *parent = m_outlineStack.empty() ? nullptr : m_outlineStack.top();
    OpmlOutline *outline = new OpmlOutline( parent );
    //adding outline to stack
    m_outlineStack.push( outline );
    if( parent )
    {
        parent->setHasChildren( true );
        parent->addChild( outline );
    }

    for( const QXmlStreamAttribute &attribute : attributes() )
        outline->addAttribute( attribute.name().toString(), attribute.value().toString() );

    Q_EMIT outlineParsed( outline );
}

void
OpmlParser::beginNoElement()
{
    debug() << "no element expected here, but got element: " << QXmlStreamReader::name();
}

void
OpmlParser::endDocument()
{
    Q_EMIT doneParsing();
}

void
OpmlParser::endHead()
{
    Q_EMIT headerDone();
}

void
OpmlParser::endTitle()
{
    m_headerData.insert( "title", m_buffer.trimmed() );
}

void
OpmlParser::endOutline()
{
    OpmlOutline *outline = m_outlineStack.pop();
    if( m_outlineStack.isEmpty() )
        m_outlines << outline;
}

void
OpmlParser::readCharacters()
{
    m_buffer += text();
}

void
OpmlParser::readNoCharacters()
{
    DEBUG_BLOCK
    debug() << "no characters expected here";
}
