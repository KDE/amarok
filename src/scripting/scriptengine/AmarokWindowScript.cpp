/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "AmarokWindowScript.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "App.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "ScriptingDefines.h"

#include <QAction>
#include <KActionCollection>

#include <QClipboard>
#include <QToolTip>

using namespace AmarokScript;

class ToolTipEventFilter : public QObject
{
    public:
        static ToolTipEventFilter *instance();

    private:
        ToolTipEventFilter();
        bool eventFilter( QObject *object, QEvent *event ) override;

        static QPointer<ToolTipEventFilter> s_instance;
};

QPointer<ToolTipEventFilter> ToolTipEventFilter::s_instance;

ToolTipEventFilter*
ToolTipEventFilter::instance()
{
    if( !s_instance )
        s_instance = new ToolTipEventFilter();
    return s_instance.data();
}

ToolTipEventFilter::ToolTipEventFilter()
: QObject( The::mainWindow() )
{}

bool
ToolTipEventFilter::eventFilter( QObject *object, QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
        QApplication::clipboard()->setText( object->objectName() );
    return false;
}

// AmarokWindowScript

AmarokWindowScript::AmarokWindowScript( AmarokScriptEngine* scriptEngine )
    : QObject( scriptEngine )
    , m_toolsMenu( The::mainWindow()->ToolsMenu() )
    , m_settingsMenu( The::mainWindow()->SettingsMenu() )
    , m_scriptEngine( scriptEngine )
{
    QJSValue windowObject = scriptEngine->newQObject( this );
    scriptEngine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Window"), windowObject );

    windowObject.setProperty( QStringLiteral("ToolsMenu"), scriptEngine->newObject() );
    windowObject.setProperty( QStringLiteral("SettingsMenu"), scriptEngine->newObject() );

    connect( pApp, &App::prepareToQuit, this, &AmarokWindowScript::prepareToQuit );
}

void
AmarokWindowScript::addToolsMenu( QMenu *menu )
{
    m_toolsMenu.data()->addMenu( menu );
}

void
AmarokWindowScript::addSettingsMenu( QMenu *menu )
{
    m_settingsMenu.data()->addMenu( menu );

}

bool
AmarokWindowScript::addToolsMenu( const QString &id, const QString &menuTitle, const QString &icon )
{
    return addMenuAction( m_toolsMenu, id, menuTitle, QStringLiteral("ToolsMenu"), icon );
}

void
AmarokWindowScript::addToolsSeparator()
{
    m_toolsMenu.data()->addSeparator();
}

bool
AmarokWindowScript::addSettingsMenu( const QString &id, const QString &actionName, const QString &icon )
{
    return addMenuAction( m_settingsMenu, id, actionName, QStringLiteral("SettingsMenu"), icon );
}

bool
AmarokScript::AmarokWindowScript::addCustomAction(const QString &menuName, const QString &id, const QString &actionName, const QString &icon)
{
    if( !m_customMenus.contains( menuName ) )
        return false;

    return addMenuAction( m_customMenus.value( menuName ), id, actionName, menuName, icon );
}

void
AmarokWindowScript::addSettingsSeparator()
{
    m_settingsMenu->addSeparator();
}

bool
AmarokWindowScript::addMenuAction( QMenu *menu, const QString &id, const QString &actionName, const QString &menuProperty, const QString &icon )
{
    KActionCollection* const ac = Amarok::actionCollection();
    if( ac->action( id ) )
        return false;

    QAction *action = new QAction( QIcon::fromTheme( icon ), actionName, this );
    ac->addAction( id, action );

    // don't forget to read the shortcut settings from the config file so
    // the shortcuts for the actions are updated
    ac->readSettings();

    // add the action to the given menu
    menu->addAction( ac->action( id ) );

    QJSValue newMenu = m_scriptEngine->newQObject( action );
    m_scriptEngine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Window") ).property( menuProperty ).setProperty( id, newMenu );
    return true;
}

void
AmarokWindowScript::showTrayIcon( bool show )
{
    AmarokConfig::setShowTrayIcon( show );
    pApp->applySettings();
}

QString
AmarokWindowScript::activeBrowserName()
{
    return The::mainWindow()->activeBrowserName();
}

bool
AmarokWindowScript::isTrayIconShown()
{
    return AmarokConfig::showTrayIcon();
}

QMainWindow*
AmarokWindowScript::mainWindow()
{
    return The::mainWindow();
}

QPalette
AmarokWindowScript::palette() const
{
    return The::paletteHandler()->palette();
}

void
AmarokWindowScript::setPalette( const QPalette &palette )
{
    The::paletteHandler()->setPalette( palette );
}

void
AmarokWindowScript::setStyleSheet( const QString &styleSheet )
{
    The::mainWindow()->setStyleSheet( styleSheet );
}

QString
AmarokWindowScript::styleSheet() const
{
    return The::mainWindow()->styleSheet();
}

QFont
AmarokWindowScript::font() const
{
    return The::mainWindow()->font();
}

void
AmarokWindowScript::setFont( const QFont &font )
{
    The::mainWindow()->setFont( font );
    The::mainWindow()->collectionBrowser()->update();
}

void
AmarokWindowScript::showToolTip()
{
    auto list = The::mainWindow()->findChildren<QWidget*>();
    for( QWidget *widget : list )
    {
        widget->setToolTip( widget->objectName() );
        widget->installEventFilter( ToolTipEventFilter::instance() );
    }
}
