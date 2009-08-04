/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AmarokUrlAction.h"

AmarokUrlAction::AmarokUrlAction( const QIcon & icon, AmarokUrlPtr url, QObject * parent )
    : QAction( icon, url->name(), parent )
    , m_url( url )
{
    if ( !url->description().isEmpty() )
        setToolTip( url->description() );
    connect( this, SIGNAL( triggered( bool ) ), this, SLOT( run() ) );
}

AmarokUrlAction::AmarokUrlAction( AmarokUrlPtr url, QObject * parent )
    : QAction(url->name(), parent )
    , m_url( url )
{
    if ( !url->description().isEmpty() )
        setToolTip( url->description() );
    connect( this, SIGNAL( triggered( bool ) ), this, SLOT( run() ) );
}


void AmarokUrlAction::run()
{
    m_url->run();
}