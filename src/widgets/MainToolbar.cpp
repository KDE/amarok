/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "MainToolbar.h"

#include "actionclasses.h" 
#include "amarok.h"
#include "analyzerwidget.h"
#include "SvgTinter.h"
#include "TheInstances.h"



#include "debug.h"
#include "enginecontroller.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "progressslider.h"

#include <KAction>  
#include <KToolBar>
#include <KStandardDirs>
#include <KVBox> 

#include <QPainter>
#include <QPixmapCache>
#include <QVBoxLayout>
#include <QResizeEvent>

MainToolbar::MainToolbar( QWidget * parent )
 : KHBox( parent )
 , EngineObserver( EngineController::instance() )
{

    QString file = KStandardDirs::locate( "data","amarok/images/toolbar-background.svg" );
    
    m_svgRenderer = new QSvgRenderer( The::svgTinter()->tint( file ).toAscii() );
    if ( ! m_svgRenderer->isValid() )
        debug() << "svg is kaputski";

    setMaximumSize( 20000, 62 );
    setMinimumSize( 200, 62 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins(0,0,0,2);
    layout()->setContentsMargins(0,0,0,2);

    KVBox *aVBox     = new KVBox( this );
    aVBox->setMaximumSize( 50000, 60 );
    aVBox->setContentsMargins(0,0,0,0);
    aVBox->layout()->setContentsMargins(0,0,0,0);

    //m_insideBox = new KHBox( aVBox );
    m_insideBox = new QWidget( aVBox );

    
    m_insideBox->setMaximumSize( 600000, 45 );
    m_insideBox->setContentsMargins(0,0,0,0);
   // m_insideBox->layout()->setContentsMargins(0,0,0,0);

    /*AnalyzerWidget *aw = new AnalyzerWidget( m_insideBox );
    //aw->setMinimumSize( 200, 30 );
    aw->setFixedSize( 200, 30 );
    aw->move( 0, 0 );*/

    //m_insideBox->layout()->setAlignment( aw, Qt::AlignLeft );

    ProgressWidget *pWidget = new ProgressWidget( aVBox );
    pWidget->setMinimumSize( 400, 17 );
    pWidget->setMaximumSize( 600000, 17 );

    

    m_playerControlsToolbar = new Amarok::ToolBar( m_insideBox );
    //playerControlsToolbar->setMinimumSize( 180, 45 );
    m_playerControlsToolbar->setFixedSize( 180, 45 );

    //m_insideBox->layout()->setAlignment( playerControlsToolbar, Qt::AlignCenter );
    m_playerControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_playerControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_playerControlsToolbar->setIconDimensions( 32 );
    m_playerControlsToolbar->setMovable( false );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "prev" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "play_pause" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "stop" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "next" ) );


    m_addControlsToolbar = new Amarok::ToolBar( m_insideBox );
    m_addControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_addControlsToolbar->setIconDimensions( 16 );
    m_addControlsToolbar->setMovable( false );
    m_addControlsToolbar->setFixedSize( 90, 22 );

    m_volumeWidget = new VolumeWidget( m_insideBox );
    //vw->setMinimumSize( 150, 30 );
    m_volumeWidget->setFixedSize( 150, 30 );
    
    //m_insideBox->layout()->setAlignment( vw, Qt::AlignRight |  Qt::AlignVCenter);

    //trigger a resize event to get everything laid out
    //resize( m_insideBox->width(), 62 );

}


MainToolbar::~MainToolbar()
{
}

void MainToolbar::paintEvent(QPaintEvent *)
{
    int middle = contentsRect().width() / 2;
    QRect controlRect( middle - 90, 0, 180, 40 );
    QRect addControlRect( controlRect.bottomRight().x() + 10, 10, 90, 20 );


    //Meta::TrackPtr track = EngineController::instance()->currentTrack();
    QString addString;

    if ( m_renderAddControls )
        addString = "-add";

    QString key = QString("toolbar-background%1:%2x%3")
                            .arg( addString )
                            .arg( contentsRect().width() )
                            .arg( contentsRect().height() );

    QPixmap background(contentsRect().width(), contentsRect().height() );

    if (!QPixmapCache::find(key, background)) {
        debug() << QString("toolbar background %1 not in cache...").arg( key );

        QPainter pt( &background );
        m_svgRenderer->render( &pt, "toolbarbackground",  contentsRect() );
        m_svgRenderer->render( &pt, "buttonbar",  controlRect );

        if ( m_renderAddControls )
            m_svgRenderer->render( &pt, "buttonbar",  addControlRect );
        
        QPixmapCache::insert(key, background);
    }


    QPainter painter( this );
    painter.drawPixmap( 0, 0, background );

}

void MainToolbar::engineStateChanged(Engine::State state, Engine::State oldState)
{
    handleAddActions();
}

void MainToolbar::engineNewMetaData(const QHash< qint64, QString > & newMetaData, bool trackChanged)
{
    handleAddActions();
}

void MainToolbar::handleAddActions()
{

    foreach( QAction * action, m_additionalActions ) {
        m_addControlsToolbar->removeAction( action );
    }
    
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track ) {
        m_renderAddControls = false;
        return;
    }

    if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
        Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {

            m_renderAddControls = true;
            m_additionalActions = cac->customActions();

            foreach( QAction *action, m_additionalActions )
                m_addControlsToolbar->addAction( action );
            

            int middle = contentsRect().width() / 2;
            m_addControlsToolbar->move( middle + 100, 10 );
            
            //m_insideBox->layout()->setAlignment( m_addControlsToolbar, Qt::AlignCenter );
            
        } else {
            m_renderAddControls = false;
        }
    } else {
        m_renderAddControls = false;
    }

    repaint ( 0, 0, -1, -1 ); // make sure that the add info area is shown or hidden at once.

    

}

void MainToolbar::resizeEvent(QResizeEvent * event)
{

    DEBUG_BLOCK
            
    QWidget::resizeEvent( event );
    //as we handle our own layout, we need to position items correctly
    
    int middle = event->size().width() / 2;
    
    m_playerControlsToolbar->move( middle - 90, 0 );
    m_volumeWidget->move( event->size().width() - 150, ( m_insideBox->height() - m_volumeWidget->height() ) / 2 );
    m_addControlsToolbar->move( middle + 100, 10 ); //TODO:move a bit depending on how many actions are present so the actions are centered
    //and considder what happens if there are more than 3 actions

    
}


