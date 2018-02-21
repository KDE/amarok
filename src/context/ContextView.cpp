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
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlPropertyMap>
#include <QQuickWindow>
#include <QScreen>

#include <KDeclarative/KDeclarative>
#include <KI18n/KLocalizedContext>
#include <KIconThemes/KIconLoader>
#include <KPackage/PackageLoader>


class FontFilter : public QObject
{
    Q_OBJECT

public:
    FontFilter( QObject *parent )
        : QObject( parent )
    {
        qApp->installEventFilter( this );
    }

    bool eventFilter( QObject *watched, QEvent *event )
    {
        if( watched == qApp )
        {
            if( event->type() == QEvent::ApplicationFontChange )
                emit fontChanged();
        }
        return QObject::eventFilter( watched, event );
    }

signals:
    void fontChanged();
};


namespace Context
{

ContextView* ContextView::s_self = Q_NULLPTR;


ContextView::ContextView( QWidget *parent )
    : QQuickWidget( parent )
    , m_urlRunner( Q_NULLPTR )
    , m_loader( new AppletLoader( this ) )
    , m_appletModel( new AppletModel( m_loader, this ) )
    , m_proxyModel( new AppletProxyModel( m_appletModel, this ) )
    , m_fontFilter( new FontFilter( this ) )
    , m_smallSpacing( 2 )
    , m_largeSpacing( 8 )
    , m_iconSizes( new QQmlPropertyMap( this ) )
{
    DEBUG_BLOCK

    KDeclarative::KDeclarative decl;
    decl.setDeclarativeEngine( engine() );
    decl.setupBindings();

    connect( this, &QQuickWidget::statusChanged, this, &ContextView::slotStatusChanged );
    connect( qApp, &QGuiApplication::primaryScreenChanged, this, &ContextView::updateDevicePixelRatio );
    connect( m_fontFilter, &FontFilter::fontChanged, this, &ContextView::updateSpacing );
    connect( KIconLoader::global(), &KIconLoader::iconLoaderSettingsChanged, this, &ContextView::iconLoaderSettingsChanged );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &ContextView::updatePalette );

    updateSpacing();
    updateDevicePixelRatio( qApp->primaryScreen() );

    m_urlRunner = new ContextUrlRunner();
    The::amarokUrlHandler()->registerRunner( m_urlRunner, "context" );

    rootContext()->setContextProperty( QStringLiteral( "AppletModel" ), m_appletModel );
    rootContext()->setContextProperty( QStringLiteral( "AppletProxyModel" ), m_proxyModel );
    rootContext()->setContextProperty( QStringLiteral( "Context" ), this );
    rootContext()->setContextProperty( QStringLiteral( "Svg" ), The::svgHandler() );

    quickWindow()->setColor( The::paletteHandler()->palette().color( QPalette::Window ) );

    auto qmlPackage = KPackage::PackageLoader::self()->loadPackage( QStringLiteral( "KPackage/GenericQML" ),
                                                                    QStringLiteral( "org.kde.amarok.context" ) );
    Q_ASSERT( qmlPackage.isValid() );

    const QString sourcePath = qmlPackage.filePath( "mainscript" );
    Q_ASSERT( QFile::exists( sourcePath ) );

    ::debug() << "Loading context qml mainscript:" << sourcePath;

    setSource( QUrl::fromLocalFile( sourcePath ) );
    setResizeMode( SizeRootObjectToView );

    // keep this assignment at bottom so that premature usage of ::self() asserts out
    s_self = this;
}

ContextView::~ContextView()
{
    DEBUG_BLOCK

    delete m_urlRunner;
    s_self = Q_NULLPTR;
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
ContextView::updateSpacing()
{
    int gridUnit = QFontMetrics( QGuiApplication::font() ).boundingRect( QStringLiteral("M") ).height();
    if (gridUnit % 2 != 0)
        gridUnit++;

    if (gridUnit != m_largeSpacing)
    {
        m_smallSpacing = qMax( 2, (int)( gridUnit / 4 ) ); // 1/4 of gridUnit, at least 2
        m_largeSpacing = gridUnit; // msize.height
        emit spacingChanged();
    }
}

void
ContextView::updateDevicePixelRatio( QScreen *screen )
{
    if (!screen)
        return;

    const qreal dpi = screen->logicalDotsPerInchX();
    // Usual "default" is 96 dpi
    m_devicePixelRatio = (qreal)dpi / (qreal)96;
    iconLoaderSettingsChanged();
    emit devicePixelRatioChanged();
}

void
ContextView::iconLoaderSettingsChanged()
{
    m_iconSizes->insert( QStringLiteral( "tiny" ), devicePixelIconSize( KIconLoader::SizeSmall ) / 2 );
    m_iconSizes->insert( QStringLiteral( "small" ), devicePixelIconSize( KIconLoader::SizeSmall ) );
    m_iconSizes->insert( QStringLiteral( "smallMedium" ), devicePixelIconSize( KIconLoader::SizeSmallMedium ) );
    m_iconSizes->insert( QStringLiteral( "medium" ), devicePixelIconSize( KIconLoader::SizeMedium ) );
    m_iconSizes->insert( QStringLiteral( "large" ), devicePixelIconSize( KIconLoader::SizeLarge ) );
    m_iconSizes->insert( QStringLiteral( "huge" ), devicePixelIconSize( KIconLoader::SizeHuge ) );
    m_iconSizes->insert( QStringLiteral( "enormous" ), devicePixelIconSize( KIconLoader::SizeEnormous ) );

    emit iconSizesChanged();
}

int
ContextView::devicePixelIconSize( int size ) const
{
    const qreal ratio = devicePixelRatio();

    if ( ratio < 1.5 )
        return size;
    else if ( ratio < 2.0 )
        return size * 1.5;
    else if ( ratio < 2.5 )
        return size * 2.0;
    else if ( ratio < 3.0 )
         return size * 2.5;
    else if ( ratio < 3.5 )
         return size * 3.0;
    else
        return size * ratio;
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

#include <ContextView.moc>
