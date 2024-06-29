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

#include "CoreLocale.h"

#include <QDate>
#include <QDateTime>
#include <QJSEngine>
#include <QTime>
#include <QMetaMethod>

using namespace QtBindings::Core;

Locale::Locale()
{
}

Locale::Locale(const QString &name) : QLocale(name)
{
}

Locale::Locale(QLocale::Language language, QLocale::Country country) : QLocale(language, country)
{
}

Locale::Locale(QLocale::Language language, QLocale::Script script,
                                 QLocale::Country country) : QLocale(language, script, country)
{
}

Locale::Locale(const QLocale &other) : QLocale(other)
{
}

Locale::Locale(const Locale &other) : QObject(), QLocale(other)
{
}

QString Locale::amText() const
{
    return QLocale::amText();
}

QString Locale::bcp47Name() const
{
    return QLocale::bcp47Name();
}

QLocale::Country Locale::country() const
{
    return QLocale::country();
}

QString Locale::createSeparatedList(const QStringList &strl) const
{
    return QLocale::createSeparatedList(strl);
}

QString Locale::currencySymbol(QLocale::CurrencySymbolFormat format) const
{
    return QLocale::currencySymbol(format);
}

QString Locale::dateFormat(QLocale::FormatType format) const
{
    return QLocale::dateFormat(format);
}

QString Locale::dateTimeFormat(QLocale::FormatType format) const
{
    return QLocale::dateTimeFormat(format);
}

QString Locale::dayName(int day, QLocale::FormatType format) const
{
    return QLocale::dayName(day, format);
}

QString Locale::decimalPoint() const
{
    return QLocale::decimalPoint();
}

QString Locale::exponential() const
{
    return QLocale::exponential();
}

Qt::DayOfWeek Locale::firstDayOfWeek() const
{
    return QLocale::firstDayOfWeek();
}

QString Locale::groupSeparator() const
{
    return QLocale::groupSeparator();
}

QLocale::Language Locale::language() const
{
    return QLocale::language();
}

QLocale::MeasurementSystem Locale::measurementSystem() const
{
    return QLocale::measurementSystem();
}

QString Locale::monthName(int month, QLocale::FormatType format) const
{
    return QLocale::monthName(month, format);
}

QString Locale::name() const
{
    return QLocale::name();
}

QString Locale::nativeCountryName() const
{
    return QLocale::nativeCountryName();
}

QString Locale::nativeLanguageName() const
{
    return QLocale::nativeLanguageName();
}

QString Locale::negativeSign() const
{
    return QLocale::negativeSign();
}

QLocale::NumberOptions Locale::numberOptions() const
{
    return QLocale::numberOptions();
}

QString Locale::percent() const
{
    return QLocale::percent();
}

QString Locale::pmText() const
{
    return QLocale::pmText();
}

QString Locale::positiveSign() const
{
    return QLocale::positiveSign();
}

QString Locale::quoteString(const QStringView &str, QLocale::QuotationStyle style) const
{
    return QLocale::quoteString(str.toString(), style);
}

QString Locale::quoteString(const QString &str, QLocale::QuotationStyle style) const
{
    return QLocale::quoteString(str, style);
}

QLocale::Script Locale::script() const
{
    return QLocale::script();
}

void Locale::setNumberOptions(QLocale::NumberOptions options)
{
    QLocale::setNumberOptions(options);
}

QString Locale::standaloneDayName(int day, QLocale::FormatType format) const
{
    return QLocale::standaloneDayName(day, format);
}

QString Locale::standaloneMonthName(int month, QLocale::FormatType format) const
{
    return QLocale::standaloneMonthName(month, format);
}

void Locale::swap(QLocale &other)
{
    QLocale::swap(other);
}

Qt::LayoutDirection Locale::textDirection() const
{
    return QLocale::textDirection();
}

QString Locale::timeFormat(QLocale::FormatType format) const
{
    return QLocale::timeFormat(format);
}

QString Locale::toCurrencyString(double value, const QString &symbol, int precision) const
{
    return QLocale::toCurrencyString(value, symbol, precision);
}

QString Locale::toCurrencyString(double value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(float i, const QString &symbol, int precision) const
{
    return QLocale::toCurrencyString(i, symbol, precision);
}

QString Locale::toCurrencyString(float i, const QString &symbol) const
{
    return QLocale::toCurrencyString(i, symbol);
}

QString Locale::toCurrencyString(int value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(qlonglong value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(qulonglong value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(short value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(uint value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QString Locale::toCurrencyString(ushort value, const QString &symbol) const
{
    return QLocale::toCurrencyString(value, symbol);
}

QDate Locale::toDate(const QString &string, const QString &format) const
{
    return QLocale::toDate(string, format);
}

QDate Locale::toDate(const QString &string, QLocale::FormatType format) const
{
    return QLocale::toDate(string, format);
}

QDateTime Locale::toDateTime(const QString &string, const QString &format) const
{
    return QLocale::toDateTime(string, format);
}

QDateTime Locale::toDateTime(const QString &string, QLocale::FormatType format) const
{
    return QLocale::toDateTime(string, format);
}

double Locale::toDouble(const QStringView &s, bool *ok) const
{
    return QLocale::toDouble(s, ok);
}

double Locale::toDouble(const QString &s, bool *ok) const
{
    return QLocale::toDouble(s, ok);
}

float Locale::toFloat(const QStringView &s, bool *ok) const
{
    return QLocale::toFloat(s, ok);
}

float Locale::toFloat(const QString &s, bool *ok) const
{
    return QLocale::toFloat(s, ok);
}

int Locale::toInt(const QStringView &s, bool *ok) const
{
    return QLocale::toInt(s, ok);
}

int Locale::toInt(const QString &s, bool *ok) const
{
    return QLocale::toInt(s, ok);
}

qlonglong Locale::toLongLong(const QStringView &s, bool *ok) const
{
    return QLocale::toLongLong(s, ok);
}

qlonglong Locale::toLongLong(const QString &s, bool *ok) const
{
    return QLocale::toLongLong(s, ok);
}

QString Locale::toLower(const QString &str) const
{
    return QLocale::toLower(str);
}

short Locale::toShort(const QStringView &s, bool *ok) const
{
    return QLocale::toShort(s, ok);
}

short Locale::toShort(const QString &s, bool *ok) const
{
    return QLocale::toShort(s, ok);
}

QString Locale::toString(const QDate &date, const QString &formatStr) const
{
    return QLocale::toString(date, formatStr);
}

QString Locale::toString(const QDate &date, QLocale::FormatType format) const
{
    return QLocale::toString(date, format);
}

QString Locale::toString(const QDateTime &dateTime, const QString &format) const
{
    return QLocale::toString(dateTime, format);
}

QString Locale::toString(const QDateTime &dateTime, QLocale::FormatType format) const
{
    return QLocale::toString(dateTime, format);
}

QString Locale::toString(const QTime &time, const QString &formatStr) const
{
    return QLocale::toString(time, formatStr);
}

QString Locale::toString(const QTime &time, QLocale::FormatType format) const
{
    return QLocale::toString(time, format);
}

QString Locale::toString(double i, char f, int prec) const
{
    return QLocale::toString(i, f, prec);
}

QString Locale::toString(float i, char f, int prec) const
{
    return QLocale::toString(i, f, prec);
}

QString Locale::toString(int i) const
{
    return QLocale::toString(i);
}

QString Locale::toString(qlonglong i) const
{
    return QLocale::toString(i);
}

QString Locale::toString(qulonglong i) const
{
    return QLocale::toString(i);
}

QString Locale::toString(short i) const
{
    return QLocale::toString(i);
}

QString Locale::toString(uint i) const
{
    return QLocale::toString(i);
}

QString Locale::toString(ushort i) const
{
    return QLocale::toString(i);
}

QTime Locale::toTime(const QString &string, const QString &format) const
{
    return QLocale::toTime(string, format);
}

QTime Locale::toTime(const QString &string, QLocale::FormatType format) const
{
    return QLocale::toTime(string, format);
}

uint Locale::toUInt(const QStringView &s, bool *ok) const
{
    return QLocale::toUInt(s, ok);
}

uint Locale::toUInt(const QString &s, bool *ok) const
{
    return QLocale::toUInt(s, ok);
}

qulonglong Locale::toULongLong(const QStringView &s, bool *ok) const
{
    return QLocale::toULongLong(s, ok);
}

qulonglong Locale::toULongLong(const QString &s, bool *ok) const
{
    return QLocale::toULongLong(s, ok);
}

QString Locale::toUpper(const QString &str) const
{
    return QLocale::toUpper(str);
}

ushort Locale::toUShort(const QStringView &s, bool *ok) const
{
    return QLocale::toUShort(s, ok);
}

ushort Locale::toUShort(const QString &s, bool *ok) const
{
    return QLocale::toUShort(s, ok);
}

QStringList Locale::uiLanguages() const
{
    return QLocale::uiLanguages();
}

QList<Qt::DayOfWeek> Locale::weekdays() const
{
    return QLocale::weekdays();
}

QString Locale::zeroDigit() const
{
    return QLocale::zeroDigit();
}

QList<QLocale::Country> Locale::countriesForLanguage(QLocale::Language lang)
{
    return QLocale::countriesForLanguage(lang);
}

QList<QLocale>
Locale::matchingLocales(QLocale::Language language, QLocale::Script script, QLocale::Country country)
{
    return QLocale::matchingLocales(language,script,country);
}

QLocale Locale::c()
{
    return QLocale::c();
}

Locale Locale::system()
{
    return Locale(QLocale::system());
}

QString Locale::countryToString(QLocale::Country country)
{
    return QLocale::countryToString(country);
}

QString Locale::languageToString(QLocale::Language language)
{
    return QLocale::languageToString(language);
}

QString Locale::scriptToString(QLocale::Script script)
{
    return QLocale::scriptToString(script);
}

void Locale::setDefault(const QLocale &locale)
{
    QLocale::setDefault(locale);
}

Locale &Locale::operator=(const Locale &other)
{
    if (this != &other)
        QLocale::operator=(other);
    return *this;
}
