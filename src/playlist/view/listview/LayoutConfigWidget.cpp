/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "LayoutConfigWidget.h"

#include "Debug.h"
#include "LayoutManager.h"
#include "widgets/EditDeleteDelegate.h"
#include "widgets/EditDeleteComboBoxView.h"

#include <QLabel>
#include <QComboBox>

namespace Playlist {

LayoutConfigWidget::LayoutConfigWidget( QWidget * parent )
    : KVBox( parent )
{
    new QLabel( "Config gui goes here....", this );
    QComboBox *comboBox = new QComboBox( this );
    EditDeleteComboBoxView * comboView = new EditDeleteComboBoxView( comboBox );
    comboView->setModel( comboBox->model() );
    comboBox->setView( comboView );
    comboBox->setItemDelegate( new EditDeleteDelegate( comboBox ) );

    connect( comboView, SIGNAL( editItem( const QString & ) ), this, SLOT( editItem( const QString & ) ) );
    connect( comboView, SIGNAL( deleteItem( const QString & ) ), this, SLOT( deleteItem( const QString & ) ) );

    comboBox->addItems( LayoutManager::instance()->layouts() );

    connect( comboBox, SIGNAL( currentIndexChanged ( const QString ) ), this, SLOT( setActiveLayout(const QString & ) ) );

}


LayoutConfigWidget::~LayoutConfigWidget()
{
}


}

void Playlist::LayoutConfigWidget::setActiveLayout( const QString & layout )
{
    LayoutManager::instance()->setActiveLayout( layout );
}

void Playlist::LayoutConfigWidget::editItem( const QString &itemName )
{
    DEBUG_BLOCK
    debug() << "edit item: " << itemName;
}

void Playlist::LayoutConfigWidget::deleteItem( const QString &itemName )
{
    DEBUG_BLOCK
    debug() << "delete item: " << itemName;
}

#include "LayoutConfigWidget.moc"
