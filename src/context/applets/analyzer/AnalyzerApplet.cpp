/****************************************************************************************
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "AnalyzerApplet"

#include "AnalyzerApplet.h"

#include "core/support/Amarok.h"

#include "BallsAnalyzer.h"
#include "BlockAnalyzer.h"
#include "DiscoAnalyzer.h"
#include "ASCIIAnalyzer.h"

#include <QAction>
#include <QGraphicsView>
#include <QMenu>


AnalyzerApplet::AnalyzerApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_analyzer( 0 )
{
    setHasConfigurationInterface( false );

    connect( this, SIGNAL(geometryChanged()), SLOT(newGeometry()) );
}

AnalyzerApplet::~AnalyzerApplet()
{
    KConfigGroup config = Amarok::config( "Analyzer Applet" );
    config.writeEntry( "Height", (int)m_currentHeight );
    config.writeEntry( "Current Analyzer", m_analyzerName );
}

void
AnalyzerApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    m_analyzerNames["Balls"] = i18nc( "Analyzer name", "Balls" );
    m_analyzerNames["Blocky"] = i18nc( "Analyzer name", "Blocky" );
    m_analyzerNames["Disco"] = i18nc( "Analyzer name", "Disco" );
    m_analyzerNames["ASCII"] = i18nc( "Analyzer name", "ASCII" );

    KConfigGroup config = Amarok::config( "Analyzer Applet" );
    setNewHeight( (WidgetHeight)config.readEntry( "Height", int() ) );

    setCurrentAnalyzer( config.readEntry( "Current Analyzer", "Blocky" ) );
}

void
AnalyzerApplet::newGeometry() // SLOT
{
    if( !m_analyzer )
        return;

    // Use the applet's geometry for showing the analyzer widget at the same position
    QRect analyzerGeometry = geometry().toRect();

    // Adjust widget geometry to keep the applet border intact
    analyzerGeometry.adjust( +3, +3, -3, -3 );

    m_analyzer->setGeometry( analyzerGeometry );
}

void
AnalyzerApplet::hideEvent( QHideEvent* )
{
    m_analyzer->hide();
}

void
AnalyzerApplet::showEvent( QShowEvent* )
{
    m_analyzer->show();
}

QList<QAction *>
AnalyzerApplet::contextualActions ()
{
    QList<QAction*> actions;
    QAction *action;

    QMenu *heightMenu = new QMenu( i18n( "Height" ), view() );
    actions << heightMenu->menuAction();

    QActionGroup *heightActions = new QActionGroup( this );

    action = heightMenu->addAction( i18nc( "Height of the Analyzer applet", "Tiny" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Tiny );
    action->setActionGroup( heightActions );
    action->setData( (int)Tiny );
    connect( action, SIGNAL(triggered()), SLOT(heightActionTriggered()) );

    action = heightMenu->addAction( i18nc( "Height of the Analyzer applet", "Small" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Small );
    action->setActionGroup( heightActions );
    action->setData( (int)Small );
    connect( action, SIGNAL(triggered()), SLOT(heightActionTriggered()) );

    action = heightMenu->addAction( i18nc( "Height of the Analyzer applet", "Medium" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Medium );
    action->setActionGroup( heightActions );
    action->setData( (int)Medium );
    connect( action, SIGNAL(triggered()), SLOT(heightActionTriggered()) );

    action = heightMenu->addAction( i18nc( "Height of the Analyzer applet", "Tall" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Tall );
    action->setActionGroup( heightActions );
    action->setData( (int)Tall );
    connect( action, SIGNAL(triggered()), SLOT(heightActionTriggered()) );

    action = new QAction( this );
    action->setSeparator( true );
    actions << action;

    QActionGroup *analyzerActions = new QActionGroup( this );
    connect( analyzerActions, SIGNAL(triggered(QAction*)), SLOT( analyzerAction(QAction*)) );

    QMap<QString, QString>::const_iterator i = m_analyzerNames.constBegin();
    while ( i != m_analyzerNames.constEnd() ) {
        action = new QAction( i.value(), this );
        action->setData( i.key() );
        action->setCheckable( true );
        action->setChecked( m_analyzerName == i.key() );
        action->setActionGroup( analyzerActions );
        actions << action;
        i++;
    }

    return actions;
}

void
AnalyzerApplet::setNewHeight( WidgetHeight height )
{
    if( !( height == Tiny || height == Small || height == Medium || height == Tall ) )
        height = Default;

    setMinimumHeight( (int)height );
    m_currentHeight = height;
}

void
AnalyzerApplet::heightActionTriggered() // SLOT
{
    QAction *action = static_cast<QAction*>( sender() );
    setNewHeight( static_cast<WidgetHeight>( action->data().toInt() ) );
}

void
AnalyzerApplet::analyzerAction( QAction *action ) // SLOT
{
    setCurrentAnalyzer( action->data().toString() );
}

void
AnalyzerApplet::setCurrentAnalyzer( const QString &name )
{
    if( m_analyzerName == name )
        return;

    delete m_analyzer;

    if( name == "Balls" )
        m_analyzer = new BallsAnalyzer( view()->viewport() );
    else if( name == "Disco" )
        m_analyzer = new DiscoAnalyzer( view()->viewport() );
    else if( name == "ASCII" )
        m_analyzer = new ASCIIAnalyzer( view()->viewport() );
    else
        m_analyzer = new BlockAnalyzer( view()->viewport() ); // The default

    m_analyzerName = m_analyzer->objectName();
    m_analyzer->setToolTip( i18n( "Right-click to configure" ) );

    connect( this, SIGNAL(appletDestroyed(Plasma::Applet*)), m_analyzer, SLOT(deleteLater()) );

    newGeometry();
    m_analyzer->show();
}

#include "AnalyzerApplet.moc"
