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
#ifndef COREBYTEARRAY_H 
#define COREBYTEARRAY_H

#include "QtBinding.h"

#include <QObject>
#include <QByteArray>

class QJSEngine;

namespace QtBindings
{
    namespace Core
    {
        class ByteArray : public QObject, public QByteArray, public QtBindings::Base<ByteArray>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE ByteArray();
            Q_INVOKABLE explicit ByteArray(const char *data, int size = -1);
            Q_INVOKABLE ByteArray(int size, char ch);
            Q_INVOKABLE explicit ByteArray(const QByteArray &other);
            Q_INVOKABLE ByteArray(const ByteArray &other);
            Q_INVOKABLE ByteArray(ByteArray &&other) noexcept;
            Q_INVOKABLE explicit ByteArray(const QString &other);
            Q_INVOKABLE static ByteArray fromBase64(const QByteArray &base64);
            Q_INVOKABLE static ByteArray fromBase64(const QByteArray &base64, QByteArray::Base64Options options);
            Q_INVOKABLE static ByteArray fromHex(const QByteArray &hexEncoded);
            Q_INVOKABLE static ByteArray fromPercentEncoding(const QByteArray &input, char percent = '%');
            Q_INVOKABLE static ByteArray fromRawData(const char *data, int size);
            Q_INVOKABLE static ByteArray fromStdString(const std::string &str);
            Q_INVOKABLE static ByteArray number(int n, int base = 10);
            Q_INVOKABLE static ByteArray number(uint n, int base = 10);
            Q_INVOKABLE static ByteArray number(qlonglong n, int base = 10);
            Q_INVOKABLE static ByteArray number(qulonglong n, int base = 10);
            Q_INVOKABLE static ByteArray number(double n, char f = 'g', int prec = 6);
            ByteArray &operator=(const ByteArray &other);
        public Q_SLOTS:
            ByteArray &append(const QByteArray &ba);
            ByteArray &append(int count, char ch);
            ByteArray &append(const char *str);
            ByteArray &append(const char *str, int len);
            ByteArray &append(char ch);
            char at(int i) const;
            QByteArray::iterator begin();
            QByteArray::const_iterator begin() const;
            int capacity() const;
            QByteArray::const_iterator cbegin() const;
            QByteArray::const_iterator cend() const;
            void chop(int n);
            void clear();
            QByteArray::const_iterator constBegin() const;
            const char *constData() const;
            QByteArray::const_iterator constEnd() const;
            bool contains(const QByteArray &ba) const;
            bool contains(const char *str) const;
            bool contains(char ch) const;
            int count(const QByteArray &ba) const;
            int count(const char *str) const;
            int count(char ch) const;
            int count() const;
            QByteArray::const_reverse_iterator crbegin() const;
            QByteArray::const_reverse_iterator crend() const;
            char *data();
            const char *data() const;
            QByteArray::iterator end();
            QByteArray::const_iterator end() const;
            bool endsWith(const QByteArray &ba) const;
            bool endsWith(char ch) const;
            bool endsWith(const char *str) const;
            QByteArray &fill(char ch, int size = -1);
            int indexOf(const QByteArray &ba, int from = 0) const;
            int indexOf(const char *str, int from = 0) const;
            int indexOf(char ch, int from = 0) const;
            ByteArray &insert(int i, const QByteArray &ba);
            ByteArray &insert(int i, int count, char ch);
            ByteArray &insert(int i, const char *str);
            ByteArray &insert(int i, const char *str, int len);
            ByteArray &insert(int i, char ch);
            bool isEmpty() const;
            bool isNull() const;
            int lastIndexOf(const QByteArray &ba, int from = -1) const;
            int lastIndexOf(const char *str, int from = -1) const;
            int lastIndexOf(char ch, int from = -1) const;
            QByteArray left(int len) const;
            QByteArray leftJustified(int width, char fill = ' ', bool truncate = false) const;
            int length() const;
            QByteArray mid(int pos, int len = -1) const;
            ByteArray &prepend(const QByteArray &ba);
            ByteArray &prepend(int count, char ch);
            ByteArray &prepend(const char *str);
            ByteArray &prepend(const char *str, int len);
            ByteArray &prepend(char ch);
            void push_back(const QByteArray &other);
            void push_back(const char *str);
            void push_back(char ch);
            void push_front(const QByteArray &other);
            void push_front(const char *str);
            void push_front(char ch);
            QByteArray::reverse_iterator rbegin();
            QByteArray::const_reverse_iterator rbegin() const;
            QByteArray &remove(int pos, int len);
            QByteArray::reverse_iterator rend();
            QByteArray::const_reverse_iterator rend() const;
            QByteArray repeated(int times) const;
            QByteArray &replace(int pos, int len, const QByteArray &after);
            QByteArray &replace(int pos, int len, const char *after, int alen);
            QByteArray &replace(int pos, int len, const char *after);
            QByteArray &replace(char before, const char *after);
            QByteArray &replace(char before, const QByteArray &after);
            QByteArray &replace(const char *before, const char *after);
            QByteArray &replace(const char *before, int bsize, const char *after, int asize);
            QByteArray &replace(const QByteArray &before, const QByteArray &after);
            QByteArray &replace(const QByteArray &before, const char *after);
            QByteArray &replace(const char *before, const QByteArray &after);
            QByteArray &replace(char before, char after);
            void reserve(int size);
            void resize(int size);
            QByteArray right(int len) const;
            QByteArray rightJustified(int width, char fill = ' ', bool truncate = false) const;
            QByteArray &setNum(int n, int base = 10);
            QByteArray &setNum(ushort n, int base = 10);
            QByteArray &setNum(short n, int base = 10);
            QByteArray &setNum(uint n, int base = 10);
            QByteArray &setNum(qlonglong n, int base = 10);
            QByteArray &setNum(qulonglong n, int base = 10);
            QByteArray &setNum(float n, char f = 'g', int prec = 6);
            QByteArray &setNum(double n, char f = 'g', int prec = 6);
            QByteArray &setRawData(const char *data, uint size);
            QByteArray simplified() const;
            int size() const;
            QList<QByteArray> split(char sep) const;
            void squeeze();
            bool startsWith(const QByteArray &ba) const;
            bool startsWith(char ch) const;
            bool startsWith(const char *str) const;
            void swap(QByteArray &other);
            QByteArray toBase64() const;
            QByteArray toBase64(QByteArray::Base64Options options) const;
            double toDouble(bool *ok = Q_NULLPTR) const;
            float toFloat(bool *ok = Q_NULLPTR) const;
            QByteArray toHex() const;
            QByteArray toHex(char separator) const;
            int toInt(bool *ok = Q_NULLPTR, int base = 10) const;
            long toLong(bool *ok = Q_NULLPTR, int base = 10) const;
            qlonglong toLongLong(bool *ok = Q_NULLPTR, int base = 10) const;
            QByteArray toLower() const;
            QByteArray toPercentEncoding(const QByteArray &exclude = QByteArray(), const QByteArray &include = QByteArray(), char percent = '%') const;
            short toShort(bool *ok = Q_NULLPTR, int base = 10) const;
            std::string toStdString() const;
            uint toUInt(bool *ok = Q_NULLPTR, int base = 10) const;
            ulong toULong(bool *ok = Q_NULLPTR, int base = 10) const;
            qulonglong toULongLong(bool *ok = Q_NULLPTR, int base = 10) const;
            ushort toUShort(bool *ok = Q_NULLPTR, int base = 10) const;
            QByteArray toUpper() const;
            QByteArray trimmed() const;
            void truncate(int pos);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::ByteArray)
#endif // COREBYTEARRAY_H
