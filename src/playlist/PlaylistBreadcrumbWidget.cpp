/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "PlaylistBreadcrumbWidget.h"

namespace Playlist
{

BreadcrumbWidget::BreadcrumbWidget( QWidget *parent )
    : QWidget( parent )
{
    setFixedHeight( 28 );
    setContentsMargins( 3, 0, 3, 0 );

    m_layout = new QHBoxLayout( this );
    setLayout( m_layout );
    m_layout->setSpacing( 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );

    BreadcrumbItemButton *rootItem = new BreadcrumbItemButton( KIcon( "format-list-ordered" ), QString(), this );
    rootItem->setFixedWidth( 20 );
    m_layout->addWidget( rootItem );
    connect( rootItem, SIGNAL( clicked() ), this, SLOT( trimToLevel() ) );

    m_ribbon = new QHBoxLayout( this );
    m_layout->addLayout( m_ribbon );
    m_ribbon->setContentsMargins( 0, 0, 0, 0 );
    m_ribbon->setSpacing( 0 );

    m_addButton = new BreadcrumbAddMenuButton( this );
    m_layout->addWidget( m_addButton );
    m_layout->addStretch( 10 );

    connect( m_addButton, SIGNAL( siblingClicked( QString ) ), this, SLOT( addLevel( QString ) ) );
}

BreadcrumbWidget::~BreadcrumbWidget()
{}

void
BreadcrumbWidget::addLevel( QString internalColumnName, const int level )
{
    if( level > -1 )
        trimToLevel( level );
    BreadcrumbLevel *bLevel = new BreadcrumbLevel( internalColumnName );
    BreadcrumbItem *item = new BreadcrumbItem( bLevel, this );
    m_ribbon->addWidget( item );
    connect( item, SIGNAL( clicked() ), this, SLOT( onItemClicked() ) );
}

void
BreadcrumbWidget::trimToLevel( const int level )
{
    for( int i = m_ribbon->count() - 1 ; i > level; i-- )
    {
        BreadcrumbItem *item = qobject_cast< BreadcrumbItem * >( m_ribbon->itemAt( i )->widget() );
        m_ribbon->removeWidget( item );
        delete item;
    }
}

void
BreadcrumbWidget::onItemClicked()
{
    const int level = m_ribbon->indexOf( qobject_cast< QWidget * >( sender() ) );
    trimToLevel( level );
}

}
