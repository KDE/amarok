/***************************************************************************
*   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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

#include "PlaylistSortWidget.h"

#include "playlist/proxymodels/SortProxy.h"
#include "Debug.h"

#include <KPushButton>

namespace Playlist
{

SortWidget::SortWidget( QWidget *parent ) : KHBox( parent )
{
    DEBUG_BLOCK
    m_sortCombo = new KComboBox( this );
    KPushButton *btnSort = new KPushButton( "Just sort it!", this );


    m_sortCombo->addItem( "Artist/Album/Track add" );
    m_sortCombo->addItem( "Artist/Title da" );

    connect(btnSort, SIGNAL( clicked() ), this, SLOT( applySortingScheme() ) );
}

void
SortWidget::applySortingScheme()
{
    DEBUG_BLOCK
    SortScheme *schemeAaAdTd = new SortScheme();
    schemeAaAdTd->addLevel( SortLevel( Artist, Qt::AscendingOrder ) );
    schemeAaAdTd->addLevel( SortLevel( Album, Qt::DescendingOrder ) );
    schemeAaAdTd->addLevel( SortLevel( TrackNumber, Qt::DescendingOrder ) );
    SortScheme *schemeAdTa = new SortScheme();
    schemeAdTa->addLevel( SortLevel( Artist, Qt::DescendingOrder ) );
    schemeAdTa->addLevel( SortLevel( Title, Qt::AscendingOrder ) );

    if( m_sortCombo->currentText() == "Artist/Album/Track add" )
        SortProxy::instance()->updateSortMap( schemeAaAdTd );
    if( m_sortCombo->currentText() =="Artist/Title da" )
        SortProxy::instance()->updateSortMap( schemeAdTa );
}


}   //namespace Playlist