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
#include "MainControlsWidget.h"
#include "ProgressSlider.h"
#include "SvgHandler.h"
#include "SvgTinter.h"
#include "VolumeWidget.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "ToolBar.h"

#include <KAction>
#include <KApplication>
#include <KVBox>

#include <QPainter>
#include <KPixmapCache>
#include <QResizeEvent>
#include <QVBoxLayout>

MainToolbar::MainToolbar( QWidget * parent )
    : KHBox( parent )
    , EngineObserver( The::engineController() )
    , m_addActionsOffsetX( 0 )
    , m_ignoreCache( false )
{
    setObjectName( "MainToolbar" );

    setMaximumSize( 20000, 67 );
    setMinimumSize( 200, 67 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    layout()->setContentsMargins( 0, 0, 0, 0 );
    setAutoFillBackground ( false );

    KHBox * hBox = new KHBox( this );

    m_mainControlsWidget = new MainControlsWidget( hBox );

    KVBox * vBox = new KVBox( hBox );
    vBox->setContentsMargins( 0, 6, 0, 0 );

    KHBox * topHBox = new KHBox( vBox );

    m_addControlsToolbar = new Amarok::ToolBar( topHBox );
    m_addControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    m_addControlsToolbar->setIconDimensions( 16 );
    m_addControlsToolbar->setMovable( false );
    m_addControlsToolbar->setFloatable ( false );
    m_addControlsToolbar->setFixedHeight( 22 );
    m_addControlsToolbar->setContentsMargins( 0, 0, 0, 0 );

    ProgressWidget *pWidget = new ProgressWidget( vBox );
    pWidget->setMinimumSize( 100, 17 );
    //pWidget->setMaximumSize( 600000, 24 );
    pWidget->setContentsMargins( 0, 2, 0, 0 );

    m_volumeWidget = new VolumeWidget( topHBox );
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
    QPainter painter( this );

    //const int watermarkWidth = height() * 1.36;

    //painter.drawPixmap( width() - watermarkWidth, 0, The::svgHandler()->renderSvg( "volume_watermark", watermarkWidth, height(), "volume_watermark" ) );
    
    painter.drawPixmap( 0, 0, The::svgHandler()->renderSvg( "toolbar_bg", width(), height(), "toolbar_bg" ) );
}

void MainToolbar::engineStateChanged( Phonon::State state, Phonon::State oldState )
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
            const int numberOfActions = m_additionalActions.size();

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

    /*const int middle = event->size().width() / 2;
    const int controlWidth = m_playerControlsToolbar->width();

    m_playerControlsToolbar->move( middle - ( controlWidth / 2 ), 0 );
    m_addControlsToolbar->move( middle + ( controlWidth / 2 ) + 10 , 9 );
    m_volumeWidget->move( event->size().width() - 170, 11 );*/
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

void MainToolbar::reRender()
{
    m_ignoreCache = true;
    update();
}


