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

#include "CoreTranslator.h"

using namespace QtBindings::Core;

Translator::Translator(QObject *parent) : QTranslator(parent)
{

}

Translator::Translator(const Translator &other) : QTranslator()
{
    Q_UNUSED(other);
}

QString Translator::translate(const char *context, const char *sourceText,
                              const char *disambiguation, int n) const
{
    return QTranslator::translate(context, sourceText, disambiguation, n);
}

bool Translator::isEmpty() const
{
    return QTranslator::isEmpty();
}

bool Translator::load(const QString &filename, const QString &directory,
                      const QString &search_delimiters, const QString &suffix)
{
    return QTranslator::load(filename, directory, search_delimiters, suffix);
}

bool
Translator::load(const QLocale &locale, const QString &filename, const QString &prefix,
                 const QString &directory, const QString &suffix)
{
    return QTranslator::load(locale, filename, prefix, directory, suffix);
}

bool Translator::load(const uchar *data, int len, const QString &directory)
{
    return QTranslator::load(data, len, directory);
}

Translator &Translator::operator=(const Translator &other)
{
    Q_UNUSED(other);
    /* Nothing to do here */
    return *this;
}

