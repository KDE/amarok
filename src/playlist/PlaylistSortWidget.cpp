/****************************************************************************************
 * Copyright (c) 2009 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "PlaylistSortWidget.h"

#include "Debug.h"

#include <KPushButton>

namespace Playlist
{

SortWidget::SortWidget( QWidget *parent ) : KHBox( parent )
{
    DEBUG_BLOCK
    m_sortCombo = new KComboBox( this );
    KPushButton *btnSort = new KPushButton( "Just sort it!", this );

    m_sortCombo->addItem( "ArtistA/AlbumD/TrackD" );
    m_schemeList.append( new SortScheme() );
    m_schemeList.last()->addLevel( SortLevel( Artist, Qt::AscendingOrder ) );
    m_schemeList.last()->addLevel( SortLevel( Album, Qt::DescendingOrder ) );
    m_schemeList.last()->addLevel( SortLevel( TrackNumber, Qt::DescendingOrder ) );

    m_sortCombo->addItem( "ArtistD/TitleA" );
    m_schemeList.append( new SortScheme() );
    m_schemeList.last()->addLevel( SortLevel( Artist, Qt::DescendingOrder ) );
    m_schemeList.last()->addLevel( SortLevel( Title, Qt::AscendingOrder ) );

    connect(btnSort, SIGNAL( clicked() ), this, SLOT( applySortingScheme() ) );
}

void
SortWidget::applySortingScheme()
{
    DEBUG_BLOCK
    SortProxy::instance()->updateSortMap( m_schemeList[ m_sortCombo->currentIndex() ]  );
}


}   //namespace Playlist
