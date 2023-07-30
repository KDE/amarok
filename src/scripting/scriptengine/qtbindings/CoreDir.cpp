/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedro.gomes@ipsoft.com>
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

#include "CoreDir.h"

using namespace QtBindings::Core;

Dir::Dir(const QDir &path) : QDir(path)
{
}

Dir::Dir(const Dir &path) : QObject(), QDir(path)
{
}

Dir::Dir(const QString &path) : QDir(path)
{
}

Dir::Dir(const QString &path, const QString &nameFilter,
        QDir::SortFlags sort, QDir::Filters filter) : QDir(path, nameFilter, sort, filter)
{
}

bool Dir::cd(const QString &dirName)
{
    return QDir::cd(dirName);
}

bool Dir::cdUp()
{
    return QDir::cdUp();
}

bool Dir::exists() const
{
    return QDir::exists();
}

bool Dir::exists(const QString &name) const
{
    return QDir::exists(name);
}

bool Dir::isEmpty(QDir::Filters filters) const
{
    return QDir::isEmpty(filters);
}

bool Dir::isReadable() const
{
    return QDir::isReadable();
}

bool Dir::isRelative() const
{
    return QDir::isRelative();
}

bool Dir::isRoot() const
{
    return QDir::isRoot();
}

bool Dir::makeAbsolute()
{
    return QDir::makeAbsolute();
}

bool Dir::mkdir(const QString &dirName) const
{
    return QDir::mkdir(dirName);
}

bool Dir::mkpath(const QString &dirPath) const
{
    return QDir::mkpath(dirPath);
}

bool Dir::remove(const QString &fileName)
{
    return QDir::remove(fileName);
}

bool Dir::removeRecursively()
{
    return QDir::removeRecursively();
}

bool Dir::rename(const QString &oldName, const QString &newName)
{
    return QDir::rename(oldName, newName);
}

bool Dir::rmdir(const QString &dirName) const
{
    return QDir::rmdir(dirName);
}

bool Dir::rmpath(const QString &dirPath) const
{
    return QDir::rmpath(dirPath);
}

QDir::Filters Dir::filter() const
{
    return QDir::filter();
}

QFileInfoList Dir::entryInfoList(const QStringList &nameFilters, QDir::Filters filters, QDir::SortFlags sort) const
{
    return QDir::entryInfoList(nameFilters, filters, sort);
}

QFileInfoList Dir::entryInfoList(QDir::Filters filters, QDir::SortFlags sort) const
{
    return QDir::entryInfoList(filters, sort);
}

QString Dir::absoluteFilePath(const QString &fileName) const
{
    return QDir::absoluteFilePath(fileName);
}

QString Dir::absolutePath() const
{
    return QDir::absolutePath();
}

QString Dir::canonicalPath() const
{
    return QDir::canonicalPath();
}

QString Dir::dirName() const
{
    return QDir::dirName();
}

QString Dir::filePath(const QString &fileName) const
{
    return QDir::filePath(fileName);
}

QStringList Dir::entryList(const QStringList &nameFilters, QDir::Filters filters, QDir::SortFlags sort) const
{
    return QDir::entryList(nameFilters, filters, sort);
}

QStringList Dir::entryList(QDir::Filters filters, QDir::SortFlags sort) const
{
    return QDir::entryList(filters, sort);
}

QStringList Dir::nameFilters() const
{
    return QDir::nameFilters();
}

QString Dir::path() const
{
    return QDir::path();
}

QString Dir::relativeFilePath(const QString &fileName) const
{
    return QDir::relativeFilePath(fileName);
}

QDir::SortFlags Dir::sorting() const
{
    return QDir::sorting();
}

uint Dir::count() const
{
    return QDir::count();
}

void Dir::refresh() const
{
    QDir::refresh();
}

void Dir::setFilter(QDir::Filters filter)
{
    QDir::setFilter(filter);
}

void Dir::setNameFilters(const QStringList &nameFilters)
{
    QDir::setNameFilters(nameFilters);
}

void Dir::setPath(const QString &path)
{
    QDir::setPath(path);
}

void Dir::setSorting(QDir::SortFlags sort)
{
    QDir::setSorting(sort);
}

void Dir::swap(QDir &other)
{
    QDir::swap(other);
}

bool Dir::isAbsolutePath(const QString &path)
{
    return QDir::isAbsolutePath(path);
}

bool Dir::isRelativePath(const QString &path)
{
    return QDir::isRelativePath(path);
}

bool Dir::match(const QString &filter, const QString &fileName)
{
    return QDir::match(filter,fileName);
}

bool Dir::match(const QStringList &filters, const QString &fileName)
{
    return QDir::match(filters,fileName);
}

bool Dir::setCurrent(const QString &path)
{
    return QDir::setCurrent(path);
}

QChar Dir::listSeparator()
{
    return QDir::listSeparator();
}

QChar Dir::separator()
{
    return QDir::separator();
}

QDir Dir::current()
{
    return QDir::current();
}

QDir Dir::home()
{
    return QDir::home();
}

QDir Dir::root()
{
    return QDir::root();
}

QDir Dir::temp()
{
    return QDir::temp();
}

QFileInfoList Dir::drives()
{
    return QDir::drives();
}

QString Dir::cleanPath(const QString &path)
{
    return QDir::cleanPath(path);
}

QString Dir::currentPath()
{
    return QDir::currentPath();
}

QString Dir::fromNativeSeparators(const QString &pathName)
{
    return QDir::fromNativeSeparators(pathName);
}

QString Dir::homePath()
{
    return QDir::homePath();
}

QStringList Dir::nameFiltersFromString(const QString &nameFilter)
{
    return QDir::nameFiltersFromString(nameFilter);
}

QStringList Dir::searchPaths(const QString &prefix)
{
    return QDir::searchPaths(prefix);
}

QString Dir::rootPath()
{
    return QDir::rootPath();
}

QString Dir::tempPath()
{
    return QDir::tempPath();
}

QString Dir::toNativeSeparators(const QString &pathName)
{
    return QDir::toNativeSeparators(pathName);
}

void Dir::addSearchPath(const QString &prefix, const QString &path)
{
    QDir::addSearchPath(prefix,path);
}

void Dir::setSearchPaths(const QString &prefix, const QStringList &searchPaths)
{
    QDir::setSearchPaths(prefix,searchPaths);
}

Dir &Dir::operator=(const Dir &other)
{
    if (this != &other) {
       QDir::operator=( other );
    }
    return *this;
}
