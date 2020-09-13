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

#ifndef COREIODEVICE_H
#define COREIODEVICE_H

#include "QtBinding.h"

#include <QIODevice>

namespace QtBindings
{
    namespace Core
    {
        class IODevice : public QObject, public QtBindings::Base<IODevice>
        {
        Q_OBJECT
        public:
            enum OpenModeFlag {
                NotOpen = QIODevice::NotOpen,
                ReadOnly = QIODevice::ReadOnly,
                WriteOnly = QIODevice::WriteOnly,
                ReadWrite = QIODevice::ReadOnly | WriteOnly,
                Append = QIODevice::Append,
                Truncate = QIODevice::Truncate,
                Text = QIODevice::Text,
                Unbuffered = QIODevice::Unbuffered,
                NewOnly = QIODevice::NewOnly,
                ExistingOnly = QIODevice::ExistingOnly
            };
            Q_FLAG(OpenModeFlag);
            IODevice();
            IODevice(const IODevice &other);
            ~IODevice();
            IODevice &operator=(const IODevice &other);
            static void installJSType( QJSEngine *engine );
        };

        /* MOC does not support nested classes - Had to hack it out */
        class OpenMode : public QObject, public QIODevice::OpenMode
        {
        Q_OBJECT
        public:
            Q_INVOKABLE OpenMode();
            Q_INVOKABLE OpenMode(const OpenMode& other);
            Q_INVOKABLE OpenMode(QIODevice::OpenMode flags);
            Q_INVOKABLE ~OpenMode();
            Q_INVOKABLE operator Int();
            OpenMode &operator=(const OpenMode& other);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::IODevice)
Q_DECLARE_METATYPE(QtBindings::Core::OpenMode)

#endif //COREIODEVICE_H
