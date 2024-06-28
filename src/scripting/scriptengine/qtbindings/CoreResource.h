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

#ifndef CORERESOURCE_H
#define CORERESOURCE_H

#include "QtBinding.h"

#include <QObject>
#include <QResource>

namespace QtBindings
{
    namespace Core
    {
        class Resource : public QObject, public QResource, public QtBindings::Base<Resource>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE Resource(const QString &file=QString(), const QLocale &locale=QLocale());
            Q_INVOKABLE Resource(const Resource& resource);
            Q_INVOKABLE static bool registerResource(const QString &rccFilename, const QString &resourceRoot=QString());
            Q_INVOKABLE static bool registerResource(const uchar *rccData, const QString &resourceRoot=QString());
            Q_INVOKABLE static bool unregisterResource(const QString &rccFilename, const QString &resourceRoot=QString());
            Q_INVOKABLE static bool unregisterResource(const uchar *rccData, const QString &resourceRoot=QString());
            Resource &operator=(const Resource &other);
        public Q_SLOTS:
            QString absoluteFilePath() const;
            const uchar *data() const;
            QString fileName() const;
            QResource::Compression compressionAlgorithm() const;
            bool isValid() const;
            QDateTime lastModified() const;
            QLocale locale() const;
            void setFileName(const QString &file);
            void setLocale(const QLocale &locale);
            qint64 size() const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::Resource)
#endif //CORERESOURCE_H
