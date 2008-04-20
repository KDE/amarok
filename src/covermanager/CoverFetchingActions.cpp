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
#include <KIcon>
#include <KFile>
#include <KFileDialog>
#include <KLocale>
#include <ksharedptr.h>

#include "CoverFetcher.h"
#include "CoverManager.h"


/////////////////////////////////////
//  FetchCoverAction
/////////////////////////////////////

void FetchCoverAction::init()
{
    setText( i18np("Fetch Cover", "Fetch Covers", m_albums.count()) );
    setIcon( KIcon("list-add") );
    setToolTip( i18np("Fetch the artwork for this album", "Fetch artwork for %1 albums", m_albums.count()) );
    BaseCoverAction::init();
}

void FetchCoverAction::slotTriggered()
{
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
    BaseCoverAction::init();
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
    BaseCoverAction::init();
}

void
UnsetCoverAction::slotTriggered()
{
    foreach( Meta::AlbumPtr album, m_albums )
        album->removeImage();
}

/////////////////////////////////////
//  SetCustomCoverAction
/////////////////////////////////////

void SetCustomCoverAction::init()
{
    setText( i18n("Set Custom Cover") );
    setIcon( KIcon("list-remove") );
    setToolTip( i18np("Set custom artwork for this album", "Set custom artwork for these %1 albums", m_albums.count()) );
    BaseCoverAction::init();
}

void
SetCustomCoverAction::slotTriggered()
{
    QString startPath = m_albums.first()->tracks().first()->playableUrl().directory();
    KUrl file = KFileDialog::getImageOpenUrl( startPath, qobject_cast<QWidget*>( parent() ), i18n( "Select Cover Image File" ) );
    if( !file.isEmpty() )
    {
        QImage image( file.fileName() );
        foreach( Meta::AlbumPtr album, m_albums )
            album->setImage( image );
    }
}
