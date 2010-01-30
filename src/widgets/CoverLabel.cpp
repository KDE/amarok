/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverLabel.h"

CoverLabel::CoverLabel( QWidget * parent, Qt::WindowFlags f )
        : QLabel( parent, f)
{}

void CoverLabel::setInformation( const QString &artist, const QString &album )
{
    m_artist = artist;
    m_album = album;
}

void CoverLabel::mouseReleaseEvent( QMouseEvent *pEvent )
{
    if( pEvent->button() == Qt::LeftButton || pEvent->button() == Qt::RightButton )
    {
//         Amarok::coverContextMenu( this, pEvent->globalPos(), m_albumPtr, false );
    }
}

