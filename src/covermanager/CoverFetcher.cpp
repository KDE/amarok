/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverFetcher.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "amarokconfig.h"
#include "CoverFetchQueue.h"
#include "CoverFoundDialog.h"
#include "NetworkAccessManagerProxy.h"

#include <KLocale>
#include <KUrl>

#include <QNetworkReply>

#define DEBUG_PREFIX "CoverFetcher"
#include "core/support/Debug.h"

CoverFetcher* CoverFetcher::s_instance = 0;

CoverFetcher*
CoverFetcher::instance()
{
    return s_instance ? s_instance : new CoverFetcher();
}

void CoverFetcher::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

CoverFetcher::CoverFetcher()
    : QObject()
    , m_limit( 10 )
    , m_dialog( 0 )
{
    setObjectName( "CoverFetcher" );

    m_queue = new CoverFetchQueue( this );
    connect( m_queue, SIGNAL(fetchUnitAdded(const CoverFetchUnit::Ptr)),
                      SLOT(slotFetch(const CoverFetchUnit::Ptr)) );

    connect( The::networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
                                          SLOT(slotResult(QNetworkReply*)) );
    s_instance = this;
}

CoverFetcher::~CoverFetcher()
{
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    debug() << QString("Adding interactive cover fetch for: '%1' from %2")
        .arg( album->name() )
        .arg( Amarok::config("Cover Fetcher").readEntry("Interactive Image Source", "LastFm") );
    switch( fetchSource() )
    {
    case CoverFetch::LastFm:
        m_queue->add( album, CoverFetch::Interactive, fetchSource() );
        break;

    case CoverFetch::Discogs:
    case CoverFetch::Google:
    case CoverFetch::Yahoo:
        queueQueryForAlbum( album );
        break;

    default:
        break;
    }
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    if( m_queue->size() > m_limit )
        m_queueLater.append( album );
    else
        m_queue->add( album, CoverFetch::Automatic );
    debug() << "Queueing automatic cover fetch for:" << album->name();
}

void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    foreach( Meta::AlbumPtr album, albums )
    {
        if( m_queue->size() > m_limit )
            m_queueLater.append( album );
        else
            m_queue->add( album, CoverFetch::Automatic );
    }
}

void
CoverFetcher::queueQuery( Meta::AlbumPtr album, const QString &query, int page )
{
    m_queue->addQuery( query, fetchSource(), page, album );
    debug() << QString( "Queueing cover fetch query: '%1' (page %2)" ).arg( query ).arg( page );
}

void
CoverFetcher::queueQueryForAlbum( Meta::AlbumPtr album )
{
    QString query( album->name() );
    if( album->hasAlbumArtist() )
        query += ' ' + album->albumArtist()->name();
    queueQuery( album, query, 0 );
}

void
CoverFetcher::slotFetch( const CoverFetchUnit::Ptr unit )
{
    if( !unit )
        return;

    const CoverFetchPayload *payload = unit->payload();
    const CoverFetch::Urls urls = payload->urls();

    // show the dialog straight away if fetch is interactive
    if( !m_dialog && unit->isInteractive() )
    {
        showCover( unit, QPixmap() );
    }
    else if( urls.isEmpty() )
    {
        finish( unit, NotFound );
        return;
    }

    const KUrl::List uniqueUrls = urls.uniqueKeys();
    foreach( const KUrl &url, uniqueUrls )
    {
        if( !url.isValid() )
            continue;

        QNetworkRequest req( url );
        The::networkAccessManager()->get( req );
        m_urls.insert( url, unit );

        if( unit->isInteractive() )
        {
            // FIXME: switching to using a QNAM means we can't monitor fetches using a KJob anymore
            // Amarok::Components::logger()->newProgressOperation( job, i18n( "Fetching Cover" ) );
        }
        else if( payload->type() == CoverFetchPayload::Art )
        {
            // only one is needed when the fetch is non-interactive
            return;
        }
    }
}

void
CoverFetcher::slotResult( QNetworkReply *reply )
{
    const KUrl url = reply->request().url();
    if( !m_urls.contains( url ) )
        return;

    const CoverFetchUnit::Ptr unit( m_urls.take( url ) );
    if( !unit )
    {
        reply->deleteLater();
        return;
    }

    if( reply->error() != QNetworkReply::NoError )
    {
        finish( unit, Error, i18n( "There was an error communicating with cover provider." ) );
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    const CoverFetchPayload *payload = unit->payload();

    switch( payload->type() )
    {
    case CoverFetchPayload::Info:
        m_queue->add( unit->album(), unit->options(), unit->payload()->source(), data );
        m_queue->remove( unit );
        break;

    case CoverFetchPayload::Search:
        m_queue->add( unit->options(), fetchSource(), data );
        m_queue->remove( unit );
        break;

    case CoverFetchPayload::Art:
        QPixmap pixmap;
        if( pixmap.loadFromData( data ) )
        {
            if( unit->isInteractive() )
            {
                const CoverFetch::Metadata metadata = payload->urls().value( url );
                showCover( unit, pixmap, metadata );
            }
            else
            {
                m_selectedPixmaps.insert( unit, pixmap );
                finish( unit );
            }
        }
        break;
    }
    reply->deleteLater();
}

void
CoverFetcher::slotDialogFinished()
{
    const CoverFetchUnit::Ptr unit = m_dialog->unit();
    switch( m_dialog->result() )
    {
    case KDialog::Accepted:
        m_selectedPixmaps.insert( unit, m_dialog->image() );
        finish( unit );
        break;

    case KDialog::Rejected:
        finish( unit, Cancelled );
        break;

    default:
        finish( unit, Error );
    }

    /*
     * Remove all manual fetch jobs from the queue if the user accepts, cancels,
     * or closes the cover found dialog. This way, the dialog will not reappear
     * if there are still covers yet to be retrieved.
     */
    QList< CoverFetchUnit::Ptr > units = m_urls.values();
    foreach( const CoverFetchUnit::Ptr &unit, units )
    {
        if( unit->isInteractive() )
            abortFetch( unit );
    }

    m_dialog->delayedDestruct();
}

void
CoverFetcher::showCover( CoverFetchUnit::Ptr unit, const QPixmap cover, CoverFetch::Metadata data )
{
    if( !m_dialog )
    {
        const Meta::AlbumPtr album = unit->album();
        if( !album )
        {
            finish( unit, Error );
            return;
        }

        m_dialog = new CoverFoundDialog( unit, data, static_cast<QWidget*>( parent() ) );
        connect( m_dialog, SIGNAL(newCustomQuery(Meta::AlbumPtr, const QString&, int)),
                           SLOT(queueQuery(Meta::AlbumPtr, const QString&, int)) );
        connect( m_dialog, SIGNAL(accepted()), SLOT(slotDialogFinished()) );
        connect( m_dialog, SIGNAL(rejected()), SLOT(slotDialogFinished()) );

        if( fetchSource() == CoverFetch::LastFm )
            queueQueryForAlbum( album );
        m_dialog->setQueryPage( 1 );

        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
    }
    else
    {
        if( !cover.isNull() )
        {
            typedef CoverFetchArtPayload CFAP;
            const CFAP *payload = dynamic_cast< const CFAP* >( unit->payload() );
            if( payload )
                m_dialog->add( cover, data, payload->imageSize() );
        }
    }
}

void
CoverFetcher::abortFetch( CoverFetchUnit::Ptr unit )
{
    Meta::AlbumPtr album = unit->album();
    m_queue->remove( album );
    m_queueLater.removeAll( album );
    m_selectedPixmaps.remove( unit );
    m_urls.remove( m_urls.key( unit ) );
}

void
CoverFetcher::finish( const CoverFetchUnit::Ptr unit,
                      CoverFetcher::FinishState state,
                      const QString &message )
{
    Meta::AlbumPtr album = unit->album();
    const QString albumName = album ? album->name() : QString();

    switch( state )
    {
    case Success:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Retrieved cover successfully for '%1'.", albumName );
            Amarok::Components::logger()->shortMessage( text );
            debug() << "Finished successfully for album" << albumName;
        }
        album->setImage( m_selectedPixmaps.take( unit ) );
        abortFetch( unit );
        break;

    case Error:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Fetching cover for '%1' failed.", albumName );
            Amarok::Components::logger()->shortMessage( text );
            QString debugMessage;
            if( !message.isEmpty() )
                debugMessage = '[' + message + ']';
            debug() << "Finished with errors for album" << albumName << debugMessage;
        }
        m_errors += message;
        break;

    case Cancelled:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Canceled fetching cover for '%1'.", albumName );
            Amarok::Components::logger()->shortMessage( text );
            debug() << "Finished, cancelled by user for album" << albumName;
        }
        break;

    case NotFound:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Unable to find a cover for '%1'.", albumName );
            //FIXME: Not visible behind cover manager
            Amarok::Components::logger()->shortMessage( text );
            m_errors += text;
            debug() << "Finished due to cover not found for album" << albumName;
        }
        break;
    }

    m_queue->remove( unit );

    if( !m_queueLater.isEmpty() )
    {
        const int diff = m_limit - m_queue->size();
        if( diff > 0 )
        {
            for( int i = 0; i < diff && !m_queueLater.isEmpty(); ++i )
            {
                Meta::AlbumPtr album = m_queueLater.takeFirst();
                // automatic fetching only uses Last.fm as source
                m_queue->add( album, CoverFetch::Automatic, CoverFetch::LastFm );
            }
        }
    }

    emit finishedSingle( static_cast< int >( state ) );
}

CoverFetch::Source
CoverFetcher::fetchSource() const
{
    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString sourceEntry = config.readEntry( "Interactive Image Source", "LastFm" );
    CoverFetch::Source source;
    if( sourceEntry == "LastFm" )
        source = CoverFetch::LastFm;
    else if( sourceEntry == "Google" )
        source = CoverFetch::Google;
    else if( sourceEntry == "Discogs" )
        source = CoverFetch::Discogs;
    else
        source = CoverFetch::Yahoo;
    return source;
}

#include "CoverFetcher.moc"

