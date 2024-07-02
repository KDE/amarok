/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#define DEBUG_PREFIX "CoverFetchingActions"

#include "CoverFetchingActions.h"

#include "core/support/Debug.h"
#include "MainWindow.h"
#include "CoverFetcher.h"
#include "CoverManager.h"
#include "CoverViewDialog.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QIcon>
#include <QImageReader>
#include <QTemporaryDir>

#include <KConfigGroup>
#include <KDirOperator>
#include <KFile>
#include <KIO/CopyJob>
#include <KLocalizedString>
#include <KMessageBox>

/////////////////////////////////////
//  FetchCoverAction
/////////////////////////////////////

void FetchCoverAction::init()
{
    setText( i18np("Fetch Cover", "Fetch Covers", m_albums.count()) );
    setIcon( QIcon::fromTheme(QStringLiteral("insert-image")) );
    setToolTip( i18np("Fetch the artwork for this album", "Fetch artwork for %1 albums", m_albums.count()) );

    bool enabled = !m_albums.isEmpty();
    for( Meta::AlbumPtr album : m_albums )
    {
        enabled &= album->canUpdateImage();
    }
    setEnabled( enabled );
}

void FetchCoverAction::slotTriggered()
{
    // Queuing multiple albums causes the fetcher to automatically assign values without asking
    // Such as case would be the CoverManager's Fetch All Missing feature
    if( m_albums.size() == 1 )
        CoverFetcher::instance()->manualFetch( m_albums.first() );
    else
        CoverFetcher::instance()->queueAlbums( m_albums );
}


/////////////////////////////////////
//  DisplayCoverAction
/////////////////////////////////////

void DisplayCoverAction::init()
{
    setText( i18n("Display Cover") );
    setIcon( QIcon::fromTheme(QStringLiteral("zoom-original")) );
    setToolTip( i18n("Display artwork for this album") );
    Meta::AlbumPtr album = m_albums.first();
    if( album )
        setEnabled( album->hasImage() );
}

void DisplayCoverAction::slotTriggered()
{
    ( new CoverViewDialog( m_albums.first(), The::mainWindow() ) )->show();
}


/////////////////////////////////////
//  UnsetCoverAction
/////////////////////////////////////

void UnsetCoverAction::init()
{
    setText( i18np("Unset Cover", "Unset Covers", m_albums.count()) );
    setIcon( QIcon::fromTheme(QStringLiteral("list-remove")) );
    setToolTip( i18np("Remove artwork for this album", "Remove artwork for %1 albums", m_albums.count()) );

    // this action is enabled if any one of the albums has an image and can be updated
    bool enabled = false;
    for( Meta::AlbumPtr album : m_albums )
        enabled |= ( album->hasImage() && album->canUpdateImage() );
    setEnabled( enabled );
}

void
UnsetCoverAction::slotTriggered()
{
    int button = KMessageBox::warningContinueCancel( qobject_cast<QWidget*>( parent() ),
                            i18np( "Are you sure you want to remove this cover from the Collection?",
                                  "Are you sure you want to delete these %1 covers from the Collection?",
                                  m_albums.count() ),
                            QString(),
                            KStandardGuiItem::del() );

    if ( button == KMessageBox::Continue )
    {
        for( Meta::AlbumPtr album : m_albums )
        {
            if( album && album->canUpdateImage() )
                album->removeImage();
        }
        qApp->processEvents();
    }
}

/////////////////////////////////////
//  SetCustomCoverAction
/////////////////////////////////////

void SetCustomCoverAction::init()
{
    setText( i18n("Set Custom Cover") );
    setIcon( QIcon::fromTheme(QStringLiteral("document-open")) );
    setToolTip( i18np("Set custom artwork for this album", "Set custom artwork for these %1 albums", m_albums.count()) );

    // this action is enabled if any one of the albums can be updated
    bool enabled = false;

    for( Meta::AlbumPtr album : m_albums )
        if( album )
            enabled |= album->canUpdateImage();

    setEnabled( enabled );
}

void
SetCustomCoverAction::slotTriggered()
{
    if( m_albums.isEmpty() || m_albums.first()->tracks().isEmpty() )
        return;

    const QString& startPath = m_albums.first()->tracks().first()->playableUrl().adjusted(QUrl::RemoveFilename).path();

    const auto mt( QImageReader::supportedMimeTypes() );
    QStringList mimetypes;
    for( const auto &mimetype : mt )
        mimetypes << QLatin1String( mimetype );

    QFileDialog dlg;

    dlg.setDirectory( startPath );
    dlg.setAcceptMode( QFileDialog::AcceptOpen );
    dlg.setFileMode( QFileDialog::ExistingFile );
    dlg.setMimeTypeFilters( mimetypes );
    dlg.setWindowTitle( i18n("Select Cover Image File") );

    dlg.exec();
    QUrl file = dlg.selectedUrls().value( 0 );

    if( !file.isEmpty() )
    {
        QImage image;

        if( file.isLocalFile() )
        {
            image.load( file.path() );
        }
        else
        {
            debug() << "Custom Cover Fetch: " << file.toDisplayString();

            QTemporaryDir tempDir;
            tempDir.setAutoRemove( true );

            const QString coverDownloadPath = tempDir.path() + QLatin1Char('/') + file.fileName();

            auto copyJob = KIO::copy( file, QUrl::fromLocalFile( coverDownloadPath ) );
            bool ret = copyJob->exec();

            if( ret )
                image.load( coverDownloadPath );
        }

        if( !image.isNull() )
        {
            for( Meta::AlbumPtr album : m_albums )
            {
                if( album && album->canUpdateImage() )
                    album->setImage( image );
            }
        }
    }
}


