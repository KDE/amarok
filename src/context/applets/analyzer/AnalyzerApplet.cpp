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
#include "core/support/Debug.h"

#include "BallsAnalyzer.h"
#include "BlockAnalyzer.h"
#include "DiscoAnalyzer.h"

#include <QAction>
#include <QGraphicsView>
#include <QMenu>


AnalyzerApplet::AnalyzerApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_analyzer( 0 )
{
    setHasConfigurationInterface( false );

    connect( this, SIGNAL( geometryChanged() ), this, SLOT( newGeometry() ) );
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

    m_analyzerNames["Balls"] = i18n( "Balls (OpenGL)" );
    m_analyzerNames["Blocky"] = i18n( "Blocky" );
    m_analyzerNames["Disco"] = i18n( "Disco (OpenGL)" );

    KConfigGroup config = Amarok::config( "Analyzer Applet" );

    if( config.readEntry( "Height", (int)Small ) == Small )
        setHeightSmall();
    if( config.readEntry( "Height", (int)Small ) == Medium )
        setHeightMedium();
    if( config.readEntry( "Height", (int)Small ) == Tall )
        setHeightTall();

    setCurrentAnalyzer( config.readEntry( "Current Analyzer", "Blocky" ) );
}

void
AnalyzerApplet::newGeometry()
{
    DEBUG_BLOCK

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

    action = heightMenu->addAction( i18n( "Small" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Small );
    action->setActionGroup( heightActions );
    connect( action, SIGNAL( triggered() ), this, SLOT( setHeightSmall() ) );
    action = heightMenu->addAction( i18n( "Medium" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Medium );
    action->setActionGroup( heightActions );
    connect( action, SIGNAL( triggered() ), this, SLOT( setHeightMedium() ) );
    action = heightMenu->addAction( i18n( "Tall" ) );
    action->setCheckable( true );
    action->setChecked( m_currentHeight == Tall );
    action->setActionGroup( heightActions );
    connect( action, SIGNAL( triggered() ), this, SLOT( setHeightTall() ) );

    action = new QAction( this );
    action->setSeparator( true );
    actions << action;

    QActionGroup *analyzerActions = new QActionGroup( this );
    connect( analyzerActions, SIGNAL( triggered( QAction* ) ), this, SLOT( analyzerAction( QAction* ) ) );

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
AnalyzerApplet::setHeightSmall()
{
    setMinimumHeight( 100 );
    m_currentHeight = Small;
}

void
AnalyzerApplet::setHeightMedium()
{
    setMinimumHeight( 150 );
    m_currentHeight = Medium;
}

void
AnalyzerApplet::setHeightTall()
{
    setMinimumHeight( 200 );
    m_currentHeight = Tall;
}

void
AnalyzerApplet::analyzerAction( QAction *action )
{
    DEBUG_BLOCK

    setCurrentAnalyzer( action->data().toString() );
}

void
AnalyzerApplet::setCurrentAnalyzer( const QString &name )
{
    DEBUG_BLOCK

    debug() << "name: " << name;

    if( m_analyzerName == name )
        return;

    delete m_analyzer;
    m_analyzerName = name;

    if( name == "Balls" )
        m_analyzer = new BallsAnalyzer( view()->viewport() );
    else if( name == "Blocky" )
        m_analyzer = new BlockAnalyzer( view()->viewport() );
    else if( name == "Disco" )
        m_analyzer = new DiscoAnalyzer( view()->viewport() );

    connect( this, SIGNAL( appletDestroyed( Plasma::Applet* ) ), m_analyzer, SLOT( deleteLater() ) );

    newGeometry();
    m_analyzer->show();
}

#include "AnalyzerApplet.moc"
