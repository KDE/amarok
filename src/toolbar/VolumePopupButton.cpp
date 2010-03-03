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
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "SliderWidget.h"

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
    m_volumeMenu = new QMenu( 0 );

    KVBox * mainBox = new KVBox( 0 );

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

    connect( m_volumeSlider, SIGNAL( sliderMoved( int ) ), ec, SLOT( setVolume( int ) ) );
    connect( m_volumeSlider, SIGNAL( sliderReleased( int ) ), ec, SLOT( setVolume( int ) ) );

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

    // For filtering Wheel events
    m_volumeSlider->installEventFilter( this );
}

void
VolumePopupButton::engineVolumeChanged( int newVolume )
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

    setToolTip( i18n( "Volume: %1% %2", newVolume, ( m_muteAction->isChecked() ? i18n( "(muted)" ) : "" ) ) );
}

void
VolumePopupButton::engineMuteStateChanged( bool muted )
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

bool
VolumePopupButton::eventFilter( QObject *object, QEvent *event )
{
    if( event->type() == QEvent::Wheel )
    {
        QWheelEvent* mackintosh = static_cast<QWheelEvent*>( event );
        Amarok::VolumeSlider* slider = qobject_cast<Amarok::VolumeSlider*>( object );
        if( slider )
        {
            // TODO:
            // This is a liiiiiiitle bit of a hack, but I can't be bothered to fix up
            // the logic in Amarok::Slider right now, so we invert the delta of the QWheelEvent.

            QWheelEvent* hackintosh;
            hackintosh = new QWheelEvent( mackintosh->pos(), mackintosh->globalPos(), -mackintosh->delta(),
                                          mackintosh->buttons(), mackintosh->modifiers(), mackintosh->orientation() );

            slider->wheelEvent( hackintosh );
            delete hackintosh;

            return true;
        }
    }

    return QToolButton::eventFilter( object, event );
}


#include "VolumePopupButton.moc"
