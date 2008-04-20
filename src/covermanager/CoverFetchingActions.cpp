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

FetchCoverAction::FetchCoverAction( QObject *parent, Meta::Album *album )
    : QAction( parent )
    , m_album( album )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Fetch Cover") );
    setIcon( KIcon("list-add") );

    setToolTip( i18n("Fetch the artwork for this album") );
}

void
FetchCoverAction::slotTriggered()
{
    CoverFetcher::instance()->manualFetch( KSharedPtr<Meta::Album>( m_album ) );
}


/////////////////////////////////////
//  DisplayCoverAction
/////////////////////////////////////

DisplayCoverAction::DisplayCoverAction( QObject *parent, Meta::Album *album )
    : QAction( parent )
    , m_album( album )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Display Cover") );
    setIcon( KIcon("zoom-original") );

    setToolTip( i18n("Display artwork for this album") );
}

void
DisplayCoverAction::slotTriggered()
{
    ( new CoverViewDialog( KSharedPtr<Meta::Album>( m_album ), qobject_cast<QWidget*>( parent() ) ) )->show();
}


/////////////////////////////////////
//  UnsetCoverAction
/////////////////////////////////////

UnsetCoverAction::UnsetCoverAction( QObject *parent, Meta::Album *album )
    : QAction( parent )
    , m_album( album )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Unset Cover") );
    setIcon( KIcon("list-remove") );

    setToolTip( i18n("Remove artwork for this album") );
}

void
UnsetCoverAction::slotTriggered()
{
    m_album->removeImage();
}

/////////////////////////////////////
//  SetCustomCoverAction
/////////////////////////////////////

SetCustomCoverAction::SetCustomCoverAction( QObject *parent, Meta::Album *album )
    : QAction( parent )
    , m_album( album )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setText( i18n("Set Custom Cover") );
    setIcon( KIcon("list-remove") );

    setToolTip( i18n("Set custom artwork for this album") );
}

void
SetCustomCoverAction::slotTriggered()
{
    QString startPath = m_album->tracks().first()->playableUrl().directory();
    KUrl file = KFileDialog::getImageOpenUrl( startPath, qobject_cast<QWidget*>( parent() ), i18n( "Select Cover Image File" ) );
    if( !file.isEmpty() )
    {
        QImage image( file.fileName() );
        m_album->setImage( image );
    }
}
