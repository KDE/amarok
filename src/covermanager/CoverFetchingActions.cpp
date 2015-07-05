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

#include <KApplication>
#include <KDirOperator>
#include <KFile>
#include <KFileDialog>
#include <KFileWidget>
#include <QIcon>
#include <KImageIO>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTempDir>
#include <KIO/NetAccess>

#include <QDesktopWidget>
#include <KConfigGroup>

/////////////////////////////////////
//  FetchCoverAction
/////////////////////////////////////

void FetchCoverAction::init()
{
    setText( i18np("Fetch Cover", "Fetch Covers", m_albums.count()) );
    setIcon( QIcon::fromTheme("insert-image") );
    setToolTip( i18np("Fetch the artwork for this album", "Fetch artwork for %1 albums", m_albums.count()) );

    bool enabled = !m_albums.isEmpty();
    foreach( Meta::AlbumPtr album, m_albums )
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
    setIcon( QIcon::fromTheme("zoom-original") );
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
    setIcon( QIcon::fromTheme("list-remove") );
    setToolTip( i18np("Remove artwork for this album", "Remove artwork for %1 albums", m_albums.count()) );

    // this action is enabled if any one of the albums has an image and can be updated
    bool enabled = false;
    foreach( Meta::AlbumPtr album, m_albums )
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
        foreach( Meta::AlbumPtr album, m_albums )
        {
            if( album && album->canUpdateImage() )
                album->removeImage();
        }
        kapp->processEvents();
    }
}

/////////////////////////////////////
//  SetCustomCoverAction
/////////////////////////////////////

void SetCustomCoverAction::init()
{
    setText( i18n("Set Custom Cover") );
    setIcon( QIcon::fromTheme("document-open") );
    setToolTip( i18np("Set custom artwork for this album", "Set custom artwork for these %1 albums", m_albums.count()) );

    // this action is enabled if any one of the albums can be updated
    bool enabled = false;

    foreach( Meta::AlbumPtr album, m_albums )
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

    const QStringList mimetypes( KImageIO::mimeTypes( KImageIO::Reading ) );
    KFileDialog dlg( startPath,
                     mimetypes.join(" "),
                     qobject_cast<QWidget*>( parent() ) );

    dlg.setOperationMode( KFileDialog::Opening );
    dlg.setMode( KFile::File );
    dlg.setWindowTitle( i18n("Select Cover Image File") );
    dlg.setInlinePreviewShown( true );

    dlg.exec();
    QUrl file = dlg.selectedUrl();

    if( !file.isEmpty() )
    {
        QImage image;

        if( file.isLocalFile() )
        {
            image.load( file.path() );
        }
        else
        {
            debug() << "Custom Cover Fetch: " << file.toDisplayString() << endl;

            KTempDir tempDir;
            tempDir.setAutoRemove( true );

            const QString coverDownloadPath = tempDir.name() + file.fileName();

            bool ret = KIO::NetAccess::file_copy( file,
                                                  QUrl( coverDownloadPath ),
                                                  qobject_cast<QWidget*>( parent() ) );

            if( ret )
                image.load( coverDownloadPath );
        }

        if( !image.isNull() )
        {
            foreach( Meta::AlbumPtr album, m_albums )
            {
                if( album && album->canUpdateImage() )
                    album->setImage( image );
            }
        }
    }
}


