/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UiToolsUiLoader.h"

#include "CoreDir.h"

using namespace QtBindings::UiTools;

UiLoader::UiLoader(QObject *parent) : QUiLoader(parent)
{
}

UiLoader::UiLoader(const UiLoader &other) : QUiLoader(other.parent())
{
    *this=other;
}

UiLoader::~UiLoader()
{
}

void UiLoader::addPluginPath(const QString &path)
{
    QUiLoader::addPluginPath(path);
}

QStringList UiLoader::availableLayouts() const
{
    return QUiLoader::availableLayouts();
}

QStringList UiLoader::availableWidgets() const
{
    return QUiLoader::availableWidgets();
}

void UiLoader::clearPluginPaths()
{
    QUiLoader::clearPluginPaths();
}

QAction *UiLoader::createAction(QObject *parent, const QString &name)
{
    return QUiLoader::createAction(parent, name);
}

QActionGroup *
UiLoader::createActionGroup(QObject *parent, const QString &name)
{
    return QUiLoader::createActionGroup(parent, name);
}

QLayout *
UiLoader::createLayout(const QString &className, QObject *parent,
                                            const QString &name)
{
    return QUiLoader::createLayout(className, parent, name);
}

QWidget *
UiLoader::createWidget(const QString &className, QWidget *parent,
                                            const QString &name)
{
    return QUiLoader::createWidget(className, parent, name);
}

QString UiLoader::errorString() const
{
    return QUiLoader::errorString();
}

bool UiLoader::isLanguageChangeEnabled() const
{
    return QUiLoader::isLanguageChangeEnabled();
}

QWidget *UiLoader::load(QIODevice *device, QWidget *parentWidget)
{
    return QUiLoader::load(device, parentWidget);
}

QStringList UiLoader::pluginPaths() const
{
    return QUiLoader::pluginPaths();
}

void UiLoader::setLanguageChangeEnabled(bool enabled)
{
    QUiLoader::setLanguageChangeEnabled(enabled);
}

void UiLoader::setWorkingDirectory(const QDir &dir)
{
    QUiLoader::setWorkingDirectory(dir);
}

QDir UiLoader::workingDirectory() const
{
    return QtBindings::Core::Dir(QUiLoader::workingDirectory());
}

UiLoader &UiLoader::operator=(const UiLoader &other)
{
    if (this != &other) {
        this->setLanguageChangeEnabled( other.isLanguageChangeEnabled() );
        this->setWorkingDirectory( other.workingDirectory() );
    }
    return *this;
}

