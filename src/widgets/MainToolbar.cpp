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
#include "Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "ProgressSlider.h"
#include "popupdropper/PopupDropperAction.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include <KAction>
#include <KApplication>
#include <KToolBar>
#include <KVBox>

#include <QPainter>
#include <QPixmapCache>
#include <QVBoxLayout>
#include <QResizeEvent>

MainToolbar::MainToolbar( QWidget * parent )
    : KHBox( parent )
    , EngineObserver( The::engineController() )
    , m_addActionsOffsetX( 0 )

{
    setObjectName( "MainToolbar" );

    setMaximumSize( 20000, 67 );
    setMinimumSize( 200, 67 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    layout()->setContentsMargins( 0, 0, 0, 0 );

    KVBox *aVBox     = new KVBox( this );
    aVBox->setMaximumSize( 50000, 60 );
    aVBox->setContentsMargins(0,0,0,0);
    aVBox->layout()->setContentsMargins(0,0,0,0);

    //m_insideBox = new KHBox( aVBox );
    m_insideBox = new QWidget( aVBox );

    m_insideBox->setMaximumSize( 600000, 45 );
    m_insideBox->setContentsMargins( 0, 0, 0, 0 );

    /*AnalyzerWidget *aw = new AnalyzerWidget( m_insideBox );
    //aw->setMinimumSize( 200, 30 );
    aw->setFixedSize( 200, 30 );
    aw->move( 0, 0 );*/

    //m_insideBox->layout()->setAlignment( aw, Qt::AlignLeft );

    ProgressWidget *pWidget = new ProgressWidget( aVBox );
    pWidget->setMinimumSize( 400, 17 );
    pWidget->setMaximumSize( 600000, 17 );
    pWidget->setContentsMargins( 0, 2, 0, 0 );

    m_playerControlsToolbar = new Amarok::ToolBar( m_insideBox );
    m_playerControlsToolbar->setFixedHeight( 40 );

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
    m_addControlsToolbar->setFixedHeight( 22 );

    m_volumeWidget = new VolumeWidget( m_insideBox );
    m_volumeWidget->setFixedSize( 160, 24 );

    m_renderAddControls = false;
    kapp->installEventFilter( this );
}

MainToolbar::~MainToolbar()
{
    DEBUG_BLOCK
}

void MainToolbar::paintEvent( QPaintEvent * )
{
    int middle = contentsRect().width() / 2;

    int controlWidth = m_playerControlsToolbar->width();
    int addControlWidth = m_addControlsToolbar->width();
    QRect controlRect( middle - controlWidth / 2, 0, controlWidth, 40 );
    QRect addControlRect( controlRect.bottomRight().x() + 10, 10, addControlWidth, 20 );

    QSize backgroundSize = MainWindow::self()->backgroundSize();

    //debug() << "Background size: " << backgroundSize;

   //lets cache this or we will be cutting up this big image all the damn time!

    int width = contentsRect().width();
    int height= contentsRect().height();
    
    QString key = QString("toolbar:%1x%2").arg( width, height );
    QPixmap toobarBackground( width, height );
    toobarBackground.fill( Qt::blue );

    if ( !QPixmapCache::find( key, toobarBackground ) ) {

        QPixmap mainBackground = The::svgHandler()->renderSvg( "main_background", backgroundSize.width(), backgroundSize.height(), "context_wallpaper" );
        toobarBackground = mainBackground.copy( 0, 0, width, height );
        QPixmapCache::insert( key, toobarBackground );
    }


    QPainter painter( this );
 
    //paint as much as we need
    painter.drawPixmap( 0, 0, toobarBackground );

    QPixmap controlArea = The::svgHandler()->renderSvg( "buttonbar", controlRect.width(), controlRect.height(), "buttonbar" );
    painter.drawPixmap( controlRect.x(), controlRect.y(), controlArea );

    if ( m_renderAddControls )
    {
        QPixmap addControlArea = The::svgHandler()->renderSvg( "buttonbar", addControlRect.width(), addControlRect.height(), "buttonbar" );
        painter.drawPixmap( addControlRect.x(), addControlRect.y(), addControlArea );
    }
}

void MainToolbar::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( state ); Q_UNUSED( oldState );
    handleAddActions();
}

void MainToolbar::engineNewMetaData( const QHash< qint64, QString > &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData ); Q_UNUSED( trackChanged );
    handleAddActions();
}

void MainToolbar::handleAddActions()
{
    foreach( QAction * action, m_additionalActions )
        m_addControlsToolbar->removeAction( action );

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
    {
        m_renderAddControls = false;
        return;
    }

    if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {

            m_additionalActions = cac->customActions();
            int numberOfActions = m_additionalActions.size();

            if ( numberOfActions < 1 )
            {
                m_renderAddControls = false;
                return;
            }

            m_renderAddControls = true;

            foreach( PopupDropperAction *action, m_additionalActions )
                m_addControlsToolbar->addAction( action );

            m_addControlsToolbar->adjustSize();

            //centerAddActions();

            //m_insideBox->layout()->setAlignment( m_addControlsToolbar, Qt::AlignCenter );

        }
        else
            m_renderAddControls = false;
    }
    else
        m_renderAddControls = false;

    repaint ( 0, 0, -1, -1 ); // make sure that the add info area is shown or hidden at once.
}

void MainToolbar::resizeEvent(QResizeEvent *event)
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

bool MainToolbar::eventFilter( QObject* object, QEvent* event )
{
    // This makes it possible to change volume by using the mouse wheel anywhere on the toolbar
    if( event->type() == QEvent::Wheel && object == this ) {
        kapp->sendEvent( m_volumeWidget->slider(), event );
        return true;
    }

    return QWidget::eventFilter( object, event );
}

void MainToolbar::paletteChange( const QPalette & oldPalette )
{
    Q_UNUSED( oldPalette );
    The::svgHandler()->reTint();
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


