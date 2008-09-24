/***************************************************************************
 *   Copyright (c) 2008  Seb Ruiz <ruiz@kde.org>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CoverFetchingActions.h"

#include <QAction>
#include <KApplication>
#include <KIcon>
#include <KFile>
#include <KFileDialog>
#include <KLocale>
#include <KMessageBox>
#include <ksharedptr.h>

#include "CoverFetcher.h"
#include "CoverManager.h"

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
}

void DisplayCoverAction::slotTriggered()
{
    ( new CoverViewDialog( m_albums.first(), qobject_cast<QWidget*>( parent() ) ) )->show();
}


/////////////////////////////////////
//  UnsetCoverAction
/////////////////////////////////////

void UnsetCoverAction::init()
{
    setText( i18np("Unset Cover", "Unset Covers", m_albums.count()) );
    setIcon( KIcon("list-remove") );
    setToolTip( i18np("Remove artwork for this album", "Remove artwork for %1 albums", m_albums.count()) );
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
            if( album->canUpdateImage() )
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
    setIcon( KIcon("document-open-folder") );
    setToolTip( i18np("Set custom artwork for this album", "Set custom artwork for these %1 albums", m_albums.count()) );
}

void
SetCustomCoverAction::slotTriggered()
{
    QString startPath = m_albums.first()->tracks().first()->playableUrl().directory();
    KUrl file = KFileDialog::getImageOpenUrl( startPath, qobject_cast<QWidget*>( parent() ), i18n( "Select Cover Image File" ) );
    if( !file.isEmpty() )
    {
        QImage image( file.path() );

        foreach( Meta::AlbumPtr album, m_albums )
            album->setImage( image );
    }
}

#include "CoverFetchingActions.moc"
