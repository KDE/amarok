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

#include "CoreCoreApplication.h"

#include <QCoreApplication>

using namespace QtBindings::Core;

CoreApplication::CoreApplication()
{
}

CoreApplication::CoreApplication(const CoreApplication &other) : QObject()
{
    Q_UNUSED(other);
}

bool CoreApplication::installTranslator(Translator *messageFile)
{
    return QCoreApplication::installTranslator(messageFile);
}

QString CoreApplication::translate(QString context, QString key)
{
    return QCoreApplication::translate(context.toLocal8Bit().constData(),
            key.toLocal8Bit().constData());
}

CoreApplication &CoreApplication::operator=(const CoreApplication &other)
{
    /* Nothing to do here */
    return *this;
}
