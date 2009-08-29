/****************************************************************************************
 * Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "MainToolbarNG.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"

#include "widgets/ProgressWidget.h"

#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QAction>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QSlider>
#include <QToolButton>
#include <QWidgetAction>

MainToolbarNG::MainToolbarNG( QWidget * parent )
    : QToolBar( i18n( "Main Toolbar NG" ), parent )
    , EngineObserver( The::engineController() )
    , m_currentTrackToolbar( 0 )
    , m_volumeToolButton( 0 )
    , m_volumeLabel( 0 )
    , m_volumeMenu( 0 )
    , m_volumeSlider( 0 )
    , m_muteAction( 0 )
{
    setObjectName( "Main Toolbar NG" );

    setIconSize( QSize( 28, 28 ) );
    layout()->setSpacing( 0 );
    setContentsMargins( 0, 0, 0, 0 );

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

    QToolBar *volumeToolBar = new QToolBar( this );
    volumeToolBar->setIconSize( QSize( 22, 22 ) );
    volumeToolBar->setContentsMargins( 0, 0, 0, 0 );
    volumeToolBar->addWidget( m_volumeToolButton );
    addWidget( volumeToolBar );

    //create the volume popup
    m_volumeMenu = new QMenu( 0 );

    KHBox * m_sliderLayout = new KHBox( 0 );
    m_volumeSlider = new QSlider( Qt::Vertical, m_sliderLayout );
    m_volumeSlider->setMaximum( 100 );
    m_volumeSlider->setFixedHeight( 170 );
    m_sliderLayout->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    EngineController* ec = The::engineController();

    QWidgetAction * sliderActionWidget = new QWidgetAction( this );
    sliderActionWidget->setDefaultWidget( m_sliderLayout );

    connect( m_volumeSlider, SIGNAL( valueChanged( int ) ), ec, SLOT( setVolume( int ) ) );

    m_volumeLabel= new QLabel( 0 );
    QWidgetAction * labelActionWidget = new QWidgetAction( this );
    m_volumeLabel->setAlignment( Qt::AlignHCenter );
    labelActionWidget->setDefaultWidget( m_volumeLabel );

    m_muteAction = new QAction( KIcon( "audio-volume-muted" ), QString(), 0 );
    m_muteAction->setCheckable ( true );
    m_muteAction->setChecked( ec->isMuted() );

    connect( m_muteAction, SIGNAL( toggled( bool ) ), ec, SLOT( setMuted( bool ) ) );

    m_volumeMenu->addAction( labelActionWidget );
    m_volumeMenu->addAction( sliderActionWidget );
    m_volumeMenu->addAction( m_muteAction );

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
    m_volumeSlider->setValue( newVolume );
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
    m_muteAction->setChecked( muted );
}

#include "MainToolbarNG.moc"

