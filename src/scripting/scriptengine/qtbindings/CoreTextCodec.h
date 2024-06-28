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

#ifndef CORETEXTCODEC_H
#define CORETEXTCODEC_H

#include "QtBinding.h"
#include "CoreByteArray.h"

#include <QObject>
#include <QTextCodec>

class QJSEngine;

namespace QtBindings
{
    namespace Core
    {
        class TextCodec : public QObject, public QtBindings::Base<TextCodec>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE TextCodec();
            Q_INVOKABLE TextCodec(const QTextCodec* coded);
            Q_INVOKABLE TextCodec(const TextCodec& codec);
            Q_INVOKABLE ~TextCodec();
            Q_INVOKABLE static QList<QByteArray> availableCodecs();
            Q_INVOKABLE static QList<int> availableMibs();
            Q_INVOKABLE static TextCodec codecForHtml(const ByteArray &ba, QTextCodec *defaultCodec);
            Q_INVOKABLE static TextCodec codecForHtml(const ByteArray &ba);
            Q_INVOKABLE static TextCodec codecForLocale();
            Q_INVOKABLE static TextCodec codecForMib(int mib);
            Q_INVOKABLE static TextCodec codecForName(const ByteArray &name);
            Q_INVOKABLE static TextCodec codecForName(const char *name);
            Q_INVOKABLE static TextCodec codecForUtfText(const ByteArray &ba, QTextCodec *defaultCodec);
            Q_INVOKABLE static TextCodec codecForUtfText(const ByteArray &ba);
            Q_INVOKABLE static void setCodecForLocale(QTextCodec *c);
            TextCodec &operator=(const TextCodec &other);
        public Q_SLOTS:
            virtual QList<QByteArray> aliases() const;
            bool canEncode(QChar ch) const;
            bool canEncode(const QString &s) const;
            ByteArray fromUnicode(const QString &str) const;
            ByteArray fromUnicode(const QChar *input, int number, QTextCodec::ConverterState *state = Q_NULLPTR) const;
            /*
            QTextDecoder *makeDecoder(ConversionFlags flags = DefaultConversion) const;
            QTextEncoder *makeEncoder(ConversionFlags flags = DefaultConversion) const;
             */
            virtual int	mibEnum();
            virtual ByteArray name();
            QString toUnicode(const ByteArray &a) const;
            QString toUnicode(const char *chars) const;
            QString toUnicode(const char *input, int size, QTextCodec::ConverterState *state = Q_NULLPTR) const;
        private:
            const QTextCodec *internal;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::TextCodec)
#endif //CORETEXTCODEC_H
