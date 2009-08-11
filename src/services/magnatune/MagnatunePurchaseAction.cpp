/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "MagnatunePurchaseAction.h"
#include "SvgHandler.h"

#include <KIcon>

MagnatunePurchaseAction::MagnatunePurchaseAction( const QString &text, Meta::MagnatuneAlbum * album )
    : QAction( KIcon("download-amarok" ), text, album )
    , m_album( album )
{
    setProperty( "popupdropper_svg_id", "append" );
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}


MagnatunePurchaseAction::~MagnatunePurchaseAction()
{
}

void MagnatunePurchaseAction::slotTriggered()
{
    DEBUG_BLOCK
    m_album->purchase();
}





MagnatuneAddToFavoritesAction::MagnatuneAddToFavoritesAction( const QString &text, Meta::MagnatuneAlbum * album )
    : QAction( KIcon("download-amarok" ), text, album )
    , m_album( album )
{
    setProperty( "popupdropper_svg_id", "append" );
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}


MagnatuneAddToFavoritesAction::~MagnatuneAddToFavoritesAction()
{
}

void MagnatuneAddToFavoritesAction::slotTriggered()
{
    DEBUG_BLOCK
    m_album->addToFavorites();
}

#include "MagnatunePurchaseAction.moc"



