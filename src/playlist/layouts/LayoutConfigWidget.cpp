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
#include "dialogs/PlaylistLayoutEditDialog.h"
#include "widgets/EditDeleteDelegate.h"
#include "widgets/EditDeleteComboBoxView.h"

#include <QLabel>
#include <QComboBox>

namespace Playlist
{

LayoutConfigWidget::LayoutConfigWidget( QWidget * parent )
    : KHBox( parent )
    , m_playlistEditDialog( 0 )
{
    m_comboBox = new QComboBox( this );
    m_configButton = new KPushButton( this );

    m_comboBox->addItems( LayoutManager::instance()->layouts() );
    int index = LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() );
    m_comboBox->setCurrentIndex( index );
    m_comboBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    connect( m_comboBox, SIGNAL( currentIndexChanged ( const QString ) ), this, SLOT( setActiveLayout(const QString & ) ) );

    connect( LayoutManager::instance(), SIGNAL( layoutListChanged() ), this, SLOT( layoutListChanged() ) );
    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ), this, SLOT( onActiveLayoutChanged() ) );


    m_configButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    const KIcon configIcon( "configure" );
    m_configButton->setIcon( configIcon );

    connect( m_configButton, SIGNAL( clicked() ), this, SLOT( configureLayouts() ) );
}


LayoutConfigWidget::~LayoutConfigWidget()
{
}

void LayoutConfigWidget::setActiveLayout( const QString &layout )
{
    LayoutManager::instance()->setActiveLayout( layout );
}

void LayoutConfigWidget::configureLayouts()
{
    if ( !m_playlistEditDialog )
        m_playlistEditDialog = new PlaylistLayoutEditDialog( this );
    m_playlistEditDialog->show();
}

void Playlist::LayoutConfigWidget::layoutListChanged()
{
    m_comboBox->clear();
    m_comboBox->addItems( LayoutManager::instance()->layouts() );
}

void LayoutConfigWidget::onActiveLayoutChanged()
{
    int index = LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() );
    if( index != m_comboBox->currentIndex() )
        m_comboBox->setCurrentIndex( index );
}


}

#include "LayoutConfigWidget.moc"
