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

#ifndef COREFILEINFO_H
#define COREFILEINFO_H

#include "QtBinding.h"

#include <QObject>
#include <QFileInfo>

namespace QtBindings
{
    namespace Core
    {
        class FileInfo : public QObject, public QFileInfo, public QtBindings::Base<FileInfo>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE FileInfo();
            Q_INVOKABLE FileInfo(const QString &file);
            Q_INVOKABLE FileInfo(const QFile &file);
            Q_INVOKABLE FileInfo(const QDir &dir, const QString &file);
            Q_INVOKABLE FileInfo(const QFileInfo &fileinfo);
            Q_INVOKABLE FileInfo(const FileInfo &fileInfo);
            Q_INVOKABLE FileInfo(QFileInfoPrivate *d);
            Q_INVOKABLE static bool exists(const QString &file);
            FileInfo &operator=(const FileInfo &other);
        public Q_SLOTS:
            QDir absoluteDir() const;
            QString absoluteFilePath() const;
            QString absolutePath() const;
            QString baseName() const;
            QString bundleName() const;
            bool caching() const;
            QString canonicalFilePath() const;
            QString canonicalPath() const;
            QString completeBaseName() const;
            QString completeSuffix() const;
            QDateTime birthTime() const;
            QDateTime metadataChangeTime() const;
            QDir dir() const;
            bool exists() const;
            QString fileName() const;
            QString filePath() const;
            QString group() const;
            uint groupId() const;
            bool isAbsolute() const;
            bool isBundle() const;
            bool isDir() const;
            bool isExecutable() const;
            bool isFile() const;
            bool isHidden() const;
            bool isNativePath() const;
            bool isReadable() const;
            bool isRelative() const;
            bool isRoot() const;
            bool isSymLink() const;
            bool isWritable() const;
            QDateTime lastModified() const;
            QDateTime lastRead() const;
            bool makeAbsolute();
            QString owner() const;
            uint ownerId() const;
            QString path() const;
            bool permission(QFile::Permissions permissions) const;
            QFile::Permissions permissions() const;
            void refresh();
            void setCaching(bool on);
            void setFile(const QDir &dir, const QString &file);
            void setFile(const QFile &file);
            void setFile(const QString &file);
            qint64 size() const;
            QString suffix() const;
            void swap(QFileInfo &other);
            QString symLinkTarget() const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::FileInfo)
#endif //COREFILEINFO_H
