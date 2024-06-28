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

#ifndef CORETEXTSTREAM_H
#define CORETEXTSTREAM_H

#include "QtBinding.h"

#include <QObject>
#include <QTextStream>

namespace QtBindings
{
    namespace Core
    {
        class TextStream : public QObject, public QTextStream, public QtBindings::Base<TextStream>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE TextStream();
            Q_INVOKABLE TextStream(const TextStream &textStream);
            Q_INVOKABLE TextStream(QIODevice *device);
            Q_INVOKABLE TextStream(FILE *fileHandle, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
            Q_INVOKABLE TextStream(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
            Q_INVOKABLE TextStream(QByteArray *array, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
            Q_INVOKABLE TextStream(const QByteArray &array, QIODevice::OpenMode openMode = QIODevice::ReadOnly);
            TextStream &operator=(const TextStream &other);
        public Q_SLOTS:
            bool atEnd() const;
            bool autoDetectUnicode() const;
            QTextCodec *codec() const;
            QIODevice *device() const;
            FieldAlignment fieldAlignment() const;
            int fieldWidth() const;
            void flush();
            bool generateByteOrderMark() const;
            int integerBase() const;
            QLocale locale() const;
            NumberFlags numberFlags() const;
            QChar padChar() const;
            qint64 pos() const;
            QString readAll();
            bool readLineInto(QString *line, qint64 maxlen = 0);
            QString readLine(qint64 maxlen = 0);
            QString read(qint64 maxlen);
            RealNumberNotation realNumberNotation() const;
            int realNumberPrecision() const;
            void reset();
            void resetStatus();
            bool seek(qint64 pos);
            void setAutoDetectUnicode(bool enabled);
            void setCodec(const char *codecName);
            void setCodec(QTextCodec *codec);
            void setDevice(QIODevice *device);
            void setFieldAlignment(FieldAlignment alignment);
            void setFieldWidth(int width);
            void setGenerateByteOrderMark(bool generate);
            void setIntegerBase(int base);
            void setLocale(const QLocale &locale);
            void setNumberFlags(NumberFlags flags);
            void setPadChar(QChar ch);
            void setRealNumberNotation(RealNumberNotation notation);
            void setRealNumberPrecision(int precision);
            void setStatus(Status status);
            void setString(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
            void skipWhiteSpace();
            Status status() const;
            QString *string() const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::TextStream)
#endif //CORETEXTSTREAM_H
