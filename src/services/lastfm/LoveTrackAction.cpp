/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "LoveTrackAction.h"

#include "SvgHandler.h"

#include <KIcon>
#include <KLocale>

LoveTrackAction::LoveTrackAction( LastFmService * service )
    : GlobalCollectionTrackAction( i18n( "Last.fm: Love" ), service )
    , m_service( service )
{
    setIcon( KIcon( "love-amarok") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );

    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}

void LoveTrackAction::slotTriggered()
{
    DEBUG_BLOCK
    m_service->love( track() );
}


