/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "MainToolbarNG.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"

#include "widgets/ProgressWidget.h"

#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QSlider>

MainToolbarNG::MainToolbarNG( QWidget * parent )
: QToolBar( i18n( "Main Toolbar NG" ), parent )
{
    setObjectName( "Main Toolbar NG" );

    setFixedHeight( 36 );
    setIconSize( QSize( 32, 32 ) );

    addAction( Amarok::actionCollection()->action( "prev" ) );
    addAction( Amarok::actionCollection()->action( "play_pause" ) );
    addAction( Amarok::actionCollection()->action( "stop" ) );
    addAction( Amarok::actionCollection()->action( "next" ) );

    ProgressWidget *progressWidget = new ProgressWidget( 0 );
    addWidget( progressWidget );

    m_volumeAction = new QAction( KIcon( "audio-volume-high" ), i18n( "Volume" ), 0 );

    addAction( m_volumeAction );

    //we update the volume icon based on the engine volume
    EngineController* const ec = The::engineController();
    connect( ec, SIGNAL( volumeChanged( int ) ), this, SLOT( engineVolumeChanged( int ) ) );

    connect( ec, SIGNAL( muteStateChanged( bool ) ), this, SLOT( engineMuteStateChanged( bool ) ) );

    //create the volume popup

    m_volumeMenu = new QMenu( 0 );
    m_volumeMenu->setMinimumSize( 60, 220 );

    KVBox * menuLayout = new KVBox( m_volumeMenu );

    m_volumeLabel= new QLabel( menuLayout );

    QSlider * volumeSlider = new QSlider( Qt::Vertical, menuLayout );
    volumeSlider->setFixedHeight( 200 );

    connect( ec, SIGNAL( volumeChanged( int ) ), volumeSlider, SLOT( setValue( int ) ) );

    connect( volumeSlider, SIGNAL( valueChanged( int ) ), ec, SLOT( setVolume( int ) ) );

    m_volumeAction->setMenu( m_volumeMenu );

    //set correct icon and label initially
    engineVolumeChanged( ec->volume() );

}


MainToolbarNG::~MainToolbarNG()
{
}

void MainToolbarNG::engineVolumeChanged( int newVolume )
{
    if ( newVolume < 34 )
        m_volumeAction->setIcon( KIcon( "audio-volume-low" ) );
    else if ( newVolume < 67 )
        m_volumeAction->setIcon( KIcon( "audio-volume-medium" ) );
    else
        m_volumeAction->setIcon( KIcon( "audio-volume-high" ) );

    m_volumeLabel->setText( QString::number( newVolume ) + "%" );
}

void MainToolbarNG::engineMuteStateChanged( bool muted )
{
    if ( muted )
        m_volumeAction->setIcon( KIcon( "audio-volume-muted" ) );
    else
    {
        EngineController* const ec = The::engineController();
        engineVolumeChanged( ec->volume() );
    }
}

#include "MainToolbarNG.moc"

