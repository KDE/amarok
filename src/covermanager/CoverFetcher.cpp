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

#define DEBUG_PREFIX "CoverFetcher"

#include "CoverFetcher.h"

#include "amarokconfig.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "CoverFetchQueue.h"
#include "CoverFoundDialog.h"
#include "CoverFetchUnit.h"

#include <QBuffer>
#include <QImageReader>
#include <QThread>
#include <QUrl>

#include <KConfigGroup>
#include <KLocalizedString>

#include <functional>
#include <thread>


CoverFetcher* CoverFetcher::s_instance = nullptr;

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
        s_instance = nullptr;
    }
}

CoverFetcher::CoverFetcher()
    : QObject()
{
    DEBUG_BLOCK
    setObjectName( "CoverFetcher" );
    qRegisterMetaType<CoverFetchUnit::Ptr>("CoverFetchUnit::Ptr");

    s_instance = this;

    m_queueThread = new QThread( this );
    m_queueThread->start();
    m_queue = new CoverFetchQueue;
    m_queue->moveToThread( m_queueThread );

    connect( m_queue, &CoverFetchQueue::fetchUnitAdded,
             this, &CoverFetcher::slotFetch );

    connect( The::networkAccessManager(), &NetworkAccessManagerProxy::requestRedirectedReply,
             this, &CoverFetcher::fetchRequestRedirected );
}

CoverFetcher::~CoverFetcher()
{
    m_queue->deleteLater();
    m_queueThread->quit();
    m_queueThread->wait();
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    debug() << QStringLiteral("Adding interactive cover fetch for: '%1' from %2")
        .arg( album->name(),
              Amarok::config("Cover Fetcher").readEntry("Interactive Image Source", "LastFm") );
    switch( fetchSource() )
    {
    case CoverFetch::LastFm:
        QTimer::singleShot( 0, m_queue, [=] () { m_queue->add( album, CoverFetch::Interactive, fetchSource() ); } );
        break;

    case CoverFetch::Discogs:
    case CoverFetch::Google:
        queueQueryForAlbum( album );
        break;

    default:
        break;
    }
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    QTimer::singleShot( 0, m_queue, [=] () { m_queue->add( album, CoverFetch::Automatic ); } );
    debug() << "Queueing automatic cover fetch for:" << album->name();
}

void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    for( Meta::AlbumPtr album : albums )
    {
        QTimer::singleShot( 0, m_queue, [=] () { m_queue->add( album, CoverFetch::Automatic ); } );
    }
}

void
CoverFetcher::queueQuery( const Meta::AlbumPtr &album, const QString &query, int page )
{
    QTimer::singleShot( 0, m_queue, [=] () { m_queue->addQuery( query, fetchSource(), page, album ); } );
    debug() << QString( "Queueing cover fetch query: '%1' (page %2)" ).arg( query, QString::number( page ) );
}

void
CoverFetcher::queueQueryForAlbum( Meta::AlbumPtr album )
{
    QString query( album->name() );
    if( album->hasAlbumArtist() )
        query += ' ' + album->albumArtist()->name();
    queueQuery( album, query, 1 );
}

void
CoverFetcher::slotFetch( CoverFetchUnit::Ptr unit )
{
    if( !unit )
        return;

    const CoverFetchPayload *payload = unit->payload();
    const CoverFetch::Urls urls = payload->urls();

    // show the dialog straight away if fetch is interactive
    if( !m_dialog && unit->isInteractive() )
    {
        showCover( unit, QImage() );
    }
    else if( urls.isEmpty() )
    {
        finish( unit, NotFound );
        return;
    }

    // Was with uniqueKeys, however the payload urls are ever operated only with insert, so there can't be duplicates
    for( const QUrl &url : urls.keys() )
    {
        if( !url.isValid() )
            continue;

        QNetworkReply *reply = The::networkAccessManager()->getData( url, this, &CoverFetcher::slotResult );
        m_urls.insert( url, unit );

        if( payload->type() == CoverFetchPayload::Art )
        {
            if( unit->isInteractive() )
                Amarok::Logger::newProgressOperation( reply, i18n( "Fetching Cover" ) );
            else
                return; // only one is needed when the fetch is non-interactive
        }
    }
}

void
CoverFetcher::slotResult( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    DEBUG_BLOCK
    if( !m_urls.contains( url ) )
        return;
//     debug() << "Data dump from the result: " << data;

    const CoverFetchUnit::Ptr unit( m_urls.take( url ) );
    if( !unit )
    {
        QTimer::singleShot( 0, m_queue, [=] () { m_queue->remove( unit ); } );
        return;
    }

    if( e.code != QNetworkReply::NoError )
    {
        finish( unit, Error, i18n("There was an error communicating with cover provider: %1", e.description) );
        return;
    }

    const CoverFetchPayload *payload = unit->payload();
    switch( payload->type() )
    {
    case CoverFetchPayload::Info:
        QTimer::singleShot( 0, m_queue, [=] () { m_queue->add( unit->album(), unit->options(), payload->source(), data );
                                                 m_queue->remove( unit ); } );
        break;

    case CoverFetchPayload::Search:
        QTimer::singleShot( 0, m_queue, [=] () { m_queue->add( unit->options(), fetchSource(), data );
                                                 m_queue->remove( unit ); } );
        break;

    case CoverFetchPayload::Art:
        handleCoverPayload( unit, data, url );
        break;
    }
}

void
CoverFetcher::handleCoverPayload( const CoverFetchUnit::Ptr &unit, const QByteArray &data, const QUrl &url )
{
    if( data.isEmpty() )
    {
        finish( unit, NotFound );
        return;
    }

    QBuffer buffer;
    buffer.setData( data );
    buffer.open( QIODevice::ReadOnly );
    QImageReader reader( &buffer );
    if( !reader.canRead() )
    {
        finish( unit, Error, reader.errorString() );
        return;
    }

    QSize imageSize = reader.size();
    const CoverFetchArtPayload *payload = static_cast<const CoverFetchArtPayload*>( unit->payload() );
    const CoverFetch::Metadata &metadata = payload->urls().value( url );

    if( payload->imageSize() == CoverFetch::ThumbSize )
    {
        if( imageSize.isEmpty() )
        {
            imageSize.setWidth( metadata.value( QLatin1String("width") ).toInt() );
            imageSize.setHeight( metadata.value( QLatin1String("height") ).toInt() );
        }
        imageSize.scale( 120, 120, Qt::KeepAspectRatio );
        reader.setScaledSize( imageSize );
        // This will force the JPEG decoder to use JDCT_IFAST
        reader.setQuality( 49 );
    }

    if( unit->isInteractive() )
    {
        QImage image;
        if( reader.read( &image ) )
        {
            showCover( unit, image, metadata );
            QTimer::singleShot( 0, m_queue, [=] () {  m_queue->remove( unit ); } );
            return;
        }
    }
    else
    {
        QImage image;
        if( reader.read( &image ) )
        {
            m_selectedImages.insert( unit, image );
            finish( unit );
            return;
        }
    }
    finish( unit, Error, reader.errorString() );
}

void
CoverFetcher::slotDialogFinished()
{
    const CoverFetchUnit::Ptr unit = m_dialog->unit();
    switch( m_dialog->result() )
    {
    case QDialog::Accepted:
        m_selectedImages.insert( unit, m_dialog->image() );
        finish( unit );
        break;

    case QDialog::Rejected:
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
    for( const CoverFetchUnit::Ptr &unit : units )
    {
        if( unit->isInteractive() )
            abortFetch( unit );
    }

    m_dialog->hide();
    m_dialog->deleteLater();
}

void
CoverFetcher::fetchRequestRedirected( QNetworkReply *oldReply,
                                      QNetworkReply *newReply )
{
    QUrl oldUrl = oldReply->request().url();
    QUrl newUrl = newReply->request().url();

    // Since we were redirected we have to check if the redirect
    // was for one of our URLs and if the new URL is not handled
    // already.
    if( m_urls.contains( oldUrl ) && !m_urls.contains( newUrl ) )
    {
        // Get the unit for the old URL.
        CoverFetchUnit::Ptr unit = m_urls.value( oldUrl );

        // Add the unit with the new URL and remove the old one.
        m_urls.insert( newUrl, unit );
        m_urls.remove( oldUrl );

        // If the unit is an interactive one we have to incidate that we're
        // still fetching the cover.
        if( unit->isInteractive() )
            Amarok::Logger::newProgressOperation( newReply, i18n( "Fetching Cover" ) );
    }
}

void
CoverFetcher::showCover( const CoverFetchUnit::Ptr &unit,
                         const QImage &cover,
                         const CoverFetch::Metadata &data )
{
    if( !m_dialog )
    {
        const Meta::AlbumPtr album = unit->album();
        if( !album )
        {
            finish( unit, Error );
            return;
        }

        m_dialog = new CoverFoundDialog( unit, data );
        connect( m_dialog.data(), &CoverFoundDialog::newCustomQuery,
                 this, &CoverFetcher::queueQuery );
        connect( m_dialog.data(), &CoverFoundDialog::accepted,
                 this, &CoverFetcher::slotDialogFinished );
        connect( m_dialog.data(),&CoverFoundDialog::rejected,
                 this, &CoverFetcher::slotDialogFinished );

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
CoverFetcher::abortFetch( const CoverFetchUnit::Ptr &unit )
{
    QTimer::singleShot( 0, m_queue, [=] () {  m_queue->remove( unit ); } );
    m_selectedImages.remove( unit );
    QList<QUrl> urls = m_urls.keys( unit );
    for( const QUrl &url : urls )
        m_urls.remove( url );
    The::networkAccessManager()->abortGet( urls );
}

void
CoverFetcher::finish( const CoverFetchUnit::Ptr &unit,
                      CoverFetcher::FinishState state,
                      const QString &message )
{
    Meta::AlbumPtr album = unit->album();
    const QString albumName = album ? album->name() : QString();

    switch( state )
    {
    case Success:
    {
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Retrieved cover successfully for '%1'.", albumName );
            Amarok::Logger::shortMessage( text );
            debug() << "Finished successfully for album" << albumName;
        }
        QImage image = m_selectedImages.take( unit );
        std::thread thread( std::bind( &Meta::Album::setImage, album, image ) );
        thread.detach();
        abortFetch( unit );
        break;
    }
    case Error:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Fetching cover for '%1' failed.", albumName );
            Amarok::Logger::shortMessage( text );
            QString debugMessage;
            if( !message.isEmpty() )
                debugMessage = QLatin1Char('[') + message + QLatin1Char(']');
            debug() << "Finished with errors for album" << albumName << debugMessage;
        }
        m_errors += message;
        break;

    case Cancelled:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Canceled fetching cover for '%1'.", albumName );
            Amarok::Logger::shortMessage( text );
            debug() << "Finished, cancelled by user for album" << albumName;
        }
        break;

    case NotFound:
        if( !albumName.isEmpty() )
        {
            const QString text = i18n( "Unable to find a cover for '%1'.", albumName );
            //FIXME: Not visible behind cover manager
            Amarok::Logger::shortMessage( text );
            m_errors += text;
            debug() << "Finished due to cover not found for album" << albumName;
        }
        break;
    }

    QTimer::singleShot( 0, m_queue, [=] () { m_queue->remove( unit ); } );

    Q_EMIT finishedSingle( static_cast< int >( state ) );
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
    else
        source = CoverFetch::Discogs;
    return source;
}


