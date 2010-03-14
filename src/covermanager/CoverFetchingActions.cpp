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

#include "CoverFetchingActions.h"
#include "Debug.h"

#include <QDesktopWidget>

#include <KDirOperator>
#include <KFile>
#include <KFileDialog>
#include <KFileWidget>
#include <KIcon>
#include <KImageIO>
#include <KLocale>
#include <KMessageBox>
#include <KTempDir>
#include <kio/netaccess.h>
#include <ksharedptr.h>

#include "CoverFetcher.h"
#include "CoverManager.h"
#include "CoverViewDialog.h"

/////////////////////////////////////
//  FetchCoverAction
/////////////////////////////////////

void FetchCoverAction::init()
{
    setText( i18np("Fetch Cover", "Fetch Covers", m_albums.count()) );
    setIcon( KIcon("insert-image") );
    setToolTip( i18np("Fetch the artwork for this album", "Fetch artwork for %1 albums", m_albums.count()) );
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
    setIcon( KIcon("zoom-original") );
    setToolTip( i18n("Display artwork for this album") );
    Meta::AlbumPtr album = m_albums.first();
    if( album )
        setEnabled( album->hasImage() );
}

void DisplayCoverAction::slotTriggered()
{
    QWidget *p = dynamic_cast<QWidget*>( parent() );
    int parentScreen = KApplication::desktop()->screenNumber( p );

    ( new CoverViewDialog( m_albums.first(), QApplication::desktop()->screen( parentScreen ) ) )->show();
}


/////////////////////////////////////
//  UnsetCoverAction
/////////////////////////////////////

void UnsetCoverAction::init()
{
    setText( i18np("Unset Cover", "Unset Covers", m_albums.count()) );
    setIcon( KIcon("list-remove") );
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
            kapp->processEvents();
            if( album && album->canUpdateImage() )
                album->removeImage();
        }
    }
}

/////////////////////////////////////
//  SetCustomCoverAction
/////////////////////////////////////

void SetCustomCoverAction::init()
{
    setText( i18n("Set Custom Cover") );
    setIcon( KIcon("document-open") );
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
    if( !m_albums.first() || m_albums.first()->tracks().isEmpty() )
        return;

    const QString& startPath = m_albums.first()->tracks().first()->playableUrl().directory();
    const QStringList mimetypes( KImageIO::mimeTypes( KImageIO::Reading ) );
    KFileDialog dlg( startPath,
                     mimetypes.join(" "),
                     qobject_cast<QWidget*>( parent() ) );

    dlg.setOperationMode( KFileDialog::Opening );
    dlg.setMode( KFile::File );
    dlg.setCaption( i18n("Select Cover Image File") );
    dlg.setInlinePreviewShown( true );

    // TODO: auto error handling was disabled to allow entering an http address
    // in the dialog (commit 8b192500da7d31e314ce04759233d78fe6ce57b9). Now with
    // kde 4.4 and 4.3.3 this is no longer necessary (BR 197945).

    // trueg: there could be another implementation of the file module which does not use KFileWidget!
    if ( KFileWidget *fileWidget = dynamic_cast<KFileWidget*>( dlg.fileWidget() ) )
        if ( KDirLister *dirLister = fileWidget->dirOperator()->dirLister() )
            dirLister->setAutoErrorHandlingEnabled( false, qobject_cast<QWidget*>( parent() ) );

    dlg.exec();
    KUrl file = dlg.selectedUrl();

    if( !file.isEmpty() )
    {
        QPixmap pixmap;

        if( file.isLocalFile() )
        {
            pixmap.load( file.path() );

        }
        else
        {
            debug() << "Custom Cover Fetch: " << file.prettyUrl() << endl;

            KTempDir tempDir;
            tempDir.setAutoRemove( true );

            const QString coverDownloadPath = tempDir.name() + file.fileName();

            bool ret = KIO::NetAccess::file_copy( file,
                                                  KUrl( coverDownloadPath ),
                                                  qobject_cast<QWidget*>( parent() ) );

            if( ret )
                pixmap.load( coverDownloadPath );
        }

        if( !pixmap.isNull() )
        {
            foreach( Meta::AlbumPtr album, m_albums )
            {
                if( album && album->canUpdateImage() )
                    album->setImage( pixmap );
            }
        }
    }
}

#include "CoverFetchingActions.moc"

