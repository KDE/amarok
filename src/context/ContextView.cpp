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

#include "ContextView.h"

#include "AppletLoader.h"
#include "AppletModel.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/ContextUrlRunner.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <QDesktopServices>
#include <QFile>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickWindow>

#include <KLocalizedContext>
#include <KPackage/PackageLoader>


namespace Context
{

ContextView* ContextView::s_self = nullptr;


ContextView::ContextView( QWidget *parent )
    : QQuickWidget( parent )
    , m_urlRunner( nullptr )
    , m_loader( new AppletLoader( this ) )
    , m_appletModel( new AppletModel( m_loader, this ) )
    , m_proxyModel( new AppletProxyModel( m_appletModel, this ) )
{
    DEBUG_BLOCK

    engine()->rootContext()->setContextObject( new KLocalizedContext( this ) );

    connect( this, &QQuickWidget::statusChanged, this, &ContextView::slotStatusChanged );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &ContextView::updatePalette );

    m_urlRunner = new ContextUrlRunner();
    The::amarokUrlHandler()->registerRunner( m_urlRunner, QStringLiteral("context") );

    rootContext()->setContextProperty( QStringLiteral( "AppletModel" ), m_appletModel );
    rootContext()->setContextProperty( QStringLiteral( "AppletProxyModel" ), m_proxyModel );
    rootContext()->setContextProperty( QStringLiteral( "Context" ), this );
    rootContext()->setContextProperty( QStringLiteral( "Svg" ), The::svgHandler() );

    quickWindow()->setColor( The::paletteHandler()->palette().color( QPalette::Window ) );

    auto qmlPackage = KPackage::PackageLoader::self()->loadPackage( QStringLiteral( "KPackage/GenericQML" ),
                                                                    QStringLiteral( "org.kde.amarok.context" ) );
    Q_ASSERT( qmlPackage.isValid() );

    const QUrl sourceUrl = qmlPackage.fileUrl( "mainscript" );

    ::debug() << "Loading context qml mainscript:" << sourceUrl;

    setSource( sourceUrl );
    setResizeMode( SizeRootObjectToView );

    // keep this assignment at bottom so that premature usage of ::self() asserts out
    s_self = this;
}

ContextView::~ContextView()
{
    DEBUG_BLOCK

    delete m_urlRunner;
    s_self = nullptr;
}

QStringList
ContextView::currentApplets() const
{
    QStringList appletNames;
    
    auto applets = m_loader->enabledApplets();
    for( const auto &applet : applets )
    {
        appletNames << applet.pluginId();
    }

    ::debug() << "Current applets: " << appletNames;

    return appletNames;
}

QStringList
ContextView::currentAppletNames() const
{
    QStringList appletNames;

    auto applets = m_loader->enabledApplets();
    for( const auto &applet : applets )
    {
        appletNames << applet.name();
    }

    ::debug() << "Current applet names: " << appletNames;

    return appletNames;
}

void
ContextView::runLink( const QUrl& link ) const
{
    if( link.scheme() == QStringLiteral( "amarok" ) )
    {
        AmarokUrl aUrl( link.toString() );
        aUrl.run();
    }
    else
        QDesktopServices::openUrl( link );
}

void
ContextView::slotStatusChanged( Status status )
{
    if( status == Error )
        for( const auto &e : errors() )
            error( e.description() );
}

void
ContextView::updatePalette( const QPalette &palette )
{
    quickWindow()->setColor( palette.color( QPalette::Window ) );
}

void
ContextView::debug( const QString &error ) const
{
    ::debug() << error;
}

void
ContextView::warning( const QString &error ) const
{
    ::warning() << error;
}

void
ContextView::error( const QString &error ) const
{
    ::error() << error;
}

} // Context namespace
