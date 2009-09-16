/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "VolumePopupButton.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"

#include "KVBox"

#include <QWidgetAction>
#include <QToolBar>

VolumePopupButton::VolumePopupButton( QWidget * parent )
{
    //create the volume popup
    m_volumeMenu = new QMenu( 0 );

    KVBox * mainBox = new KVBox( 0 );

    m_volumeLabel= new QLabel( mainBox );
    m_volumeLabel->setAlignment( Qt::AlignHCenter );

    KHBox * sliderBox = new KHBox( mainBox );
    m_volumeSlider = new QSlider( Qt::Vertical, sliderBox );
    m_volumeSlider->setMaximum( 100 );
    m_volumeSlider->setFixedHeight( 170 );
    mainBox->setMargin( 0 );
    mainBox->setSpacing( 0 );
    sliderBox->setSpacing( 0 );
    sliderBox->setMargin( 0 );
    mainBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    sliderBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    EngineController* ec = The::engineController();

    QWidgetAction * sliderActionWidget = new QWidgetAction( this );
    sliderActionWidget->setDefaultWidget( mainBox );

    connect( m_volumeSlider, SIGNAL( valueChanged( int ) ), ec, SLOT( setVolume( int ) ) );

    QToolBar *muteBar = new QToolBar( QString(), mainBox );
    muteBar->setContentsMargins( 0, 0, 0, 0 );
    muteBar->setIconSize( QSize( 16, 16 ) );
    m_muteAction = new QAction( KIcon( "audio-volume-muted" ), QString(), 0 );
    m_muteAction->setCheckable ( true );
    m_muteAction->setChecked( ec->isMuted() );

    connect( m_muteAction, SIGNAL( toggled( bool ) ), ec, SLOT( setMuted( bool ) ) );

    m_volumeMenu->addAction( sliderActionWidget );
    muteBar->addAction( m_muteAction );

    //set correct icon and label initially
    engineVolumeChanged( ec->volume() );

    connect( this, SIGNAL( clicked ( bool ) ), this, SLOT( clicked() ) );

}

void VolumePopupButton::engineVolumeChanged( int newVolume )
{
    if ( newVolume < 34 )
        setIcon( KIcon( "audio-volume-low" ) );
    else if ( newVolume < 67 )
        setIcon( KIcon( "audio-volume-medium" ) );
    else
        setIcon( KIcon( "audio-volume-high" ) );

    m_volumeLabel->setText( QString::number( newVolume ) + '%' );
    m_volumeSlider->setValue( newVolume );

    //make sure to uncheck mute toolbar when moving slider
    m_muteAction->setChecked( false );

    setToolTip( i18n( "Volume: %1% %2", newVolume, ( m_muteAction->isChecked() ? i18n( "(muted)" ) : "" ) ) );
}

void VolumePopupButton::engineMuteStateChanged( bool muted )
{
    const int volume = The::engineController()->volume();

    if ( muted )
    {
        setIcon( KIcon( "audio-volume-muted" ) );
        setToolTip( i18n( "Volume: %1% %2", volume, ( muted ? i18n( "(muted)" ) : "" ) ) );
    }
    else
    {
        engineVolumeChanged( volume );
    }

    m_muteAction->setChecked( muted );
}

void VolumePopupButton::clicked()
{
    if ( m_volumeMenu->isVisible() )
        m_volumeMenu->hide();
    else
    {
        QPoint pos( 0, height() );
        m_volumeMenu->exec(  mapToGlobal( pos ) );
    }
}

void VolumePopupButton::wheelEvent( QWheelEvent * event )
{
    DEBUG_BLOCK
    debug() << "delta: " << event->delta();
    event->accept();

    EngineController* const ec = The::engineController();
    int volume = ec->volume();

    volume = qBound( 0, volume + event->delta() / 40 , 100 );
    ec->setVolume( volume );
}
#include "VolumePopupButton.moc"
