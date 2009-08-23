/****************************************************************************************
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/*
  Significant parts of this code is inspired and/or copied from KDE plasma sources,
  available at kdebase/workspace/plasma
*/

#include "ContextView.h"

#include "Amarok.h"
#include "Context.h"
#include "ContextScene.h"
#include "Debug.h"
#include "Svg.h"
#include "Theme.h"
#include "amarokconfig.h"

#include <plasma/dataenginemanager.h>

#include <QWheelEvent>


#define DEBUG_PREFIX "ContextView"

namespace Context
{

ContextView* ContextView::s_self = 0;


ContextView::ContextView( Plasma::Containment *cont, Plasma::Corona *corona, QWidget* parent )
    : Plasma::View( cont, parent )
    , EngineObserver( The::engineController() )
    , m_curState( Home )
    , m_firstPlayingState( true )
{
    Q_UNUSED( corona )
    DEBUG_BLOCK

    s_self = this;

    scene()->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::NoAnchor );
    setCacheMode( QGraphicsView::CacheBackground );
    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
   // setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setMouseTracking( true );
    setScreen( -1 );
    
    //make background transparent
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    PERF_LOG( "Accessing Plasma::Theme" );
    // here we initialize all the Plasma paths to Amarok paths
    Plasma::Theme::defaultTheme()->setUseGlobalSettings( false );
    Theme::defaultTheme()->setThemeName( "Amarok-Mockup" );
    PERF_LOG( "Access to Plasma::Theme complete" )
    contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );
  
    cont->setPos( 0, 0 );
    cont->updateConstraints();
    Containment* amarokContainment = qobject_cast<Containment* >( cont );    
    if( amarokContainment )
    {
        amarokContainment->setView( this );
    //    amarokContainment->addCurrentTrack();
    }
}

ContextView::~ContextView()
{
    DEBUG_BLOCK

    // Unload and destroy all Amarok plasma-engines
    const QStringList engines = Plasma::DataEngineManager::self()->listAllEngines( "Amarok" );

    // Assert added for tracing crash on exit, see BUG 187384
    Q_ASSERT_X( !engines.isEmpty(), "Listing loaded Plasma engines", "List is empty (no engines loaded!?)" );

    foreach( const QString &engine, engines )
    {
        debug() << "Unloading plasma engine: " << engine;

        // PlasmaDataEngineManager uses refcounting for the engines, so we need to unload until the refcount reaches 0
        while( Plasma::DataEngineManager::self()->engine( engine )->isValid() )
            Plasma::DataEngineManager::self()->unloadEngine( engine );
    }
     
    clear( m_curState );
    //this should be done to prevent a crash on exit
    clearFocus();
}


void
ContextView::clear( const ContextState& state )
{
    Q_UNUSED( state )
    DEBUG_BLOCK

    QString name = "amarok_homerc";
    // now we save the state, remembering the column info etc
    KConfig appletConfig( name );
    // erase previous config
    foreach( const QString& group, appletConfig.groupList() )
        appletConfig.deleteGroup( group );

    const int numContainments = contextScene()->containments().size();
    for( int i = 0; i < numContainments; i++ )
    {
        DEBUG_LINE_INFO
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        KConfigGroup cg( &appletConfig, QString( "Containment %1" ).arg( i ) );
        if( containment )
            containment->saveToConfig( cg );
    }
    contextScene()->clearContainments();
}


void ContextView::enginePlaybackEnded(int finalPosition, int trackLength, EngineObserver::PlaybackEndedReason reason)
{
    DEBUG_BLOCK
    messageNotify( Home );
}


void ContextView::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    messageNotify( Current );
    m_firstPlayingState = false;
   
}


void
ContextView::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData )
    Q_UNUSED( trackChanged )
    DEBUG_BLOCK


    // if we are listening to a stream, take the new metadata as a "new track"
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track && track->type() == "stream" )
        messageNotify( Current );
}

void ContextView::showHome()
{
    DEBUG_BLOCK

    m_curState = Home;
    loadConfig();
    messageNotify( m_curState );
}


// loads applets onto the ContextScene from saved data, using m_curState
void
ContextView::loadConfig()
{
    contextScene()->clearContainments();    

    int numContainments = contextScene()->containments().size();
    KConfig conf( "amarok_homerc", KConfig::FullConfig );
    for( int i = 0; i < numContainments; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
        {
            KConfigGroup cg( &conf, QString( "Containment %1" ).arg( i ) );
            containment->loadConfig( cg );
        }
    }
}

Plasma::Applet*
ContextView::addApplet( const QString& name, const QStringList& args )
{
    QVariantList argList;
    QStringListIterator i(args);
    while( i.hasNext() )
        argList << QVariant( i.next() );

    if( !containment() )
        contextScene()->addContainment( "amarok_containment_vertical" );

    return containment()->addApplet( name, argList );
}

ContextScene*
ContextView::contextScene()
{
    return static_cast<ContextScene*>( scene() );
}

void
ContextView::resizeEvent( QResizeEvent* event )
{
  //  DEBUG_BLOCK
    Q_UNUSED( event )
       
    if ( testAttribute( Qt::WA_PendingResizeEvent ) ) {
        return; // lets not do this more than necessary, shall we?
    }

   updateContainmentsGeometry();
}


void
ContextView::updateContainmentsGeometry()
{
  //  DEBUG_BLOCK
    
    debug() << "resizing containment to: " << rect();

    containment()->resize( rect().size() );
    containment()->setPos( rect().topLeft() );

}

void
ContextView::wheelEvent( QWheelEvent* event )
{
    if( event->orientation() != Qt::Horizontal )
        QGraphicsView::wheelEvent( event );
}

} // Context namespace

#include "ContextView.moc"

