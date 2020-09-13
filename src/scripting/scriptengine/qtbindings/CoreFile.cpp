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

#include "CoreFile.h"

using namespace QtBindings::Core;

File::File()
{
}

File::File(const File &file) : QFile()
{
    *this = file;
}

File::File(const QString &name, QObject *parent) : QFile(name, parent)
{
}

File::File(QObject *parent) : QFile(parent)
{
}

File::File(const QString &name) : QFile(name)
{
}

File::~File()
{
}

void File::close()
{
    QFile::close();
}

bool File::copy(const QString &fileName, const QString &newName)
{
    return QFile::copy(fileName,newName);
}

QString File::decodeName(const char *localFileName)
{
    return QFile::decodeName(localFileName);
}

QString File::decodeName(const QByteArray &localFileName)
{
    return QFile::decodeName(localFileName);
}

QByteArray File::encodeName(const QString &fileName)
{
    return QFile::encodeName(fileName);
}

bool File::exists(const QString &fileName)
{
    return QFile::exists(fileName);
}

bool File::link(const QString &fileName, const QString &linkName)
{
    return QFile::link(fileName,linkName);
}

QFileDevice::Permissions File::permissions(const QString &fileName)
{
    return QFile::permissions(fileName);
}

bool File::remove(const QString &fileName)
{
    return QFile::remove(fileName);
}

bool File::rename(const QString &oldName, const QString &newName)
{
    return QFile::rename(oldName,newName);
}

bool File::resize(const QString &fileName, qint64 sz)
{
    return QFile::resize(fileName,sz);
}

bool File::setPermissions(const QString &fileName,
                                            QFileDevice::Permissions permissions)
{
    return QFile::setPermissions(fileName,permissions);
}

QString File::symLinkTarget(const QString &fileName)
{
    return QFile::symLinkTarget(fileName);
}

bool File::copy(const QString &newName)
{
    return QFile::copy(newName);
}

bool File::exists() const
{
    return QFile::exists();
}

QString File::fileName() const
{
    return QFile::fileName();
}

bool File::link(const QString &linkName)
{
    return QFile::link(linkName);
}
/*
bool File::open(FILE *fh, QIODevice::OpenMode mode,
                                  QFileDevice::FileHandleFlags handleFlags)
{
    return QFile::open(fh, mode, handleFlags);
}

bool File::open(int fd, QIODevice::OpenMode mode,
                                  QFileDevice::FileHandleFlags handleFlags)
{
    return QFile::open(fd, mode, handleFlags);
}

bool File::open(QIODevice::OpenMode mode)
{
    return QFile::open(mode);
}
*/

bool File::open(QtBindings::Core::IODevice::OpenModeFlag mode)
{
    return QFile::open( QIODevice::OpenMode(mode) );
}

QFileDevice::Permissions File::permissions() const
{
    return QFile::permissions();
}

bool File::remove()
{
    return QFile::remove();
}

bool File::rename(const QString &newName)
{
    return QFile::rename(newName);
}

bool File::resize(qint64 sz)
{
    return QFile::resize(sz);
}

void File::setFileName(const QString &name)
{
    QFile::setFileName(name);
}

bool File::setPermissions(QFileDevice::Permissions permissions)
{
    return QFile::setPermissions(permissions);
}

qint64 File::size() const
{
    return QFile::size();
}

QString File::symLinkTarget() const
{
    return QFile::symLinkTarget();
}

File &File::operator=(const File &other)
{
    if (this != &other ) {
        this->setFileName(other.fileName());
        this->setPermissions(other.permissions());
        this->setCurrentReadChannel(other.currentReadChannel());
        this->setCurrentWriteChannel(other.currentWriteChannel());
        this->setTextModeEnabled(other.isTextModeEnabled());
        this->setErrorString(other.errorString());
        this->setOpenMode(other.openMode());
    }
    return *this;
}
