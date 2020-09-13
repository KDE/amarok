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

#include "CoreFileInfo.h"

#include <QDateTime>
#include <QDir>

using namespace QtBindings::Core;

FileInfo::FileInfo()
{
}

FileInfo::FileInfo(const QString &file) : QFileInfo(file)
{
}

FileInfo::FileInfo(const QFile &file) : QFileInfo(file)
{
}

FileInfo::FileInfo(const QDir &dir, const QString &file) : QFileInfo(dir, file)
{
}

FileInfo::FileInfo(const QFileInfo &fileinfo) : QFileInfo(fileinfo)
{
}

FileInfo::FileInfo(const FileInfo &fileinfo) : QObject(), QFileInfo(fileinfo)
{
}

FileInfo::FileInfo(QFileInfoPrivate *d) : QFileInfo(d)
{
}

QDir FileInfo::absoluteDir() const
{
    return QFileInfo::absoluteDir();
}

QString FileInfo::absoluteFilePath() const
{
    return QFileInfo::absoluteFilePath();
}

QString FileInfo::absolutePath() const
{
    return QFileInfo::absolutePath();
}

QString FileInfo::baseName() const
{
    return QFileInfo::baseName();
}

QString FileInfo::bundleName() const
{
    return QFileInfo::bundleName();
}

bool FileInfo::caching() const
{
    return QFileInfo::caching();
}

QString FileInfo::canonicalFilePath() const
{
    return QFileInfo::canonicalFilePath();
}

QString FileInfo::canonicalPath() const
{
    return QFileInfo::canonicalPath();
}

QString FileInfo::completeBaseName() const
{
    return QFileInfo::completeBaseName();
}

QString FileInfo::completeSuffix() const
{
    return QFileInfo::completeSuffix();
}

QDateTime FileInfo::created() const
{
    return QFileInfo::created();
}

QDir FileInfo::dir() const
{
    return QFileInfo::dir();
}

bool FileInfo::exists() const
{
    return QFileInfo::exists();
}

QString FileInfo::fileName() const
{
    return QFileInfo::fileName();
}

QString FileInfo::filePath() const
{
    return QFileInfo::filePath();
}

QString FileInfo::group() const
{
    return QFileInfo::group();
}

uint FileInfo::groupId() const
{
    return QFileInfo::groupId();
}

bool FileInfo::isAbsolute() const
{
    return QFileInfo::isAbsolute();
}

bool FileInfo::isBundle() const
{
    return QFileInfo::isBundle();
}

bool FileInfo::isDir() const
{
    return QFileInfo::isDir();
}

bool FileInfo::isExecutable() const
{
    return QFileInfo::isExecutable();
}

bool FileInfo::isFile() const
{
    return QFileInfo::isFile();
}

bool FileInfo::isHidden() const
{
    return QFileInfo::isHidden();
}

bool FileInfo::isNativePath() const
{
    return QFileInfo::isNativePath();
}

bool FileInfo::isReadable() const
{
    return QFileInfo::isReadable();
}

bool FileInfo::isRelative() const
{
    return QFileInfo::isRelative();
}

bool FileInfo::isRoot() const
{
    return QFileInfo::isRoot();
}

bool FileInfo::isSymLink() const
{
    return QFileInfo::isSymLink();
}

bool FileInfo::isWritable() const
{
    return QFileInfo::isWritable();
}

QDateTime FileInfo::lastModified() const
{
    return QFileInfo::lastModified();
}

QDateTime FileInfo::lastRead() const
{
    return QFileInfo::lastRead();
}

bool FileInfo::makeAbsolute()
{
    return QFileInfo::makeAbsolute();
}

QString FileInfo::owner() const
{
    return QFileInfo::owner();
}

uint FileInfo::ownerId() const
{
    return QFileInfo::ownerId();
}

QString FileInfo::path() const
{
    return QFileInfo::path();
}

bool FileInfo::permission(QFile::Permissions permissions) const
{
    return QFileInfo::permission(permissions);
}

QFile::Permissions FileInfo::permissions() const
{
    return QFileInfo::permissions();
}

QString FileInfo::readLink() const
{
    return QFileInfo::readLink();
}

void FileInfo::refresh()
{
    QFileInfo::refresh();
}

void FileInfo::setCaching(bool on)
{
    QFileInfo::setCaching(on);
}

void FileInfo::setFile(const QDir &dir, const QString &file)
{
    QFileInfo::setFile(dir, file);
}

void FileInfo::setFile(const QFile &file)
{
    QFileInfo::setFile(file);
}

void FileInfo::setFile(const QString &file)
{
    QFileInfo::setFile(file);
}

qint64 FileInfo::size() const
{
    return QFileInfo::size();
}

QString FileInfo::suffix() const
{
    return QFileInfo::suffix();
}

void FileInfo::swap(QFileInfo &other)
{
    QFileInfo::swap(other);
}

QString FileInfo::symLinkTarget() const
{
    return QFileInfo::symLinkTarget();
}

bool FileInfo::exists(const QString &file)
{
    return QFileInfo::exists(file);
}

FileInfo &FileInfo::operator=(const FileInfo &other)
{
    if (this != &other)
        QFileInfo::operator=(other);
    return *this;
}
