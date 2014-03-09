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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "ContextView"

/*
  Significant parts of this code is inspired and/or copied from KDE plasma sources,
  available at kdebase/workspace/plasma
*/

#include "ContextView.h"

#include <config.h>

#include "App.h"
#include "Context.h"
#include "ContextScene.h"
#include "Svg.h"
#include "Theme.h"
#include "amarokconfig.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/ContextUrlRunner.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "EngineController.h"

#include <plasma/dataenginemanager.h>

#include <Phonon/AudioOutput>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QWheelEvent>

namespace Context
{

ContextView* ContextView::s_self = 0;


ContextView::ContextView( Plasma::Containment *cont, Plasma::Corona *corona, QWidget* parent )
    : Plasma::View( cont, parent )
    , m_curState( Home )
    , m_urlRunner(0)
    , m_appletExplorer(0)
    , m_collapseAnimations(0)
    , m_queuedAnimations(0)
    , m_collapseGroupTimer(0)
{
    Q_UNUSED( corona )
    DEBUG_BLOCK

    // using QGraphicsScene::BspTreeIndex leads to crashes in some Qt versions
    scene()->setItemIndexMethod( QGraphicsScene::NoIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::NoAnchor );
    setCacheMode( QGraphicsView::CacheBackground );
    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    // setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    //make background transparent
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );

    cont->setPos( 0, 0 );
    cont->updateConstraints();
    Containment* amarokContainment = qobject_cast<Containment* >( cont );
    if( amarokContainment )
        amarokContainment->setView( this );

    m_urlRunner = new ContextUrlRunner();
    The::amarokUrlHandler()->registerRunner( m_urlRunner, "context" );

    m_queuedAnimations = new QSequentialAnimationGroup( this );
    m_collapseAnimations = new QParallelAnimationGroup( this );
    connect( m_collapseAnimations, SIGNAL(finished()),
             this, SLOT(slotCollapseAnimationsFinished()) );

    m_collapseGroupTimer = new QTimer( this );
    m_collapseGroupTimer->setSingleShot( true );
    connect( m_collapseGroupTimer, SIGNAL(timeout()), SLOT(slotStartCollapseAnimations()) );

    EngineController* const engine = The::engineController();

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(updateNeeded()) );
//    connect( App::instance()->mainWindow(), SIGNAL(windowRestored()),
//             this, SLOT(updateNeeded()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(updateNeeded()) );

    // keep this assignment at bottom so that premature usage of ::self() asserts out
    s_self = this;
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

    delete m_urlRunner;
}


void
ContextView::clear( const ContextState& state )
{
    Q_UNUSED( state )
    DEBUG_BLOCK

    const QString name = "amarok_homerc";
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

void ContextView::clearNoSave()
{
    contextScene()->clearContainments();
}


void ContextView::updateNeeded()
{
    DEBUG_BLOCK
    if( !The::engineController()->isStopped() || The::engineController()->isStream() )
        messageNotify( Current );
    else
        messageNotify( Home );
    adjustSize();
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

    PERF_LOG( "Start to load config" );
    int numContainments = contextScene()->containments().size();
    KConfig conf( "amarok_homerc", KConfig::FullConfig );
    for( int i = 0; i < numContainments; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
        {
            KConfigGroup cg( &conf, QString( "Containment %1" ).arg( i ) );
#ifdef QT_QTOPENGL_FOUND
            // Special case: If this is the first time that the user runs an Amarok version
            // containing the Analyzer applet, modify the user's config so that the applet
            // will become active. We do this for discoverability and prettiness.
            // Remove this code in a future Amarok release (possibly 3.0)
            const bool firstTimeWithAnalyzer = Amarok::config( "Context View" ).readEntry( "firstTimeWithAnalyzer", true );
            if( firstTimeWithAnalyzer )
            {
                QStringList plugins = cg.readEntry( "plugins", QStringList() );
                if( EngineController::instance()->supportsAudioDataOutput() && !plugins.contains( "analyzer" ) )
                {
                    Amarok::config( "Context View" ).writeEntry( "firstTimeWithAnalyzer", false );

                    // Put the Analyzer applet at position #2, which is most likely below the Currenttrack applet.
                    if( !plugins.empty() )
                        plugins.insert( 1, "analyzer" );

                    cg.writeEntry( "plugins", plugins );
                }
            }
#endif
            containment->loadConfig( cg );
        }
    }
    PERF_LOG( "Done loading config" );
}

void
ContextView::addCollapseAnimation( QAbstractAnimation *anim )
{
    if( !anim )
    {
        debug() << "failed to add collapsing animation";
        return;
    }

    if( m_collapseAnimations->state() == QAbstractAnimation::Running ||
        m_collapseGroupTimer->isActive() )
    {
        m_queuedAnimations->addAnimation( anim );
    }
    else
    {
        m_collapseAnimations->addAnimation( anim );
        m_collapseGroupTimer->start( 0 );
    }
}

void
ContextView::slotCollapseAnimationsFinished()
{
    m_collapseGroupTimer->stop();
    m_collapseAnimations->clear();

    while( m_queuedAnimations->animationCount() > 0 )
    {
        if( QAbstractAnimation *anim = m_queuedAnimations->takeAnimation(0) )
            m_collapseAnimations->addAnimation( anim );
    }

    if( m_collapseAnimations->animationCount() > 0 )
        m_collapseGroupTimer->start( 0 );
}

void
ContextView::slotStartCollapseAnimations()
{
    if( m_collapseAnimations->animationCount() > 0 )
        m_collapseAnimations->start( QAbstractAnimation::KeepWhenStopped );
}

void
ContextView::hideAppletExplorer()
{
    if( m_appletExplorer )
        m_appletExplorer->hide();
}

void
ContextView::showAppletExplorer()
{
    if( !m_appletExplorer )
    {
        Context::Containment *cont = qobject_cast<Context::Containment*>( containment() );
        m_appletExplorer = new AppletExplorer( cont );
        m_appletExplorer->setContainment( cont );
        m_appletExplorer->setZValue( m_appletExplorer->zValue() + 1000 );
        m_appletExplorer->setFlag( QGraphicsItem::ItemIsSelectable );

        connect( m_appletExplorer, SIGNAL(addAppletToContainment(QString,int)),
                 cont, SLOT(addApplet(QString,int)) );
        connect( m_appletExplorer, SIGNAL(appletExplorerHid()), SIGNAL(appletExplorerHid()) );
        connect( m_appletExplorer, SIGNAL(geometryChanged()), SLOT(slotPositionAppletExplorer()) );

        qreal height = m_appletExplorer->effectiveSizeHint( Qt::PreferredSize ).height();
        m_appletExplorer->resize( rect().width() - 2, height );
        m_appletExplorer->setPos( 0, rect().height() - height - 2 );
    }
    m_appletExplorer->show();
}

void
ContextView::slotPositionAppletExplorer()
{
    if( !m_appletExplorer )
        return;
    qreal height = m_appletExplorer->effectiveSizeHint( Qt::PreferredSize ).height();
    m_appletExplorer->setPos( 0, rect().height() - height - 2 );
}


ContextScene*
ContextView::contextScene()
{
    return static_cast<ContextScene*>( scene() );
}

void
ContextView::resizeEvent( QResizeEvent* event )
{
    Plasma::View::resizeEvent( event );
    if( testAttribute( Qt::WA_PendingResizeEvent ) )
        return; // lets not do this more than necessary, shall we?

    QRectF rect( pos(), maximumViewportSize() );
    containment()->setGeometry( rect );
    scene()->setSceneRect( rect );
    scene()->update( rect );

    if( m_appletExplorer )
    {
        qreal height = m_appletExplorer->effectiveSizeHint( Qt::PreferredSize ).height();
        m_appletExplorer->resize( rect.width() - 2, height );
        m_appletExplorer->setPos( 0, rect.height() - height - 2 );
    }
}

void
ContextView::wheelEvent( QWheelEvent* event )
{
    if( event->orientation() != Qt::Horizontal )
        Plasma::View::wheelEvent( event );
}

QStringList
ContextView::currentApplets()
{
    DEBUG_BLOCK
    QStringList appletNames;
    
    Applet::List applets = containment()->applets();
    foreach( Plasma::Applet * applet, applets )
    {
        appletNames << applet->pluginName();
    }

    debug() << "current applets: " << appletNames;

    return appletNames;
}

QStringList ContextView::currentAppletNames()
{
    DEBUG_BLOCK
    QStringList appletNames;

    Applet::List applets = containment()->applets();
    foreach( Plasma::Applet * applet, applets )
    {
        appletNames << applet->name();
    }

    debug() << "current applets: " << appletNames;

    return appletNames; 
}

} // Context namespace

#include "ContextView.moc"
