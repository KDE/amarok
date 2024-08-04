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

#include "CoreTextStream.h"

using namespace QtBindings::Core;

TextStream::TextStream()
{
}

TextStream::TextStream(const TextStream &textStream) : QObject(), QTextStream(textStream.device())
{
}

TextStream::TextStream(QIODevice *device) : QTextStream(device)
{
}

TextStream::TextStream(FILE *fileHandle, QIODevice::OpenMode openMode) : QTextStream(fileHandle, openMode)
{
}

TextStream::TextStream(QString *string, QIODevice::OpenMode openMode) : QTextStream(string, openMode)
{
}

TextStream::TextStream(QByteArray *array, QIODevice::OpenMode openMode) : QTextStream(array, openMode)
{
}

TextStream::TextStream(const QByteArray &array, QIODevice::OpenMode openMode) : QTextStream(array, openMode)
{
}

bool TextStream::atEnd() const
{
    return QTextStream::atEnd();
}

bool TextStream::autoDetectUnicode() const
{
    return QTextStream::autoDetectUnicode();
}

QIODevice *TextStream::device() const
{
    return QTextStream::device();
}

QTextStream::FieldAlignment TextStream::fieldAlignment() const
{
    return QTextStream::fieldAlignment();
}

int TextStream::fieldWidth() const
{
    return QTextStream::fieldWidth();
}

void TextStream::flush()
{
    QTextStream::flush();
}

bool TextStream::generateByteOrderMark() const
{
    return QTextStream::generateByteOrderMark();
}

int TextStream::integerBase() const
{
    return QTextStream::integerBase();
}

QLocale TextStream::locale() const
{
    return QTextStream::locale();
}

QTextStream::NumberFlags TextStream::numberFlags() const
{
    return QTextStream::numberFlags();
}

QChar TextStream::padChar() const
{
    return QTextStream::padChar();
}

qint64 TextStream::pos() const
{
    return QTextStream::pos();
}

QString TextStream::readAll()
{
    return QTextStream::readAll();
}

bool TextStream::readLineInto(QString *line, qint64 maxlen)
{
    return QTextStream::readLineInto(line, maxlen);
}

QString TextStream::readLine(qint64 maxlen)
{
    return QTextStream::readLine(maxlen);
}

QString TextStream::read(qint64 maxlen)
{
    return QTextStream::read(maxlen);
}

QTextStream::RealNumberNotation TextStream::realNumberNotation() const
{
    return QTextStream::realNumberNotation();
}

int TextStream::realNumberPrecision() const
{
    return QTextStream::realNumberPrecision();
}

void TextStream::reset()
{
    QTextStream::reset();
}

void TextStream::resetStatus()
{
    QTextStream::resetStatus();
}

bool TextStream::seek(qint64 pos)
{
    return QTextStream::seek(pos);
}

void TextStream::setAutoDetectUnicode(bool enabled)
{
    QTextStream::setAutoDetectUnicode(enabled);
}

void TextStream::setDevice(QIODevice *device)
{
    QTextStream::setDevice(device);
}

void TextStream::setFieldAlignment(QTextStream::FieldAlignment alignment)
{
    QTextStream::setFieldAlignment(alignment);
}

void TextStream::setFieldWidth(int width)
{
    QTextStream::setFieldWidth(width);
}

void TextStream::setGenerateByteOrderMark(bool generate)
{
    QTextStream::setGenerateByteOrderMark(generate);
}

void TextStream::setIntegerBase(int base)
{
    QTextStream::setIntegerBase(base);
}

void TextStream::setLocale(const QLocale &locale)
{
    QTextStream::setLocale(locale);
}

void TextStream::setNumberFlags(QTextStream::NumberFlags flags)
{
    QTextStream::setNumberFlags(flags);
}

void TextStream::setPadChar(QChar ch)
{
    QTextStream::setPadChar(ch);
}

void TextStream::setRealNumberNotation(QTextStream::RealNumberNotation notation)
{
    QTextStream::setRealNumberNotation(notation);
}

void TextStream::setRealNumberPrecision(int precision)
{
    QTextStream::setRealNumberPrecision(precision);
}

void TextStream::setStatus(QTextStream::Status status)
{
    QTextStream::setStatus(status);
}

void TextStream::setString(QString *string, QIODevice::OpenMode openMode)
{
    QTextStream::setString(string, openMode);
}

void TextStream::skipWhiteSpace()
{
    QTextStream::skipWhiteSpace();
}

QTextStream::Status TextStream::status() const
{
    return QTextStream::status();
}

QString *TextStream::string() const
{
    return QTextStream::string();
}

TextStream &TextStream::operator=(const TextStream &other)
{
    if (this != &other) {
        this->setAutoDetectUnicode( other.autoDetectUnicode() );
        this->setDevice( other.device() );
        this->setFieldAlignment( other.fieldAlignment() );
        this->setFieldWidth( other.fieldWidth() );
        this->setGenerateByteOrderMark( other.generateByteOrderMark() );
        this->setIntegerBase( other.integerBase() );
        this->setLocale( other.locale() );
        this->setNumberFlags( other.numberFlags() );
        this->setPadChar( other.padChar() );
        this->setRealNumberNotation( other.realNumberNotation() );
        this->setRealNumberPrecision( other.realNumberPrecision() );
        this->setStatus( other.status() );
        this->setString( other.string() );
    }
    return *this;
}
