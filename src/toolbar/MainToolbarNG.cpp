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
#include <QWidgetAction>

MainToolbarNG::MainToolbarNG( QWidget * parent )
: QToolBar( i18n( "Main Toolbar NG" ), parent )
{
    setObjectName( "Main Toolbar NG" );

    setFixedHeight( 32 );
    setIconSize( QSize( 32, 32 ) );

    addAction( Amarok::actionCollection()->action( "prev" ) );
    addAction( Amarok::actionCollection()->action( "play_pause" ) );
    addAction( Amarok::actionCollection()->action( "stop" ) );
    addAction( Amarok::actionCollection()->action( "next" ) );

    m_currentTrackToolbar = new CurrentTrackToolbar( 0 );

    addWidget( m_currentTrackToolbar );

    ProgressWidget *progressWidget = new ProgressWidget( 0 );
    addWidget( progressWidget );

    m_volumeToolButton = new QToolButton( 0 );
    m_volumeToolButton->setIcon( KIcon( "audio-volume-high" ) );
    m_volumeToolButton->setPopupMode( QToolButton::InstantPopup );

    addWidget( m_volumeToolButton );

    //we update the volume icon based on the engine volume
    EngineController* const ec = The::engineController();
    connect( ec, SIGNAL( volumeChanged( int ) ), this, SLOT( engineVolumeChanged( int ) ) );
    connect( ec, SIGNAL( muteStateChanged( bool ) ), this, SLOT( engineMuteStateChanged( bool ) ) );

    //create the volume popup
    m_volumeMenu = new QMenu( 0 );

    KHBox * m_sliderLayout = new KHBox( 0 );
    QSlider * volumeSlider = new QSlider( Qt::Vertical, m_sliderLayout );
    volumeSlider->setMaximum( 100 );
    volumeSlider->setFixedHeight( 170 );
    m_sliderLayout->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );


    QWidgetAction * sliderActionWidget = new QWidgetAction( this );
    sliderActionWidget->setDefaultWidget( m_sliderLayout );
    
    connect( ec, SIGNAL( volumeChanged( int ) ), volumeSlider, SLOT( setValue( int ) ) );
    connect( volumeSlider, SIGNAL( valueChanged( int ) ), ec, SLOT( setVolume( int ) ) );

    m_volumeLabel= new QLabel( 0 );
    QWidgetAction * labelActionWidget = new QWidgetAction( this );
    m_volumeLabel->setAlignment( Qt::AlignHCenter );
    labelActionWidget->setDefaultWidget( m_volumeLabel );
    
    QAction * muteAction = new QAction( KIcon( "audio-volume-muted" ), QString(), 0 );
    muteAction->setCheckable ( true );
    muteAction->setChecked( ec->isMuted() );

    connect( ec, SIGNAL( muteStateChanged( bool ) ), muteAction, SLOT( setChecked( bool ) ) );
    connect( muteAction, SIGNAL( toggled( bool ) ), ec, SLOT( setMuted( bool ) ) );

    m_volumeMenu->addAction( labelActionWidget );
    m_volumeMenu->addAction( sliderActionWidget );
    m_volumeMenu->addAction( muteAction );

    m_volumeToolButton->setMenu( m_volumeMenu );
    m_volumeToolButton->setArrowType( Qt::NoArrow );

    //set correct icon and label initially
    engineVolumeChanged( ec->volume() );
}

MainToolbarNG::~MainToolbarNG()
{
}

void MainToolbarNG::engineVolumeChanged( int newVolume )
{
    if ( newVolume < 34 )
        m_volumeToolButton->setIcon( KIcon( "audio-volume-low" ) );
    else if ( newVolume < 67 )
        m_volumeToolButton->setIcon( KIcon( "audio-volume-medium" ) );
    else
        m_volumeToolButton->setIcon( KIcon( "audio-volume-high" ) );

    m_volumeLabel->setText( QString::number( newVolume ) + "%" );
}

void MainToolbarNG::engineMuteStateChanged( bool muted )
{
    if ( muted )
        m_volumeToolButton->setIcon( KIcon( "audio-volume-muted" ) );
    else
    {
        EngineController* const ec = The::engineController();
        engineVolumeChanged( ec->volume() );
    }
}

#include "MainToolbarNG.moc"

