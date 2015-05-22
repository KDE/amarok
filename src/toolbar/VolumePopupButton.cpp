/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "EngineController.h"
#include "core/support/Amarok.h"
#include "widgets/SliderWidget.h"

#include <KLocale>
#include <KVBox>

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolBar>
#include <QWheelEvent>
#include <QWidgetAction>


VolumePopupButton::VolumePopupButton( QWidget * parent )
    : QToolButton( parent )
{
    //create the volume popup
    m_volumeMenu = new QMenu( this );

    KVBox * mainBox = new KVBox( this );

    m_volumeLabel= new QLabel( mainBox );
    m_volumeLabel->setAlignment( Qt::AlignHCenter );

    KHBox * sliderBox = new KHBox( mainBox );
    m_volumeSlider = new Amarok::VolumeSlider( Amarok::VOLUME_MAX, sliderBox, false );
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

    connect( m_volumeSlider, SIGNAL(sliderMoved(int)), ec, SLOT(setVolume(int)) );
    connect( m_volumeSlider, SIGNAL(sliderReleased(int)), ec, SLOT(setVolume(int)) );

    QToolBar *muteBar = new QToolBar( QString(), mainBox );
    muteBar->setContentsMargins( 0, 0, 0, 0 );
    muteBar->setIconSize( QSize( 16, 16 ) );
    m_muteAction = new QAction( KIcon( "audio-volume-muted" ), QString(), muteBar );
    m_muteAction->setCheckable ( true );
    m_muteAction->setChecked( ec->isMuted() );

    connect( m_muteAction, SIGNAL(toggled(bool)), ec, SLOT(setMuted(bool)) );

    m_volumeMenu->addAction( sliderActionWidget );
    muteBar->addAction( m_muteAction );

    //set correct icon and label initially
    volumeChanged( ec->volume() );

    connect( ec, SIGNAL(volumeChanged(int)),
             this, SLOT(volumeChanged(int)) );

    connect( ec, SIGNAL(muteStateChanged(bool)),
             this, SLOT(muteStateChanged(bool)) );

}

void
VolumePopupButton::volumeChanged( int newVolume )
{
    if ( newVolume < 34 )
        setIcon( KIcon( "audio-volume-low" ) );
    else if ( newVolume < 67 )
        setIcon( KIcon( "audio-volume-medium" ) );
    else
        setIcon( KIcon( "audio-volume-high" ) );

    m_volumeLabel->setText( QString::number( newVolume ) + '%' );

    if( newVolume != m_volumeSlider->value() )
        m_volumeSlider->setValue( newVolume );

    //make sure to uncheck mute toolbar when moving slider
    if ( newVolume )
        m_muteAction->setChecked( false );

    const KLocalizedString tip = m_muteAction->isChecked() ? ki18n( "Volume: %1% (muted)" ) : ki18n( "Volume: %1%" );
    setToolTip( tip.subs( newVolume ).toString() );
}

void
VolumePopupButton::muteStateChanged( bool muted )
{
    const int volume = The::engineController()->volume();

    if ( muted )
    {
        setIcon( KIcon( "audio-volume-muted" ) );
        setToolTip( i18n( "Volume: %1% (muted)", volume ) );
    }
    else
    {
        volumeChanged( volume );
    }

    m_muteAction->setChecked( muted );
}

void
VolumePopupButton::mouseReleaseEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
    {
        if ( m_volumeMenu->isVisible() )
            m_volumeMenu->hide();
        else
        {
            const QPoint pos( 0, height() );
            m_volumeMenu->exec( mapToGlobal( pos ) );
        }
    }
    else if( event->button() == Qt::MidButton )
    {
        The::engineController()->toggleMute();
    }

    QToolButton::mouseReleaseEvent( event );
}

void
VolumePopupButton::wheelEvent( QWheelEvent * event )
{
    //debug() << "delta: " << event->delta();
    event->accept();

    EngineController* const ec = The::engineController();

    const int volume = qBound( 0, ec->volume() + event->delta() / 40 , 100 );
    ec->setVolume( volume );
}


