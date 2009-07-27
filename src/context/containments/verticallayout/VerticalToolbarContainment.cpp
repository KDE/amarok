/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "VerticalToolbarContainment.h"

#include "ContextView.h"
#include "Debug.h"
#include "PaletteHandler.h"
#include "VerticalAppletLayout.h"

#include <KConfig>

#include <QGraphicsLinearLayout>

#define TOOLBAR_X_OFFSET 2000


Context::VerticalToolbarContainment::VerticalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_applets( 0 )
    , m_noApplets( true )
    , m_noAppletText( 0 )
{
    DEBUG_BLOCK
    setContainmentType( CustomContainment );
    setDrawWallpaper( false );
   // setScreen( -1 );
    setImmutability( Plasma::Mutable );
    
    debug() << "applet containment has corona:" << corona();
    m_applets = new VerticalAppletLayout( this );
    
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), SLOT( appletRemoved( Plasma::Applet* ) ) );
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), SIGNAL( geometryChanged() ) );
             
    connect( m_applets,  SIGNAL( appletAdded( Plasma::Applet*, int ) ), SIGNAL( appletAdded( Plasma::Applet*, int) ) ); // goes out to applet toolbar
    connect( m_applets, SIGNAL(  appletAdded( Plasma::Applet*, int ) ), SIGNAL( geometryChanged() ) );
    
    connect( m_applets, SIGNAL( noApplets( bool ) ), SLOT( showEmptyText( bool ) ) );

    m_noAppletText = new QGraphicsTextItem( this );
    m_noAppletText->setHtml( QString( "<html>  <style type=\"text/css\"> body { background-color: %1; } </style> \
                                        <body> <p align=\"center\"> %3 </p></body></html>" )
                                       .arg( PaletteHandler::highlightColor().name() )
                                       .arg( i18n( "Please add some applets from the toolbar at the bottom of the context view." ) ) );
                                       
}

Context::VerticalToolbarContainment::~VerticalToolbarContainment()
{}

void 
Context::VerticalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )

    debug() << "setting applets geom to" << contentsRect();
    m_applets->setGeometry( contentsRect() );

    if (constraints & Plasma::FormFactorConstraint ||
        constraints & Plasma::StartupCompletedConstraint) {

        foreach (Applet *applet, applets()) {
            applet->setBackgroundHints(NoBackground);
        }
    }
}

QList<QAction*> 
Context::VerticalToolbarContainment::contextualActions()
{
    return QList< QAction* >();
}

void 
Context::VerticalToolbarContainment::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& contentsRect)
{
    Q_UNUSED( painter );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );

    
    if( m_noApplets ) // draw help text
    {
        QRectF masterRect = view()->rect();
        m_noAppletText->setTextWidth( masterRect.width() * .4 );
        QPointF topLeft( ( masterRect.width() / 2 ) - ( m_noAppletText->boundingRect().width() / 2 ), ( masterRect.height() / 2 ) - ( m_noAppletText->boundingRect().height() / 2 ) );
        m_noAppletText->setPos( topLeft );
    }
}

void
Context::VerticalToolbarContainment::saveToConfig( KConfigGroup &conf )
{
    m_applets->saveToConfig( conf );
}

void
Context::VerticalToolbarContainment::loadConfig( const KConfigGroup &conf )
{
    DEBUG_BLOCK

    QStringList plugins = conf.readEntry( "plugins", QStringList() );
    debug() << "plugins.size(): " << plugins.size();

    foreach( const QString& plugin, plugins )
    {
        debug() << "Adding applet: " << plugin;
        addApplet( plugin, -1 );
    }

    int showing = conf.readEntry( "firstShowingApplet", 0 );
    m_applets->showAtIndex( showing );
}

void 
Context::VerticalToolbarContainment::setView( ContextView* view )
{
    DEBUG_BLOCK

    m_view = view;
    // kick the toolbar with a real corona no w
    emit updatedContainment( this );
}

Context::ContextView*
Context::VerticalToolbarContainment::view()
{
    return m_view;
}

QRectF 
Context::VerticalToolbarContainment::boundingRect () const
{
    return QRectF( QPointF( 0, 0), m_applets->totalSize() );
}

Plasma::Applet* 
Context::VerticalToolbarContainment::addApplet( const QString& pluginName, const int loc ) // SLOT
{
    DEBUG_BLOCK

    Plasma::Applet* applet = Plasma::Containment::addApplet( pluginName );

    if( applet == 0 )
        debug() << "FAILED ADDING APPLET TO CONTAINMENT!! NOT FOUND!!";
    else
        m_applets->addApplet( applet, loc );

    return applet;
}

void    
Context::VerticalToolbarContainment::appletRemoved( Plasma::Applet* applet )
{
    m_applets->appletRemoved( applet );
}

void
Context::VerticalToolbarContainment::showApplet( Plasma::Applet* applet )
{
    m_applets->showApplet( applet );
}

void
Context::VerticalToolbarContainment::moveApplet( Plasma::Applet* applet, int a, int b)
{
    m_applets->moveApplet( applet, a, b);
}

void
Context::VerticalToolbarContainment::wheelEvent( QGraphicsSceneWheelEvent* event )
{
    Q_UNUSED( event )

    //eat wheel events, we dont want scrolling
}

void
Context::VerticalToolbarContainment::showEmptyText( bool toShow ) // SLOT
{
    m_noApplets = toShow;
    if( toShow )
        m_noAppletText->show();
    else
        m_noAppletText->hide();
    updateConstraints();
    update();
}

#include "VerticalToolbarContainment.moc"
