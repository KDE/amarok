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

#ifndef CORELOCALE_H
#define CORELOCALE_H

#include "QtBinding.h"

#include <QObject>
#include <QLocale>

class QJSEngine;

namespace QtBindings
{
    namespace Core
    {
        class Locale : public QObject, public QLocale, public QtBindings::Base<Locale>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE Locale();
            Q_INVOKABLE Locale(const QString &name);
            Q_INVOKABLE Locale(Language language, Country country = AnyCountry);
            Q_INVOKABLE Locale(Language language, Script script, Country country);
            Q_INVOKABLE Locale(const QLocale &other);
            Q_INVOKABLE Locale(const Locale &other);
            Q_INVOKABLE static QLocale c();
            Q_INVOKABLE static QString countryToString(Country country);
            Q_INVOKABLE static QString languageToString(Language language);
            Q_INVOKABLE static QString scriptToString(Script script);
            Q_INVOKABLE static void setDefault(const QLocale &locale);
            Q_INVOKABLE static Locale system();
            Locale &operator=(const Locale& other);
        public Q_SLOTS:
            QString amText() const;
            QString bcp47Name() const;
            Country country() const;
            QString createSeparatedList(const QStringList &strl) const;
            QString currencySymbol(CurrencySymbolFormat format = CurrencySymbol) const;
            QString dateFormat(FormatType format = LongFormat) const;
            QString dateTimeFormat(FormatType format = LongFormat) const;
            QString dayName(int day, FormatType format = LongFormat) const;
            QChar decimalPoint() const;
            QChar exponential() const;
            Qt::DayOfWeek firstDayOfWeek() const;
            QChar groupSeparator() const;
            Language language() const;
            MeasurementSystem measurementSystem() const;
            QString monthName(int month, FormatType format = LongFormat) const;
            QString name() const;
            QString nativeCountryName() const;
            QString nativeLanguageName() const;
            QChar negativeSign() const;
            NumberOptions numberOptions() const;
            QChar percent() const;
            QString pmText() const;
            QChar positiveSign() const;
            QString quoteString(const QStringView &str, QuotationStyle style = StandardQuotation) const;
            QString quoteString(const QString &str, QuotationStyle style = StandardQuotation) const;
            Script script() const;
            void setNumberOptions(NumberOptions options);
            QString standaloneDayName(int day, FormatType format = LongFormat) const;
            QString standaloneMonthName(int month, FormatType format = LongFormat) const;
            void swap(QLocale &other);
            Qt::LayoutDirection textDirection() const;
            QString timeFormat(FormatType format = LongFormat) const;
            QString toCurrencyString(double value, const QString &symbol, int precision) const;
            QString toCurrencyString(double value, const QString &symbol = QString()) const;
            QString toCurrencyString(float i, const QString &symbol, int precision) const;
            QString toCurrencyString(float i, const QString &symbol = QString()) const;
            QString toCurrencyString(int value, const QString &symbol = QString()) const;
            QString toCurrencyString(qlonglong value, const QString &symbol = QString()) const;
            QString toCurrencyString(qulonglong value, const QString &symbol = QString()) const;
            QString toCurrencyString(short value, const QString &symbol = QString()) const;
            QString toCurrencyString(uint value, const QString &symbol = QString()) const;
            QString toCurrencyString(ushort value, const QString &symbol = QString()) const;
            QDate toDate(const QString &string, const QString &format) const;
            QDate toDate(const QString &string, FormatType format = LongFormat) const;
            QDateTime toDateTime(const QString &string, const QString &format) const;
            QDateTime toDateTime(const QString &string, FormatType format = LongFormat) const;
            double toDouble(const QStringView &s, bool *ok = Q_NULLPTR) const;
            double toDouble(const QString &s, bool *ok = Q_NULLPTR) const;
            float toFloat(const QStringView &s, bool *ok = Q_NULLPTR) const;
            float toFloat(const QString &s, bool *ok = Q_NULLPTR) const;
            int toInt(const QStringView &s, bool *ok = Q_NULLPTR) const;
            int toInt(const QString &s, bool *ok = Q_NULLPTR) const;
            qlonglong toLongLong(const QStringView &s, bool *ok = Q_NULLPTR) const;
            qlonglong toLongLong(const QString &s, bool *ok = Q_NULLPTR) const;
            QString toLower(const QString &str) const;
            short toShort(const QStringView &s, bool *ok = Q_NULLPTR) const;
            short toShort(const QString &s, bool *ok = Q_NULLPTR) const;
            QString toString(const QDate &date, const QString &formatStr) const;
            QString toString(const QDate &date, FormatType format = LongFormat) const;
            QString toString(const QDateTime &dateTime, const QString &format) const;
            QString toString(const QDateTime &dateTime, FormatType format = LongFormat) const;
            QString toString(const QTime &time, const QString &formatStr) const;
            QString toString(const QTime &time, FormatType format = LongFormat) const;
            QString toString(double i, char f = 'g', int prec = 6) const;
            QString toString(float i, char f = 'g', int prec = 6) const;
            QString toString(int i) const;
            QString toString(qlonglong i) const;
            QString toString(qulonglong i) const;
            QString toString(short i) const;
            QString toString(uint i) const;
            QString toString(ushort i) const;
            QTime toTime(const QString &string, const QString &format) const;
            QTime toTime(const QString &string, FormatType format = LongFormat) const;
            uint toUInt(const QStringView &s, bool *ok = Q_NULLPTR) const;
            uint toUInt(const QString &s, bool *ok = Q_NULLPTR) const;
            qulonglong toULongLong(const QStringView &s, bool *ok = Q_NULLPTR) const;
            qulonglong toULongLong(const QString &s, bool *ok = Q_NULLPTR) const;
            QString toUpper(const QString &str) const;
            ushort toUShort(const QStringView &s, bool *ok = Q_NULLPTR) const;
            ushort toUShort(const QString &s, bool *ok = Q_NULLPTR) const;
            QStringList uiLanguages() const;
            QList<Qt::DayOfWeek> weekdays() const;
            QChar zeroDigit() const;
            QList<Country> countriesForLanguage(Language lang);
            QList<QLocale> matchingLocales(QLocale::Language language, QLocale::Script script, QLocale::Country country);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::Locale)
#endif //CORELOCALE_H
