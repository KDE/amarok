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

CoverFetcher::CoverFetcher() : QObject()
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
    m_queue->add( album, true );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    m_queue->add( album, false );
}

void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    foreach( Meta::AlbumPtr album, albums )
    {
        m_queue->add( album, false );
    }
}

void
CoverFetcher::slotFetch( const CoverFetchUnit::Ptr unit )
{
    DEBUG_BLOCK
    const CoverFetchPayload *payload = unit->url();
    const KUrl::List urls = payload->urls();

    foreach( const KUrl &url, urls )
    {
        KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(slotResult( KJob* )) );
        m_jobs.insert( job, unit );

        if( unit->isInteractive() )
            The::statusBar()->newProgressOperation( job, i18n( "Fetching Cover" ) );
    }
}

void
CoverFetcher::slotResult( KJob *job )
{
    DEBUG_BLOCK
    const CoverFetchUnit::Ptr unit( m_jobs.take( job ) );

    if( job && job->error() )
    {
        finish( unit, Error, i18n( "There was an error communicating with last.fm." ) );
        return;
    }

    KIO::StoredTransferJob *const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    const QByteArray data = storedJob->data();

    const CoverFetchPayload *payload = unit->url();

    switch( payload->type() )
    {
    case CoverFetchPayload::INFO:
        m_queue->add( unit->album(), unit->isInteractive(), data );
        break;

    case CoverFetchPayload::ART:
        QPixmap pixmap;
        if( pixmap.loadFromData( data ) )
        {
            QList< QPixmap > list;
            if( m_pixmaps.contains( unit ) )
            {
                list = m_pixmaps.take( unit );
            }
            m_pixmaps.insert( unit, list << pixmap );

            if( unit->isInteractive() )
            {
                showCover( unit );
            }
            else
            {
                finish( unit );
            }
        }
        else
        {
            finish( unit, NotFound );
        }
        break;
    }
    The::statusBar()->endProgressOperation( job ); //just to be safe...
}

void
CoverFetcher::showCover( const CoverFetchUnit::Ptr unit )
{
    DEBUG_BLOCK
    QList< QPixmap > pixmaps = m_pixmaps.value( unit );

    Meta::AlbumPtr album = unit->album();
    const QString text = album->hasAlbumArtist()
                       ? album->albumArtist()->prettyName() + ": " + album->prettyName()
                       : album->prettyName();

    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), pixmaps, text );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        finish( unit );
        break;
    case KDialog::Rejected: //make sure we do not show any more dialogs
        finish( unit, Cancelled );
        break;
    default:
        finish( unit, Cancelled );
        break;
    }
}

void
CoverFetcher::finish( const CoverFetchUnit::Ptr unit,
                      CoverFetcher::FinishState state,
                      const QString &message )
{
    DEBUG_BLOCK

    Meta::AlbumPtr album = unit->album();
    switch( state )
    {
        case Success:
        {
            The::statusBar()->shortMessage( i18n( "Retrieved cover successfully" ) );
            // album->setImage( unit->selectedPixmap() );
            break;
        }

        case Error:
        {
            m_errors += message;
            debug() << "Album name" << album->name();
        }

        case Cancelled:
        {
            const QString text = i18n( "Cancelled fetching cover for '%1'.", album->name() );
            The::statusBar()->shortMessage( text );
            break;
        }

        case NotFound:
        {
            const QString text = i18n( "Unable to find a cover for %1.", album->name() );
            //FIXME: Not visible behind cover manager
            if( unit->isInteractive() )
                The::statusBar()->longMessage( text, StatusBar::Sorry );
            else
                The::statusBar()->shortMessage( text );

            m_errors += text;
            break;
        }
    }

    emit finishedSingle( static_cast< int >( state ) );
}

#include "CoverFetcher.moc"

