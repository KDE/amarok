/****************************************************************************************
 * Copyright (c) 2004-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Kevin Funk <krf@electrostorm.net>                                 *
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

#include "VolumeWidget.h"

#include "core/support/Amarok.h"
#include "amarokconfig.h"
#include "EngineController.h"
#include "SliderWidget.h"

#include <KLocale>
#include <KGlobalSettings>
#include <KStandardDirs>

#include <QHBoxLayout>


VolumeWidget::VolumeWidget( QWidget *parent )
    : Amarok::ToolBar( parent )
{
    setIconDimensions( 16 );
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    setMovable( false );
    setFloatable ( false );
    setContentsMargins( 0, 0, 0, 0 );

    layout()->setSpacing( 0 );

    m_icons << KStandardDirs::locate( "data", "amarok/images/volume_icon.png" );
    m_icons << KStandardDirs::locate( "data", "amarok/images/volume_muted_icon.png" );
    m_action = new KAction( KIcon( m_icons[ AmarokConfig::muteState() ] ), i18n( "Mute" ), this );

    m_slider = new Amarok::VolumeSlider( Amarok::VOLUME_MAX, this );
    m_slider.data()->setObjectName( "ToolBarVolume" );
    m_slider.data()->setValue( AmarokConfig::masterVolume() );
    m_slider.data()->setToolTip( i18n( "Volume: %1%", AmarokConfig::masterVolume() ) );
    m_slider.data()->setMaximumSize( 600000, 20 );

    addAction( m_action );
    addWidget( m_slider.data() );

    EngineController* const ec = The::engineController();

    connect( ec, SIGNAL( volumeChanged( int ) ),
             this, SLOT( volumeChanged( int ) ) );

    connect( ec, SIGNAL( muteStateChanged( bool ) ),
             this, SLOT( muteStateChanged( bool ) ) );

    connect( m_action, SIGNAL( triggered() ), ec, SLOT( toggleMute() ) );
    connect( m_action, SIGNAL( triggered( Qt::MouseButtons, Qt::KeyboardModifiers ) ), SLOT( toggleMute( Qt::MouseButtons, Qt::KeyboardModifiers ) ) );
    connect( m_slider.data(), SIGNAL( sliderMoved( int ) ), ec, SLOT( setVolume( int ) ) );
    connect( m_slider.data(), SIGNAL( sliderReleased( int ) ), ec, SLOT( setVolume( int ) ) );
}

void
VolumeWidget::toggleMute( Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers )
{
    Q_UNUSED( modifiers )

    if( buttons == Qt::MidButton )
        The::engineController()->toggleMute();
}

void
VolumeWidget::volumeChanged( int value )
{
    m_slider.data()->setToolTip( i18n( "Volume: %1%", value ) );

    if( value != m_slider.data()->value() )
        m_slider.data()->setValue( value );
}

void
VolumeWidget::muteStateChanged( bool mute )
{
    if( mute )
        m_slider.data()->setToolTip( i18n( "Volume: <i>Muted</i>" ) );
    else
        m_slider.data()->setToolTip( i18n( "Volume: %1%", m_slider.data()->value() ) );


    m_action->setIcon( KIcon( m_icons[ (bool)mute ] ) );
}

#include "VolumeWidget.moc"
