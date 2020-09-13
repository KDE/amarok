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

#ifndef CORECOREAPPLICATION_H
#define CORECOREAPPLICATION_H

#include "QtBinding.h"
#include "CoreTranslator.h"
#include <QObject>

class QJSEngine;

namespace QtBindings
{
    namespace Core
    {
        /* This class exceptionally does not inherit from its QT respective
         * because its a singleton.
         * */
        class CoreApplication : public QObject, public QtBindings::Base<CoreApplication>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE CoreApplication();
            Q_INVOKABLE CoreApplication(const CoreApplication& other);
            Q_INVOKABLE static bool installTranslator(Translator *messageFile);
            Q_INVOKABLE static QString translate(QString context, QString key);
            CoreApplication &operator=(const CoreApplication &other);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::CoreApplication)
#endif //CORECOREAPPLICATION_H
