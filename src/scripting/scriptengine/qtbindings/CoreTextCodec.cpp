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

#include "CoreTextCodec.h"

#include <QJSEngine>

#include <iostream>

using namespace QtBindings::Core;

TextCodec::TextCodec() : TextCodec( nullptr )
{
}

TextCodec::TextCodec(const QTextCodec *codec)
{
    if ( codec != nullptr ) {
        internal = codec;
    } else {
        internal = QTextCodec::codecForLocale();
    }
}

TextCodec::TextCodec(const TextCodec &codec) : TextCodec( dynamic_cast<const QTextCodec*>(&codec) )
{
}

TextCodec::~TextCodec()
{
}

QList<QByteArray> TextCodec::aliases() const
{
    return internal->aliases();
}

bool TextCodec::canEncode(QChar ch) const
{
    return internal->canEncode(ch);
}

bool TextCodec::canEncode(const QString &s) const
{
    return internal->canEncode(s);
}

ByteArray TextCodec::fromUnicode(const QString &str) const
{
    return ByteArray( internal->fromUnicode(str) );
}

ByteArray
TextCodec::fromUnicode(const QChar *input, int number, QTextCodec::ConverterState *state) const
{
    return ByteArray( internal->fromUnicode(input, number, state) );
}

/*
QTextDecoder *TextCodec::makeDecoder(QTextCodec::ConversionFlags flags) const
{
    return internal->makeDecoder(flags);
}

QTextEncoder *TextCodec::makeEncoder(QTextCodec::ConversionFlags flags) const
{
    return internal->makeEncoder(flags);
}
*/

int TextCodec::mibEnum()
{
    return internal->mibEnum();
}

ByteArray TextCodec::name()
{
    return ByteArray( internal->name() );
}

QString TextCodec::toUnicode(const ByteArray &a) const
{
    return internal->toUnicode(a);
}

QString TextCodec::toUnicode(const char *chars) const
{
    return internal->toUnicode(chars);
}

QString
TextCodec::toUnicode(const char *input, int size, QTextCodec::ConverterState *state) const
{
    return internal->toUnicode(input, size, state);
}

QList<QByteArray> TextCodec::availableCodecs()
{
    return QTextCodec::availableCodecs();
}

QList<int> TextCodec::availableMibs()
{
    return QTextCodec::availableMibs();
}

TextCodec TextCodec::codecForHtml(const ByteArray &ba, QTextCodec *defaultCodec)
{
     return TextCodec( QTextCodec::codecForHtml(ba,defaultCodec) );
}

TextCodec TextCodec::codecForHtml(const ByteArray &ba)
{
    return QTextCodec::codecForHtml(ba);
}

TextCodec TextCodec::codecForLocale()
{
    return QTextCodec::codecForLocale();
}

TextCodec TextCodec::codecForMib(int mib)
{
    return QTextCodec::codecForMib(mib);
}

TextCodec TextCodec::codecForName(const ByteArray &name)
{
    std::cout << "CodecForName: " << name.toStdString() << std::endl;
    return QTextCodec::codecForName(name);
}
/*
TextCodec TextCodec::codecForName(const char *name)
{
    return QTextCodec::codecForName(name);
}
*/
TextCodec TextCodec::codecForUtfText(const ByteArray &ba, QTextCodec *defaultCodec)
{
    return QTextCodec::codecForUtfText(ba,defaultCodec);
}

TextCodec TextCodec::codecForUtfText(const ByteArray &ba)
{
    return QTextCodec::codecForUtfText(ba);
}

void TextCodec::setCodecForLocale(QTextCodec *c)
{
    QTextCodec::setCodecForLocale(c);
}

TextCodec &TextCodec::operator=(const TextCodec &other)
{
    if (this != &other) {
        internal = other.internal;
    }
    return *this;
}
