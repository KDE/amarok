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

#include "CoreByteArray.h"

#include <QJSEngine>

using namespace QtBindings::Core;

ByteArray::ByteArray()
{
}

ByteArray::ByteArray(const char *data, int size) : QByteArray(data, size)
{
}

ByteArray::ByteArray(int size, char ch) : QByteArray(size, ch)
{
}

ByteArray::ByteArray(const QByteArray &other) : QObject(), QByteArray(other)
{
}

ByteArray::ByteArray(const ByteArray &other) : QObject(), QByteArray(other)
{
}

ByteArray::ByteArray(ByteArray &&other) noexcept : QByteArray(other)
{
}

ByteArray::ByteArray(const QString &other) : QByteArray( other.toLatin1() )
{
}

ByteArray &ByteArray::append(const QByteArray &ba)
{
    QByteArray::append(ba);
    return *this;
}

ByteArray &ByteArray::append(int count, char ch)
{
    QByteArray::append(count, ch);
    return *this;
}

ByteArray &ByteArray::append(const char *str)
{
    QByteArray::append(str);
    return *this;
}

ByteArray &ByteArray::append(const char *str, int len)
{
    QByteArray::append(str, len);
    return *this;
}

ByteArray &ByteArray::append(char ch)
{
    QByteArray::append(ch);
    return *this;
}

ByteArray &ByteArray::append(const QString &str)
{
    QByteArray::append(str.toUtf8());
    return *this;
}

char ByteArray::at(int i) const
{
    return QByteArray::at(i);
}

QByteArray::iterator ByteArray::begin()
{
    return QByteArray::begin();
}

QByteArray::const_iterator ByteArray::begin() const
{
    return QByteArray::begin();
}

int ByteArray::capacity() const
{
    return QByteArray::capacity();
}

QByteArray::const_iterator ByteArray::cbegin() const
{
    return QByteArray::cbegin();
}

QByteArray::const_iterator ByteArray::cend() const
{
    return QByteArray::cend();
}

void ByteArray::chop(int n)
{
    QByteArray::chop(n);
}

void ByteArray::clear()
{
    QByteArray::clear();
}

QByteArray::const_iterator ByteArray::constBegin() const
{
    return QByteArray::constBegin();
}

const char *ByteArray::constData() const
{
    return QByteArray::constData();
}

QByteArray::const_iterator ByteArray::constEnd() const
{
    return QByteArray::constEnd();
}

bool ByteArray::contains(const QByteArray &ba) const
{
    return QByteArray::contains(ba);
}

bool ByteArray::contains(const char *str) const
{
    return QByteArray::contains(str);
}

bool ByteArray::contains(char ch) const
{
    return QByteArray::contains(ch);
}

int ByteArray::count(const QByteArray &ba) const
{
    return QByteArray::count(ba);
}

int ByteArray::count(const char *str) const
{
    return QByteArray::count(str);
}

int ByteArray::count(char ch) const
{
    return QByteArray::count(ch);
}

int ByteArray::count() const
{
    return QByteArray::count();
}

QByteArray::const_reverse_iterator ByteArray::crbegin() const
{
    return QByteArray::crbegin();
}

QByteArray::const_reverse_iterator ByteArray::crend() const
{
    return QByteArray::crend();
}

char *ByteArray::data()
{
    return QByteArray::data();
}

const char *ByteArray::data() const
{
    return QByteArray::data();
}

QByteArray::iterator ByteArray::end()
{
    return QByteArray::end();
}

QByteArray::const_iterator ByteArray::end() const
{
    return QByteArray::end();
}

bool ByteArray::endsWith(const QByteArray &ba) const
{
    return QByteArray::endsWith(ba);
}

bool ByteArray::endsWith(char ch) const
{
    return QByteArray::endsWith(ch);
}

bool ByteArray::endsWith(const char *str) const
{
    return QByteArray::endsWith(str);
}

QByteArray &ByteArray::fill(char ch, int size)
{
    return QByteArray::fill(ch, size);
}

int ByteArray::indexOf(const QByteArray &ba, int from) const
{
    return QByteArray::indexOf(ba, from);
}

int ByteArray::indexOf(const char *str, int from) const
{
    return QByteArray::indexOf(str, from);
}

int ByteArray::indexOf(char ch, int from) const
{
    return QByteArray::indexOf(ch, from);
}

int ByteArray::indexOf(const QString &str, int from) const
{
    return QByteArray::indexOf(str.toUtf8(), from);
}

ByteArray &ByteArray::insert(int i, const QByteArray &ba)
{
    QByteArray::insert(i, ba);
    return *this;
}

ByteArray &ByteArray::insert(int i, int count, char ch)
{
    QByteArray::insert(i, count, ch);
    return *this;
}

ByteArray &ByteArray::insert(int i, const char *str)
{
    QByteArray::insert(i, str);
    return *this;
}

ByteArray &ByteArray::insert(int i, const char *str, int len)
{
    QByteArray::insert(i, str, len);
    return *this;
}

ByteArray &ByteArray::insert(int i, char ch)
{
    QByteArray::insert(i, ch);
    return *this;
}

ByteArray &ByteArray::insert(int i, const QString &str)
{
    QByteArray::insert(i, str.toUtf8());
    return *this;
}

bool ByteArray::isEmpty() const
{
    return QByteArray::isEmpty();
}

bool ByteArray::isNull() const
{
    return QByteArray::isNull();
}

int ByteArray::lastIndexOf(const QByteArray &ba, int from) const
{
    return QByteArray::lastIndexOf(ba, from);
}

int ByteArray::lastIndexOf(const char *str, int from) const
{
    return QByteArray::lastIndexOf(str, from);
}

int ByteArray::lastIndexOf(char ch, int from) const
{
    return QByteArray::lastIndexOf(ch, from);
}

int ByteArray::lastIndexOf(const QString &str, int from) const
{
    return QByteArray::lastIndexOf(str.toUtf8(), from);
}

QByteArray ByteArray::left(int len) const
{
    return QByteArray::left(len);
}

QByteArray ByteArray::leftJustified(int width, char fill, bool truncate) const
{
    return QByteArray::leftJustified(width, fill, truncate);
}

int ByteArray::length() const
{
    return QByteArray::length();
}

QByteArray ByteArray::mid(int pos, int len) const
{
    return QByteArray::mid(pos, len);
}

ByteArray &ByteArray::prepend(const QByteArray &ba)
{
    QByteArray::prepend(ba);
    return *this;
}

ByteArray &ByteArray::prepend(int count, char ch)
{
    QByteArray::prepend(count, ch);
    return *this;
}

ByteArray &ByteArray::prepend(const char *str)
{
    QByteArray::prepend(str);
    return *this;
}

ByteArray &ByteArray::prepend(const char *str, int len)
{
    QByteArray::prepend(str, len);
    return *this;
}

ByteArray &ByteArray::prepend(char ch)
{
    QByteArray::prepend(ch);
    return *this;
}

void ByteArray::push_back(const QByteArray &other)
{
    QByteArray::push_back(other);
}

void ByteArray::push_back(const char *str)
{
    QByteArray::push_back(str);
}

void ByteArray::push_back(char ch)
{
    QByteArray::push_back(ch);
}

void ByteArray::push_front(const QByteArray &other)
{
    QByteArray::push_front(other);
}

void ByteArray::push_front(const char *str)
{
    QByteArray::push_front(str);
}

void ByteArray::push_front(char ch)
{
    QByteArray::push_front(ch);
}

QByteArray::reverse_iterator ByteArray::rbegin()
{
    return QByteArray::rbegin();
}

QByteArray::const_reverse_iterator ByteArray::rbegin() const
{
    return QByteArray::rbegin();
}

QByteArray &ByteArray::remove(int pos, int len)
{
    return QByteArray::remove(pos, len);
}

QByteArray::reverse_iterator ByteArray::rend()
{
    return QByteArray::rend();
}

QByteArray::const_reverse_iterator ByteArray::rend() const
{
    return QByteArray::rend();
}

QByteArray ByteArray::repeated(int times) const
{
    return QByteArray::repeated(times);
}

QByteArray &ByteArray::replace(int pos, int len, const QByteArray &after)
{
    return QByteArray::replace(pos, len, after);
}

QByteArray &ByteArray::replace(int pos, int len, const char *after, int alen)
{
    return QByteArray::replace(pos, len, after, alen);
}

QByteArray &ByteArray::replace(int pos, int len, const char *after)
{
    return QByteArray::replace(pos, len, after);
}

QByteArray &ByteArray::replace(char before, const char *after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(char before, const QByteArray &after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(const char *before, const char *after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(const char *before, int bsize, const char *after, int asize)
{
    return QByteArray::replace(before, bsize, after, asize);
}

QByteArray &ByteArray::replace(const QByteArray &before, const QByteArray &after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(const QByteArray &before, const char *after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(const char *before, const QByteArray &after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(char before, char after)
{
    return QByteArray::replace(before, after);
}

QByteArray &ByteArray::replace(const QString &before, const char *after)
{
    return QByteArray::replace(before.toUtf8(), after);
}

QByteArray &ByteArray::replace(char before, const QString &after)
{
    return QByteArray::replace(before, after.toUtf8());
}

QByteArray &ByteArray::replace(const QString &before, const QByteArray &after)
{
    return QByteArray::replace(before.toUtf8(), after);
}

void ByteArray::reserve(int size)
{
    QByteArray::reserve(size);
}

void ByteArray::resize(int size)
{
    QByteArray::resize(size);
}

QByteArray ByteArray::right(int len) const
{
    return QByteArray::right(len);
}

QByteArray ByteArray::rightJustified(int width, char fill, bool truncate) const
{
    return QByteArray::rightJustified(width, fill, truncate);
}

QByteArray &ByteArray::setNum(int n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(ushort n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(short n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(uint n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(qlonglong n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(qulonglong n, int base)
{
    return QByteArray::setNum(n, base);
}

QByteArray &ByteArray::setNum(float n, char f, int prec)
{
    return QByteArray::setNum(n, f, prec);
}

QByteArray &ByteArray::setNum(double n, char f, int prec)
{
    return QByteArray::setNum(n, f, prec);
}

QByteArray &ByteArray::setRawData(const char *data, uint size)
{
    return QByteArray::setRawData(data, size);
}

QByteArray ByteArray::simplified() const
{
    return QByteArray::simplified();
}

int ByteArray::size() const
{
    return QByteArray::size();
}

QList<QByteArray> ByteArray::split(char sep) const
{
    return QByteArray::split(sep);
}

void ByteArray::squeeze()
{
    QByteArray::squeeze();
}

bool ByteArray::startsWith(const QByteArray &ba) const
{
    return QByteArray::startsWith(ba);
}

bool ByteArray::startsWith(char ch) const
{
    return QByteArray::startsWith(ch);
}

bool ByteArray::startsWith(const char *str) const
{
    return QByteArray::startsWith(str);
}

void ByteArray::swap(QByteArray &other)
{
    QByteArray::swap(other);
}

QByteArray ByteArray::toBase64() const
{
    return QByteArray::toBase64();
}

QByteArray ByteArray::toBase64(QByteArray::Base64Options options) const
{
    return QByteArray::toBase64(options);
}

double ByteArray::toDouble(bool *ok) const
{
    return QByteArray::toDouble(ok);
}

float ByteArray::toFloat(bool *ok) const
{
    return QByteArray::toFloat(ok);
}

QByteArray ByteArray::toHex() const
{
    return QByteArray::toHex();
}

QByteArray ByteArray::toHex(char separator) const
{
    return QByteArray::toHex(separator);
}

int ByteArray::toInt(bool *ok, int base) const
{
    return QByteArray::toInt(ok, base);
}

long ByteArray::toLong(bool *ok, int base) const
{
    return QByteArray::toLong(ok, base);
}

qlonglong ByteArray::toLongLong(bool *ok, int base) const
{
    return QByteArray::toLongLong(ok, base);
}

QByteArray ByteArray::toLower() const
{
    return QByteArray::toLower();
}

QByteArray ByteArray::toPercentEncoding(const QByteArray &exclude, const QByteArray &include, char percent) const
{
    return QByteArray::toPercentEncoding(exclude, include, percent);
}

short ByteArray::toShort(bool *ok, int base) const
{
    return QByteArray::toShort(ok, base);
}

std::string ByteArray::toStdString() const
{
    return QByteArray::toStdString();
}

uint ByteArray::toUInt(bool *ok, int base) const
{
    return QByteArray::toUInt(ok, base);
}

ulong ByteArray::toULong(bool *ok, int base) const
{
    return QByteArray::toULong(ok, base);
}

qulonglong ByteArray::toULongLong(bool *ok, int base) const
{
    return QByteArray::toULongLong(ok, base);
}

ushort ByteArray::toUShort(bool *ok, int base) const
{
    return QByteArray::toUShort(ok, base);
}

QByteArray ByteArray::toUpper() const
{
    return QByteArray::toUpper();
}

QByteArray ByteArray::trimmed() const
{
    return QByteArray::trimmed();
}

void ByteArray::truncate(int pos)
{
    QByteArray::truncate(pos);
}

ByteArray ByteArray::fromBase64(const QByteArray &base64)
{
    return ByteArray( QByteArray::fromBase64(base64) );
}

ByteArray ByteArray::fromBase64(const QByteArray &base64, QByteArray::Base64Options options)
{
    return ByteArray( QByteArray::fromBase64(base64,options) );
}

ByteArray ByteArray::fromHex(const QByteArray &hexEncoded)
{
    return ByteArray( QByteArray::fromHex(hexEncoded) );
}

ByteArray ByteArray::fromPercentEncoding(const QByteArray &input, char percent)
{
    return ByteArray( QByteArray::fromPercentEncoding(input,percent) );
}

ByteArray ByteArray::fromRawData(const char *data, int size)
{
    return ByteArray( QByteArray::fromRawData(data,size) );
}

ByteArray ByteArray::fromStdString(const std::string &str)
{
    return ByteArray( QByteArray::fromStdString(str) );
}

ByteArray ByteArray::number(int n, int base)
{
    return ByteArray( QByteArray::number(n,base) );
}

ByteArray ByteArray::number(uint n, int base)
{
    return ByteArray( QByteArray::number(n,base) );
}

ByteArray ByteArray::number(qlonglong n, int base)
{
    return ByteArray( QByteArray::number(n,base) );
}

ByteArray ByteArray::number(qulonglong n, int base)
{
    return ByteArray( QByteArray::number(n,base) );
}

ByteArray ByteArray::number(double n, char f, int prec)
{
    return ByteArray( QByteArray::number(n,f,prec) );
}

ByteArray &ByteArray::operator=(const ByteArray &other)
{
    if (this != &other)
        QByteArray::operator=(other);
    return *this;
}
