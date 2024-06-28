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

#ifndef COREFILE_H
#define COREFILE_H

#include "QtBinding.h"
#include "CoreIODevice.h"
#include <QFile>

namespace QtBindings
{
    namespace Core
    {
        class File : public QFile, public QtBindings::Base<File>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE File();
            Q_INVOKABLE File(const File &file);
            Q_INVOKABLE File(const QString &name, QObject *parent);
            Q_INVOKABLE File(QObject *parent);
            Q_INVOKABLE File(const QString &name);
            Q_INVOKABLE virtual ~File();;
            Q_INVOKABLE static bool copy(const QString &fileName, const QString &newName);
            Q_INVOKABLE static QString decodeName(const char *localFileName);
            Q_INVOKABLE static QString decodeName(const QByteArray &localFileName);
            Q_INVOKABLE static QByteArray encodeName(const QString &fileName);
            Q_INVOKABLE static bool exists(const QString &fileName);
            Q_INVOKABLE static bool link(const QString &fileName, const QString &linkName);
            Q_INVOKABLE static QFileDevice::Permissions permissions(const QString &fileName);
            Q_INVOKABLE static bool remove(const QString &fileName);
            Q_INVOKABLE static bool rename(const QString &oldName, const QString &newName);
            Q_INVOKABLE static bool resize(const QString &fileName, qint64 sz);
            Q_INVOKABLE static bool setPermissions(const QString &fileName, QFileDevice::Permissions permissions);
            Q_INVOKABLE static QString symLinkTarget(const QString &fileName);
            File &operator=(const File &other);
            // Supress warnings about overloading virtual QFile::open
            using QFile::open;
        public Q_SLOTS:
            Q_INVOKABLE virtual void close() override;
            Q_INVOKABLE bool copy(const QString &newName);
            Q_INVOKABLE bool exists() const;
            Q_INVOKABLE virtual QString fileName() const override;
            Q_INVOKABLE bool link(const QString &linkName);
            //Q_INVOKABLE bool open(FILE *fh, QIODevice::OpenMode mode, QFileDevice::FileHandleFlags handleFlags = DontCloseHandle);
            //Q_INVOKABLE bool open(int fd, QIODevice::OpenMode mode, QFileDevice::FileHandleFlags handleFlags = DontCloseHandle);
            //Q_INVOKABLE virtual bool open(QIODevice::OpenMode mode) override;
            Q_INVOKABLE virtual bool open(QtBindings::Core::IODevice::OpenModeFlag mode);
            Q_INVOKABLE virtual QFileDevice::Permissions permissions() const override;
            Q_INVOKABLE bool remove();
            Q_INVOKABLE bool rename(const QString &newName);
            Q_INVOKABLE virtual bool resize(qint64 sz) override;
            Q_INVOKABLE void setFileName(const QString &name);
            Q_INVOKABLE virtual bool setPermissions(QFileDevice::Permissions permissions) override;
            Q_INVOKABLE virtual qint64 size() const override;
            Q_INVOKABLE QString symLinkTarget() const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::File)
#endif //COREFILE_H
