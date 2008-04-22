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

#include "ActionClasses.h"
#include "Amarok.h"
#include "AnalyzerWidget.h"
#include "ProgressSlider.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include "debug.h"
#include "EngineController.h"
#include "meta/CurrentTrackActionsCapability.h"

#include <KAction>
#include <KToolBar>
#include <KVBox>

#include <QPainter>
#include <QPixmapCache>
#include <QVBoxLayout>
#include <QResizeEvent>

MainToolbar::MainToolbar( QWidget * parent )
 : KHBox( parent )
 , EngineObserver( The::engineController() )
 , SvgHandler()
 , m_addActionsOffsetX( 0 )
{
    loadSvg( "amarok/images/toolbar-background.svg" );

    setMaximumSize( 20000, 67 );
    setMinimumSize( 200, 67 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins(0,0,0,2);
    layout()->setContentsMargins(0,0,0,5);

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
    pWidget->setContentsMargins(0,2,0,0);

    m_playerControlsToolbar = new Amarok::ToolBar( m_insideBox );
    //playerControlsToolbar->setMinimumSize( 180, 45 );
    //m_playerControlsToolbar->setFixedSize( 180, 40 );
    m_playerControlsToolbar->setFixedHeight( 40 );


    //m_insideBox->layout()->setAlignment( playerControlsToolbar, Qt::AlignCenter );
    m_playerControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_playerControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_playerControlsToolbar->setIconDimensions( 32 );
    m_playerControlsToolbar->setMovable( false );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "prev" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "play_pause" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "stop" ) );
    m_playerControlsToolbar->addAction( Amarok::actionCollection()->action( "next" ) );
    m_playerControlsToolbar->adjustSize();


    m_addControlsToolbar = new Amarok::ToolBar( m_insideBox );
    m_addControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_addControlsToolbar->setIconDimensions( 16 );
    m_addControlsToolbar->setMovable( false );
    //m_addControlsToolbar->setFixedSize( 90, 22 );
    m_addControlsToolbar->setFixedHeight( 22 );

    m_volumeWidget = new VolumeWidget( m_insideBox );
    //vw->setMinimumSize( 150, 30 );
    m_volumeWidget->setFixedSize( 160, 24 );

    //m_volumeWidget->setMinimumSize( 230, 30 );

    //m_insideBox->layout()->setAlignment( vw, Qt::AlignRight |  Qt::AlignVCenter);

    //trigger a resize event to get everything laid out
    //resize( m_insideBox->width(), 62 );

    m_renderAddControls = false;
}


MainToolbar::~MainToolbar()
{}

void MainToolbar::paintEvent(QPaintEvent *)
{
    int middle = contentsRect().width() / 2;

    int controlWidth = m_playerControlsToolbar->width();
    int addControlWidth = m_addControlsToolbar->width();
    QRect controlRect( middle - controlWidth / 2, 0, controlWidth, 40 );
    QRect addControlRect( controlRect.bottomRight().x() + 10, 10, addControlWidth, 20 );


    //Meta::TrackPtr track = The::engineController()->currentTrack();
  /*  QString addString;

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
    }*/


    QPainter painter( this );

    QPixmap background = renderSvg( "toolbarbackground", contentsRect().width(), contentsRect().height(), "toolbarbackground" );
    painter.drawPixmap( 0, 0, background );
    QPixmap controlArea = renderSvg( "buttonbar", controlRect.width(), controlRect.height(), "buttonbar" );
    painter.drawPixmap( controlRect.x(), controlRect.y(), controlArea );

    if ( m_renderAddControls ) {
        QPixmap addControlArea = renderSvg( "addbuttonbar", addControlRect.width(), addControlRect.height(), "buttonbar" );
        painter.drawPixmap( addControlRect.x(), addControlRect.y(), addControlArea );
    }
}

void MainToolbar::engineStateChanged(Phonon::State state, Phonon::State oldState)
{
    Q_UNUSED( state ); Q_UNUSED( oldState );
    handleAddActions();
}

void MainToolbar::engineNewMetaData(const QHash< qint64, QString > & newMetaData, bool trackChanged)
{
    Q_UNUSED( newMetaData ); Q_UNUSED( trackChanged );
    handleAddActions();
}

void MainToolbar::handleAddActions()
{

    foreach( QAction * action, m_additionalActions ) {
        m_addControlsToolbar->removeAction( action );
    }

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track ) {
        m_renderAddControls = false;
        return;
    }

    if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
        Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {

            m_additionalActions = cac->customActions();
            int numberOfActions = m_additionalActions.size();

            if ( numberOfActions < 1 ) {
                m_renderAddControls = false;
                return;
            }

            m_renderAddControls = true;

            foreach( QAction *action, m_additionalActions )
                m_addControlsToolbar->addAction( action );

            m_addControlsToolbar->adjustSize();

            //centerAddActions();

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

    int controlWidth = m_playerControlsToolbar->width();
    m_playerControlsToolbar->move( middle - ( controlWidth / 2 ), 0 );
    m_addControlsToolbar->move( middle + ( controlWidth / 2 ) + 10 , 10 );
    m_volumeWidget->move( event->size().width() - 170, /*( m_insideBox->height() - m_volumeWidget->height() ) / 2*/ 0 );
    //centerAddActions();
}

void MainToolbar::paletteChange( const QPalette & oldPalette )
{
    Q_UNUSED( oldPalette );
    reTint();
    repaint( 0, 0, -1, -1 );
}

/*void MainToolbar::centerAddActions()
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


