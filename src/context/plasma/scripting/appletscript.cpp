/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "scripting/appletscript.h"

#include "applet.h"
#include "package.h"

namespace Plasma
{

class AppletScriptPrivate
{
public:
    Applet *applet;
};

AppletScript::AppletScript(QObject *parent)
    : ScriptEngine(parent),
      d(new AppletScriptPrivate)
{
    d->applet = 0;
}

AppletScript::~AppletScript()
{
    delete d;
}

void AppletScript::setApplet(Plasma::Applet *applet)
{
    d->applet = applet;
}

Applet *AppletScript::applet() const
{
    Q_ASSERT(d->applet);
    return d->applet;
}

void AppletScript::paintInterface(QPainter *painter,
                                  const QStyleOptionGraphicsItem *option,
                                  const QRect &contentsRect)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(contentsRect);
}

QSizeF AppletScript::size() const
{
    if (applet()) {
        return applet()->size();
    }

    return QSizeF();
}

void AppletScript::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints);
}

QList<QAction*> AppletScript::contextualActions()
{
    return QList<QAction*>();
}

QPainterPath AppletScript::shape() const
{
    if (applet()) {
        QPainterPath path;
        path.addRect(applet()->boundingRect());
        return path;
    }

    return QPainterPath();
}

void AppletScript::setHasConfigurationInterface(bool hasInterface)
{
    Q_UNUSED(hasInterface)
    if (applet()) {
        applet()->setHasConfigurationInterface(true);
    }
}

void AppletScript::showConfigurationInterface()
{
}

void AppletScript::configChanged()
{
}

DataEngine *AppletScript::dataEngine(const QString &engine) const
{
    Q_ASSERT(d->applet);
    return d->applet->dataEngine(engine);
}

QString AppletScript::mainScript() const
{
    Q_ASSERT(d->applet);
    return d->applet->package()->filePath("mainscript");
}

const Package *AppletScript::package() const
{
    Q_ASSERT(d->applet);
    return d->applet->package();
}

} // Plasma namespace

#include "appletscript.moc"
