/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "MainControlsWidget.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "MainControlsButton.h"
#include "SvgHandler.h"

#include <KStandardDirs>

#include <QGraphicsSvgItem>
#include <QGraphicsScene>



MainControlsWidget::MainControlsWidget( QWidget * parent )
    : QGraphicsView( parent )
{
    setFixedHeight( 67 );
    setFixedWidth( 183 );
    setContentsMargins( 0, 0, 0, 0 );

    setFrameStyle( QFrame::NoFrame );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    QGraphicsScene   *scene  = new QGraphicsScene();
    QGraphicsSvgItem *shadow = new QGraphicsSvgItem( KStandardDirs::locate( "data", "amarok/images/default-theme-clean.svg" ), 0 );

    shadow->setElementId( QLatin1String("main_button_shadows") );
    shadow->moveBy( 0.0, 4.0 );
    shadow->setZValue( 1 );
    scene->addItem( shadow );

    MainControlsButton * backButton = new MainControlsButton( 0 );
    backButton->setSvgPrefix( "back_button" );
    backButton->setAction( Amarok::actionCollection()->action( "prev" ) );
    backButton->moveBy( 3.0, 6.5 );
    backButton->setZValue( 2 );
    scene->addItem( backButton );

    m_playPauseButton = new MainControlsButton( 0 );
    m_playPauseButton->setSvgPrefix( "play_button" );
    m_playPauseButton->setAction( Amarok::actionCollection()->action( "play_pause" ) );
    m_playPauseButton->moveBy( 43.0, 6.5 );
    m_playPauseButton->setZValue( 10 );
    scene->addItem( m_playPauseButton );

    MainControlsButton * stopButton = new MainControlsButton( 0 );
    stopButton->setSvgPrefix( "stop_button" );
    stopButton->setAction( Amarok::actionCollection()->action( "stop" ) );
    stopButton->moveBy( 83.0, 6.5 );
    stopButton->setZValue( 5 );
    scene->addItem( stopButton );

    MainControlsButton * nextButton = new MainControlsButton( 0 );
    nextButton->setSvgPrefix( "next_button" );
    nextButton->setAction( Amarok::actionCollection()->action( "next" ) );
    nextButton->moveBy( 123.0, 6.5 );
    nextButton->setZValue( 2 );
    scene->addItem( nextButton );

    setScene( scene );

    show();
}


MainControlsWidget::~MainControlsWidget()
{
}

/* Changes the PlayPause button icon to use the play icon. */
void MainControlsWidget::setPlayButton()
{
    m_playPauseButton->setSvgPrefix( "play_button" );
}

/* Changes the PlayPause button icon to use the pause icon. */
void MainControlsWidget::setPauseButton()
{
    m_playPauseButton->setSvgPrefix( "pause_button" );
}
