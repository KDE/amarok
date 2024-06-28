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
#ifndef COREDIR_H
#define COREDIR_H

#include "QtBinding.h"

#include <QObject>
#include <QDir>

namespace QtBindings
{
    namespace Core
    {
        class Dir : public QObject, public QDir, public QtBindings::Base<Dir>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE Dir(const QDir &path);
            Q_INVOKABLE Dir(const Dir &path);
            Q_INVOKABLE Dir(const QString &path = QString());
            Q_INVOKABLE Dir(const QString &path, const QString &nameFilter, SortFlags sort = SortFlags(Name | IgnoreCase), Filters filter = AllEntries);
            Q_INVOKABLE static bool isAbsolutePath(const QString &path);
            Q_INVOKABLE static bool isRelativePath(const QString &path);
            Q_INVOKABLE static bool match(const QString &filter, const QString &fileName);
            Q_INVOKABLE static bool match(const QStringList &filters, const QString &fileName);
            Q_INVOKABLE static bool setCurrent(const QString &path);
            Q_INVOKABLE static QChar listSeparator();
            Q_INVOKABLE static QChar separator();
            Q_INVOKABLE static QDir current();
            Q_INVOKABLE static QDir home();
            Q_INVOKABLE static QDir root();
            Q_INVOKABLE static QDir temp();
            Q_INVOKABLE static QFileInfoList drives();
            Q_INVOKABLE static QString cleanPath(const QString &path);
            Q_INVOKABLE static QString currentPath();
            Q_INVOKABLE static QString fromNativeSeparators(const QString &pathName);
            Q_INVOKABLE static QString homePath();
            Q_INVOKABLE static QStringList nameFiltersFromString(const QString &nameFilter);
            Q_INVOKABLE static QStringList searchPaths(const QString &prefix);
            Q_INVOKABLE static QString rootPath();
            Q_INVOKABLE static QString tempPath();
            Q_INVOKABLE static QString toNativeSeparators(const QString &pathName);
            Q_INVOKABLE static void addSearchPath(const QString &prefix, const QString &path);
            Q_INVOKABLE static void setSearchPaths(const QString &prefix, const QStringList &searchPaths);
            Dir &operator=(const Dir &other);
        public Q_SLOTS:
            bool cd(const QString &dirName);
            bool cdUp();
            bool exists() const;
            bool exists(const QString &name) const;
            bool isAbsolute() const { return !isRelative(); }
            bool isEmpty(Filters filters = Filters(AllEntries | NoDotAndDotDot)) const;
            bool isReadable() const;
            bool isRelative() const;
            bool isRoot() const;
            bool makeAbsolute();
            bool mkdir(const QString &dirName) const;
            bool mkpath(const QString &dirPath) const;
            bool remove(const QString &fileName);
            bool removeRecursively();
            bool rename(const QString &oldName, const QString &newName);
            bool rmdir(const QString &dirName) const;
            bool rmpath(const QString &dirPath) const;
            Filters filter() const;
            QFileInfoList entryInfoList(const QStringList &nameFilters, Filters filters = NoFilter, SortFlags sort = NoSort) const;
            QFileInfoList entryInfoList(Filters filters = NoFilter, SortFlags sort = NoSort) const;
            QString absoluteFilePath(const QString &fileName) const;
            QString absolutePath() const;
            QString canonicalPath() const;
            QString dirName() const;
            QString filePath(const QString &fileName) const;
            QStringList entryList(const QStringList &nameFilters, Filters filters = NoFilter, SortFlags sort = NoSort) const;
            QStringList entryList(Filters filters = NoFilter, SortFlags sort = NoSort) const;
            QStringList nameFilters() const;
            QString path() const;
            QString relativeFilePath(const QString &fileName) const;
            SortFlags sorting() const;
            uint count() const;
            void refresh() const;
            void setFilter(Filters filter);
            void setNameFilters(const QStringList &nameFilters);
            void setPath(const QString &path);
            void setSorting(SortFlags sort);
            void swap(QDir &other);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::Dir)
#endif //COREDIR_H
