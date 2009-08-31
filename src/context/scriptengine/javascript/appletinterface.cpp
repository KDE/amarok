/****************************************************************************************
 * Copyright (c) 2008 Chani Armitage <chani@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "appletinterface.h"

#include <QAction>
#include <QFile>
#include <QSignalMapper>

#include <KDE/KIcon>

#include <Plasma/Plasma>
#include <Plasma/Applet>
#include <Plasma/Context>
#include <Plasma/DataEngine>
#include <Plasma/Package>

#include "simplejavascriptapplet.h"

AppletInterface::AppletInterface(SimpleJavaScriptApplet *parent)
    : QObject(parent),
      m_appletScriptEngine(parent),
      m_actionSignals(0)
{
    connect(this, SIGNAL(releaseVisualFocus()), applet(), SIGNAL(releaseVisualFocus()));
    connect(this, SIGNAL(configNeedsSaving()), applet(), SIGNAL(configNeedsSaving()));
}

AppletInterface::~AppletInterface()
{
}

Plasma::DataEngine* AppletInterface::dataEngine(const QString &name)
{
    return applet()->dataEngine(name);
}

AppletInterface::FormFactor AppletInterface::formFactor()
{
    return static_cast<FormFactor>(applet()->formFactor());
}

AppletInterface::Location AppletInterface::location()
{
    return static_cast<Location>(applet()->location());
}

QString AppletInterface::currentActivity()
{
    return applet()->context()->currentActivity();
}

AppletInterface::AspectRatioMode AppletInterface::aspectRatioMode()
{
    return static_cast<AspectRatioMode>(applet()->aspectRatioMode());
}

void AppletInterface::setAspectRatioMode(AppletInterface::AspectRatioMode mode)
{
    applet()->setAspectRatioMode(static_cast<Plasma::AspectRatioMode>(mode));
}

bool AppletInterface::shouldConserveResources()
{
    return applet()->shouldConserveResources();
}

void AppletInterface::setFailedToLaunch(bool failed, const QString &reason)
{
    m_appletScriptEngine->setFailedToLaunch(failed, reason);
}

bool AppletInterface::isBusy()
{
    return applet()->isBusy();
}

void AppletInterface::setBusy(bool busy)
{
    applet()->setBusy(busy);
}

void AppletInterface::setConfigurationRequired(bool needsConfiguring, const QString &reason)
{
    m_appletScriptEngine->setConfigurationRequired(needsConfiguring, reason);
}

void AppletInterface::update()
{
    applet()->update();
}

QString AppletInterface::activeConfig() const
{
    return m_currentConfig.isEmpty() ? "main" : m_currentConfig;
}

void AppletInterface::setActiveConfig(const QString &name)
{
    if (name == "main") {
        m_currentConfig.clear();
        return;
    }

    Plasma::ConfigLoader *loader = m_configs.value(name, 0);

    if (!loader) {
        QString path = applet()->package()->filePath("config", name + ".xml");
        if (path.isEmpty()) {
            return;
        }

        QFile f(path);
        KConfigGroup cg = applet()->config();
        loader = new Plasma::ConfigLoader(&cg, &f, this);
        m_configs.insert(name, loader);
    }

    m_currentConfig = name;
}

void AppletInterface::writeConfig(const QString &entry, const QVariant &value)
{
    Plasma::ConfigLoader *config = 0;
    if (m_currentConfig.isEmpty()) {
        config = applet()->configScheme();
    } else {
        config = m_configs.value(m_currentConfig, 0);
    }

    if (config) {
        KConfigSkeletonItem *item = config->findItemByName(entry);
        if (item) {
            item->setProperty(value);
            config->writeConfig();
            m_appletScriptEngine->configNeedsSaving();
        }
    }
}

QScriptValue AppletInterface::readConfig(const QString &entry) const
{
    Plasma::ConfigLoader *config = 0;
    QVariant result;
    
    if (m_currentConfig.isEmpty()) {
        config = applet()->configScheme();
    } else {
        config = m_configs.value(m_currentConfig, 0);
    }

    if (config) {
        result = config->property(entry);
    }
    return m_appletScriptEngine->variantToScriptValue(result);
}

QString AppletInterface::file(const QString &fileType)
{
    return m_appletScriptEngine->package()->filePath(fileType.toLocal8Bit().constData());
}

QString AppletInterface::file(const QString &fileType, const QString &filePath)
{
    return m_appletScriptEngine->package()->filePath(fileType.toLocal8Bit().constData(), filePath);
}

const Plasma::Package *AppletInterface::package() const
{
    kDebug() << "woot";
    return m_appletScriptEngine->package();
}

QList<QAction*> AppletInterface::contextualActions() const
{
    QList<QAction*> actions;
    Plasma::Applet *a = applet();

    foreach (const QString &name, m_actions) {
        QAction *action = a->action(name);

        if (action) {
            actions << action;
        }
    }

    return actions;
}

QSizeF AppletInterface::size() const
{
    return applet()->size();
}

void AppletInterface::setAction(const QString &name, const QString &text, const QString &icon, const QString &shortcut)
{
    Plasma::Applet *a = applet();
    QAction *action = a->action(name);

    if (action) {
        action->setText(text);
    } else {
        action = new QAction(text, this);
        a->addAction(name, action);

        Q_ASSERT(!m_actions.contains(name));
        m_actions.insert(name);

        if (!m_actionSignals) {
            m_actionSignals = new QSignalMapper(this);
            connect(m_actionSignals, SIGNAL(mapped(QString)),
                    m_appletScriptEngine, SLOT(executeAction(QString)));
        }

        connect(action, SIGNAL(triggered()), m_actionSignals, SLOT(map()));
        m_actionSignals->setMapping(action, name);
    }

    action->setIcon(icon.isEmpty() ? QIcon() : KIcon(icon));
    action->setShortcut(shortcut);
    action->setObjectName(name);
}

void AppletInterface::removeAction(const QString &name)
{
    Plasma::Applet *a = applet();
    QAction *action = a->action(name);

    if (action) {
        if (m_actionSignals) {
            m_actionSignals->removeMappings(action);
        }

        delete action;
    }

    m_actions.remove(name);
}

void AppletInterface::resize(qreal w, qreal h)
{
    applet()->resize(w,h);
}

void AppletInterface::setMinimumSize(qreal w, qreal h)
{
    applet()->setMinimumSize(w,h);
}

void AppletInterface::setPreferredSize(qreal w, qreal h)
{
    applet()->setPreferredSize(w,h);
}

void AppletInterface::dataUpdated(QString source, Plasma::DataEngine::Data data)
{
    m_appletScriptEngine->dataUpdated(source, data);
}

Plasma::Applet *AppletInterface::applet() const
{
    return m_appletScriptEngine->applet();
}

#include "appletinterface.moc"
