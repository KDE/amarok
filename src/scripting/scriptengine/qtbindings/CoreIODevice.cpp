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

#include "CoreIODevice.h"

using namespace QtBindings::Core;

IODevice::IODevice()
{
}

IODevice::IODevice(const IODevice &other) : QObject()
{
    Q_UNUSED(other)
}

IODevice::~IODevice()
{
}

IODevice &IODevice::operator=(const IODevice &other)
{
    Q_UNUSED(other)
    return *this;
}

void IODevice::installJSType( QJSEngine *engine )
{

    Base<IODevice>::installJSType( engine );
    engine->globalObject().property( QStringLiteral("QIODevice") ).setProperty(
            QStringLiteral("OpenMode"), engine->newQMetaObject<OpenMode>() );
    qRegisterMetaType<OpenMode>("QIODevice::OpenMode");
    qRegisterMetaType<QtBindings::Core::IODevice::OpenModeFlag>();
}

OpenMode::OpenMode() : QIODevice::OpenMode()
{
}

OpenMode::OpenMode(const OpenMode& other) : QObject(), QIODevice::OpenMode( other )
{
}

OpenMode::OpenMode(QIODevice::OpenMode flags) : QIODevice::OpenMode( flags )
{
}

OpenMode::~OpenMode()
{
}

OpenMode::operator Int() {
    return QIODevice::OpenMode::operator Int();
}
