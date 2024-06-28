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

#ifndef CORETRANSLATOR_H
#define CORETRANSLATOR_H

#include "QtBinding.h"

#include <QTranslator>

namespace QtBindings
{
    namespace Core
    {
        class Translator : public QTranslator, public QtBindings::Base<Translator>
        {
        Q_OBJECT

        public:
            Q_INVOKABLE Translator(QObject *parent = Q_NULLPTR);
            Q_INVOKABLE Translator(const Translator &other);
            Translator &operator=(const Translator& other);
        public Q_SLOTS:
            virtual QString translate(const char *context, const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) const override ;
            virtual bool isEmpty() const override;
            bool load(const QString & filename, const QString & directory = QString(), const QString & search_delimiters = QString(), const QString & suffix = QString());
            bool load(const QLocale & locale, const QString & filename, const QString & prefix = QString(), const QString & directory = QString(), const QString & suffix = QString());
            bool load(const uchar *data, int len, const QString &directory = QString());
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::Translator)
#endif //CORETRANSLATOR_H
