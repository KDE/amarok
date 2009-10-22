/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "MainToolbar.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "AnalyzerWidget.h"
#include "Debug.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "MainControlsWidget.h"
#include "ProgressWidget.h"
#include "SvgHandler.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "ToolBar.h"
#include "VolumeWidget.h"

#include <KApplication>
#include <KVBox>

#include <QResizeEvent>
#include <QVBoxLayout>


MainToolbar::MainToolbar( QWidget * parent )
    : QToolBar( i18n( "Main Toolbar" ), parent )
    , EngineObserver( The::engineController() )
    , m_ignoreCache( false )
{
    setObjectName( "Main Toolbar" );

    setContentsMargins( 0, 0, 0, 0 );
    setFixedHeight( 80 );
    setMinimumWidth( 600 );
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

    setAutoFillBackground ( false );

    KVBox * mainBox = new KVBox( this );
    mainBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    addWidget( mainBox );

    m_insideBox = new QWidget( mainBox );
    m_insideBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_insideBox->setContentsMargins( 0, 0, 0, 0 );

    m_mainControlsWidget = new MainControlsWidget( m_insideBox );

    m_addControlsToolbar = new Amarok::ToolBar( m_insideBox );
    m_addControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControlsToolbar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_addControlsToolbar->setIconDimensions( 16 );
    m_addControlsToolbar->setMovable( false );
    m_addControlsToolbar->setFloatable ( false );
    m_addControlsToolbar->setContentsMargins( 0, 0, 0, 0 );

    m_volumeWidget = new VolumeWidget( m_insideBox );

    KHBox * progressBox = new KHBox( mainBox );
    progressBox->setContentsMargins( 0, 0, 0, 4 );
    ProgressWidget *progressWidget = new ProgressWidget( progressBox );
    progressWidget->setMinimumSize( 100, 12 );

    centerAddActions();
    installEventFilter( this );
}

MainToolbar::~MainToolbar()
{
    DEBUG_BLOCK
}

void
MainToolbar::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    if( (state != oldState) )
    {
        if( (state == Phonon::StoppedState) || (state == Phonon::PausedState) )
            m_mainControlsWidget->setPlayButton();
        else if( state == Phonon::PlayingState )
            m_mainControlsWidget->setPauseButton();
     }

    handleAddActions();
}

void
MainToolbar::engineNewMetaData( const QHash< qint64, QString > &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData ); Q_UNUSED( trackChanged );
    handleAddActions();
}

void
MainToolbar::handleAddActions()
{
    foreach( QAction* action, m_additionalActions )
        m_addControlsToolbar->removeAction( action );

    m_additionalActions.clear();

    Meta::TrackPtr track = The::engineController()->currentTrack();

    m_additionalActions.clear();
    foreach( QAction* action, The::globalCurrentTrackActions()->actions() )
        m_addControlsToolbar->addAction( action );
    
    if ( track && track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = track->create<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                m_additionalActions.append( action );

            m_addControlsToolbar->adjustSize();

            centerAddActions();
        }
        delete cac;
    }

    foreach( QAction* action, m_additionalActions )
        m_addControlsToolbar->addAction( action );

    m_addControlsToolbar->adjustSize();

    repaint ( 0, 0, -1, -1 ); // make sure that the add info area is shown or hidden at once.
}

void
MainToolbar::resizeEvent( QResizeEvent *event )
{
    DEBUG_BLOCK

    QWidget::resizeEvent( event );
    //as we handle our own layout, we need to position items correctly

    const int middle = event->size().width() / 2;
    const int controlWidth = m_mainControlsWidget->width();

    m_mainControlsWidget->move( middle - ( controlWidth / 2 ), 3 );
    m_volumeWidget->move( event->size().width() - 172, 11 );
    centerAddActions();
}

void
MainToolbar::centerAddActions()
{
    int marginLeft, marginRight, marginTop, marginBottom;
    m_addControlsToolbar->getContentsMargins( &marginLeft, &marginTop, &marginRight, &marginBottom );
    const int middle = contentsRect().width() / 2;
    const int controlWidth = m_mainControlsWidget->width();

    m_addControlsToolbar->move( middle + ( controlWidth / 2 ) + 3, 10 );
}

bool
MainToolbar::eventFilter( QObject* object, QEvent* event )
{
    // This makes it possible to change volume by using the mouse wheel anywhere on the toolbar
    if( event->type() == QEvent::Wheel && object == this )
    {
        kapp->sendEvent( m_volumeWidget->slider(), event );
        return true;
    }

    return QToolBar::eventFilter( object, event );
}

void
MainToolbar::reRender()
{
    m_ignoreCache = true;
    update();
}
