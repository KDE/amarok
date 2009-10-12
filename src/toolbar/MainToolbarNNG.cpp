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

#include "MainToolbarNNG.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "AnalyzerWidget.h"
#include "Debug.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "MainControlsWidget.h"
#include "ProgressWidget.h"
#include "SvgHandler.h"
#include "VolumePopupButton.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "ToolBar.h"

#include <KApplication>
#include <KVBox>

#include <QResizeEvent>
#include <QVBoxLayout>

MainToolbarNNG::MainToolbarNNG( QWidget * parent )
    : QToolBar( i18n( "Main Toolbar NNG" ), parent )
    , EngineObserver( The::engineController() )
    , m_addActionsOffsetX( 0 )
    , m_ignoreCache( false )
{
    setObjectName( "MainToolbarNNG" );

    setContentsMargins( 0, 0, 0, 0 );
    setFixedHeight( 80 );
    setMinimumWidth( 600 );
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

    setAutoFillBackground ( false );

    KVBox * vBox = new KVBox( this );
    addWidget( vBox );
    vBox->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    vBox->setContentsMargins( 0, 0, 0, 0 );

    QWidget * topBar = new QWidget( vBox );
    QHBoxLayout * layout = new QHBoxLayout( topBar );
    topBar->setLayout( layout );

    layout->addStretch();
    m_mainControlsWidget = new MainControlsWidget( topBar );
    layout->addWidget( m_mainControlsWidget );

    m_addControlsToolbar = new Amarok::ToolBar( topBar );
    m_addControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControlsToolbar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    m_addControlsToolbar->setIconDimensions( 16 );
    m_addControlsToolbar->setMovable( false );
    m_addControlsToolbar->setFloatable ( false );
    m_addControlsToolbar->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_addControlsToolbar );
    layout->addStretch();

    QToolBar *volumeToolBar = new QToolBar( topBar );
    volumeToolBar->setIconSize( QSize( 22, 22 ) );
    volumeToolBar->setContentsMargins( 0, 0, 0, 0 );
    m_volumePopupButton = new VolumePopupButton( this );
    volumeToolBar->addWidget( m_volumePopupButton );
    layout->addWidget( volumeToolBar );

    ProgressWidget *progressWidget = new ProgressWidget( vBox );
    progressWidget->setMinimumSize( 100, 12 );
}

MainToolbarNNG::~MainToolbarNNG()
{
    DEBUG_BLOCK
}

void MainToolbarNNG::engineStateChanged( Phonon::State state, Phonon::State oldState )
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

void MainToolbarNNG::engineNewMetaData( const QHash< qint64, QString > &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData ); Q_UNUSED( trackChanged );
    handleAddActions();
}

void MainToolbarNNG::handleAddActions()
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

            //centerAddActions();
            //m_insideBox->layout()->setAlignment( m_addControlsToolbar, Qt::AlignCenter );
        }
        delete cac;
    }

    foreach( QAction* action, m_additionalActions )
        m_addControlsToolbar->addAction( action );

    repaint ( 0, 0, -1, -1 ); // make sure that the add info area is shown or hidden at once.
}

void MainToolbarNNG::resizeEvent(QResizeEvent *event)
{
    DEBUG_BLOCK

    QWidget::resizeEvent( event );
    //as we handle our own layout, we need to position items correctly

    /*const int middle = event->size().width() / 2;
    const int controlWidth = m_playerControlsToolbar->width();

    m_playerControlsToolbar->move( middle - ( controlWidth / 2 ), 0 );
    m_addControlsToolbar->move( middle + ( controlWidth / 2 ) + 10 , 9 );
    m_volumeWidget->move( event->size().width() - 170, 11 );*/
    //centerAddActions();
}


/*void MainToolbarNNG::centerAddActions()
{
    int numberOfActions = m_additionalActions.size();

    int marginLeft, marginRight, marginTop, marginBottom;
    m_addControlsToolbar->getContentsMargins( &marginLeft, &marginTop, &marginRight, &marginBottom );
    int actionsSize = ( numberOfActions * 24 ) + marginLeft + marginRight + 8;
    m_addActionsOffsetX = ( m_addControlsToolbar->width() - actionsSize ) / 2;
    int middle = contentsRect().width() / 2;

    int controlWidth = m_playerControlsToolbar->width();
    m_addControlsToolbar->move( middle + ( controlWidth / 2 ) + 10 + m_addActionsOffsetX, 10 );
}*/

void MainToolbarNNG::reRender()
{
    m_ignoreCache = true;
    update();
}
