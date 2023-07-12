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

#include "CoreResource.h"

#include <QDateTime>

using namespace QtBindings::Core;


Resource::Resource(const QString &file, const QLocale &locale) : QResource(file, locale)
{
}

Resource::Resource(const Resource &resource) : Resource(resource.fileName(), resource.locale())
{
}

QString Resource::absoluteFilePath() const
{
    return QResource::absoluteFilePath();
}

const uchar *Resource::data() const
{
    return QResource::data();
}

QString Resource::fileName() const
{
    return QResource::fileName();
}

QResource::Compression Resource::compressionAlgorithm() const
{
    return QResource::compressionAlgorithm();
}

bool Resource::isValid() const
{
    return QResource::isValid();
}

QDateTime Resource::lastModified() const
{
    return QResource::lastModified();
}

QLocale Resource::locale() const
{
    return QResource::locale();
}

void Resource::setFileName(const QString &file)
{
    QResource::setFileName(file);
}

void Resource::setLocale(const QLocale &locale)
{
    QResource::setLocale(locale);
}

qint64 Resource::size() const
{
    return QResource::size();
}

bool Resource::registerResource(const QString &rccFilename, const QString &resourceRoot)
{
    return QResource::registerResource(rccFilename,resourceRoot);
}

bool Resource::registerResource(const uchar *rccData, const QString &resourceRoot)
{
    return QResource::registerResource(rccData,resourceRoot);
}

bool Resource::unregisterResource(const QString &rccFilename, const QString &resourceRoot)
{
    return QResource::unregisterResource(rccFilename,resourceRoot);
}

bool Resource::unregisterResource(const uchar *rccData, const QString &resourceRoot)
{
    return QResource::unregisterResource(rccData,resourceRoot);
}

Resource &Resource::operator=(const Resource &other)
{
    if (this != &other) {
        this->setFileName( other.fileName() );
        this->setLocale( other.locale() );
    }

    return *this;
}
