/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "AmazonActions.h"
#include "SvgHandler.h"

#include <KIcon>

AmazonAddToCartAction::AmazonAddToCartAction( const QString &text, Meta::AmazonItem * item )
    : QAction( KIcon("download-amarok" ), text, item ) //XXX: Icon?
    , m_item( item )
{
    setProperty( "popupdropper_svg_id", "append" );
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}

void AmazonAddToCartAction::slotTriggered()
{
    DEBUG_BLOCK
    m_item->addToCart();
}
