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

#ifndef UITOOLSUILOADER_H
#define UITOOLSUILOADER_H

#include "QtBinding.h"
#include <QFile>
#include <QUiLoader>

namespace QtBindings
{
    namespace UiTools
    {
        class UiLoader : public QUiLoader, public QtBindings::Base<UiLoader>
        {
            Q_OBJECT
        public:
            Q_INVOKABLE UiLoader(QObject *parent = Q_NULLPTR);
            Q_INVOKABLE UiLoader(const UiLoader &other);
            Q_INVOKABLE UiLoader(const QJSValue &other);
            Q_INVOKABLE virtual ~UiLoader();
            UiLoader &operator=(const UiLoader &other);
        public Q_SLOTS:
            void addPluginPath(const QString &path);
            QStringList availableLayouts() const;
            QStringList availableWidgets() const;
            void clearPluginPaths();
            virtual QAction *createAction(QObject *parent = Q_NULLPTR, const QString &name = QString()) override;
            virtual QActionGroup *createActionGroup(QObject *parent = Q_NULLPTR, const QString &name = QString()) override;
            virtual QLayout *createLayout(const QString &className, QObject *parent = Q_NULLPTR, const QString &name = QString()) override;
            virtual QWidget *createWidget(const QString &className, QWidget *parent = Q_NULLPTR, const QString &name = QString()) override;
            QString errorString() const;
            bool isLanguageChangeEnabled() const;
            QJSValue load(QFile *device, QJSValue parentWidget);
            QStringList pluginPaths() const;
            void setLanguageChangeEnabled(bool enabled);
            void setWorkingDirectory(const QDir &dir);
            QDir workingDirectory() const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::UiTools::UiLoader)
#endif //UITOOLSUILOADER_H
