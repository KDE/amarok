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
 
#include "MainControlsWidget.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "SvgHandler.h"

#include <QPainter>



MainControlsWidget::MainControlsWidget( QWidget * parent )
    : QWidget( parent )
{

    setFixedHeight( 54 );
    setFixedWidth( 168 );
    setContentsMargins( 0, 0, 0, 0 );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    setStyleSheet( "QToolButton { border: none }" );

    QToolButton * m_prevButton = new QToolButton( this );
    QToolButton * m_playButton = new QToolButton( this );
    QToolButton * m_stopButton = new QToolButton( this );
    QToolButton * m_nextButton = new QToolButton( this );

    m_prevButton->setDefaultAction( Amarok::actionCollection()->action( "prev" ) );
    m_playButton->setDefaultAction( Amarok::actionCollection()->action( "play_pause" ) );
    m_stopButton->setDefaultAction( Amarok::actionCollection()->action( "stop" ) );
    m_nextButton->setDefaultAction( Amarok::actionCollection()->action( "next" ) );
    m_prevButton->setIconSize( QSize( 48, 48 ) );
    m_playButton->setIconSize( QSize( 48, 48 ) );
    m_stopButton->setIconSize( QSize( 48, 48 ) );
    m_nextButton->setIconSize( QSize( 48, 48 ) );

    
    m_playButton->setAutoFillBackground( false );
    
    m_stopButton->raise();
    m_playButton->raise();

    m_prevButton->setGeometry( QRect( 6, 3, 48, 48 ) );
    m_playButton->setGeometry( QRect( 42, 3, 48, 48 ) );
    m_stopButton->setGeometry( QRect( 78, 3, 48, 48 ) );
    m_nextButton->setGeometry( QRect( 114, 3, 48, 48 ) );
}


MainControlsWidget::~MainControlsWidget()
{
}

void MainControlsWidget::paintEvent(QPaintEvent *)
{
    QPainter painter( this );
    painter.drawPixmap( 3, 0, The::svgHandler()->renderSvg( "main_button_shadows", width() - 6, height(), "main_button_shadows" ) );
}


