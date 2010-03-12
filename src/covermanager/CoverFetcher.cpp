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

#include "Amarok.h"
#include "amarokconfig.h"
#include "CoverFetchQueue.h"
#include "CoverFoundDialog.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"
#include "ui_EditCoverSearchDialog.h"

#include <KIO/Job>
#include <KLocale>
#include <KUrl>

#define DEBUG_PREFIX "CoverFetcher"

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
    DEBUG_FUNC_INFO
    setObjectName( "CoverFetcher" );

    m_queue = new CoverFetchQueue( this );
    connect( m_queue, SIGNAL(fetchUnitAdded(const CoverFetchUnit::Ptr)),
                      SLOT(slotFetch(const CoverFetchUnit::Ptr)) );

    s_instance = this;
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    DEBUG_BLOCK
    m_queue->add( album, CoverFetch::Interactive );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    if( m_queue->size() > m_limit )
        m_queueLater.append( album );
    else
        m_queue->add( album, CoverFetch::Automatic );
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
CoverFetcher::queueQuery( const QString &query )
{
    m_queue->addQuery( query );
}

void
CoverFetcher::slotFetch( const CoverFetchUnit::Ptr unit )
{
    DEBUG_BLOCK

    if( !unit )
        return;

    const CoverFetchPayload *payload = unit->payload();
    const CoverFetch::Urls urls = payload->urls();

    if( urls.isEmpty() )
    {
        if( unit->isInteractive() )
        {
            The::statusBar()->shortMessage( i18n( "No covers found." ) );
            showCover( unit );
        }
        else
        {
            finish( unit, NotFound );
        }
        return;
    }

    const KUrl::List uniqueUrls = urls.uniqueKeys();
    foreach( const KUrl &url, uniqueUrls )
    {
        KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(slotResult( KJob* )) );
        m_jobs.insert( job, unit );

        if( unit->isInteractive() )
        {
            The::statusBar()->newProgressOperation( job, i18n( "Fetching Cover" ) );
        }
        else if( payload->type() == CoverFetchPayload::Art )
        {
            // only one is needed when the fetch is non-interactive
            return;
        }
    }
}

void
CoverFetcher::slotResult( KJob *job )
{
    DEBUG_BLOCK
    const CoverFetchUnit::Ptr unit( m_jobs.take( job ) );

    if( !unit )
        return;

    if( job && job->error() )
    {
        finish( unit, Error, i18n( "There was an error communicating with last.fm." ) );
        return;
    }

    KIO::StoredTransferJob *const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    const QByteArray data = storedJob->data();
    storedJob->deleteLater();
    
    const CoverFetchPayload *payload = unit->payload();

    switch( payload->type() )
    {
    case CoverFetchPayload::Info:
        m_queue->add( unit->album(), unit->options(), data );
        m_queue->remove( unit );
        break;

    case CoverFetchPayload::Search:
        m_queue->add( unit->options(), data );
        m_queue->remove( unit );
        break;

    case CoverFetchPayload::Art:
        QPixmap pixmap;
        if( pixmap.loadFromData( data ) )
        {
            if( unit->isInteractive() )
            {
                const KUrl url = storedJob->url();
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
    The::statusBar()->endProgressOperation( job ); //just to be safe...
}

void
CoverFetcher::slotDialogFinished()
{
    /*
     * Remove all manual fetch jobs from the queue if the user accepts, cancels,
     * or closes the cover found dialog. This way, the dialog will not reappear
     * if there are still covers yet to be retrieved.
     */
    QList< CoverFetchUnit::Ptr > units = m_jobs.values();
    foreach( const CoverFetchUnit::Ptr &unit, units )
    {
        if( unit->isInteractive() )
        {
            m_queue->remove( unit );
            m_queueLater.removeAll( unit->album() );
            m_selectedPixmaps.remove( unit );

            const KJob *job = m_jobs.key( unit );
            const_cast< KJob* >( job )->kill();
            The::statusBar()->endProgressOperation( job );
            m_jobs.remove( job );
        }
    }
}

void
CoverFetcher::showCover( CoverFetchUnit::Ptr unit, const QPixmap cover, CoverFetch::Metadata data )
{
    DEBUG_BLOCK

    if( cover.isNull() )
    {
        finish( unit, Error );
        return;
    }

    if( !m_dialog )
    {
        const Meta::AlbumPtr album = unit->album();
        if( !album )
        {
            finish( unit, Error );
            return;
        }

        m_dialog = new CoverFoundDialog( album, cover, data, static_cast<QWidget*>( parent() ) );
        connect( m_dialog, SIGNAL(newCustomQuery(const QString&)), SLOT(queueQuery(const QString&)) );
        connect( m_dialog, SIGNAL(finished()), SLOT(slotDialogFinished()) );

        switch( m_dialog->exec() )
        {
        case KDialog::Accepted:
            m_selectedPixmaps.insert( unit, m_dialog->image() );
            finish( unit );
            break;

        case KDialog::Rejected:
        default:
            finish( unit, Cancelled );
            break;
        }

        delete m_dialog;
        m_dialog = 0;
    }
    else
    {
        if( !cover.isNull() )
        {
            typedef CoverFetchArtPayload CFAP;
            const CFAP *payload = dynamic_cast< const CFAP* >( unit->payload() );
            const CoverFetch::ImageSize imageSize = payload->imageSize();
            m_dialog->add( cover, data, imageSize );
        }
    }
}

void
CoverFetcher::finish( const CoverFetchUnit::Ptr unit,
                      CoverFetcher::FinishState state,
                      const QString &message )
{
    DEBUG_BLOCK

    const QString albumName = unit->album() ? unit->album()->name() : QString();

    switch( state )
    {
        case Success:
        {
            const QString text = i18n( "Retrieved cover successfully for '%1'.", albumName );
            The::statusBar()->shortMessage( text );
            unit->album()->setImage( m_selectedPixmaps.take( unit ) );
            break;
        }

        case Error:
        {
            const QString text = i18n( "Fetching cover for '%1' failed.", albumName );
            The::statusBar()->shortMessage( text );
            m_errors += message;
            break;
        }

        case Cancelled:
        {
            const QString text = i18n( "Canceled fetching cover for '%1'.", albumName );
            The::statusBar()->shortMessage( text );
            break;
        }

        case NotFound:
        {
            const QString text = i18n( "Unable to find a cover for '%1'.", albumName );
            //FIXME: Not visible behind cover manager
            The::statusBar()->shortMessage( text );
            m_errors += text;
            break;
        }
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
                m_queue->add( album, CoverFetch::Automatic );
            }
        }
    }

    emit finishedSingle( static_cast< int >( state ) );
}

#include "CoverFetcher.moc"

