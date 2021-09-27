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

#include "CoreUrl.h"

using namespace QtBindings::Core;

Url::Url()
{
}

Url::Url(const QUrl &copy) : QUrl(copy)
{
}

Url::Url(const Url &copy) : QObject(), QUrl(copy)
{
}

Url::Url(const QString &url, QUrl::ParsingMode mode) : QUrl(url, mode)
{
}

Url::Url(QUrl &&other) : QUrl(other)
{
}

QUrl Url::adjusted(QUrl::FormattingOptions options) const
{
    return QUrl::adjusted(options);
}

QString Url::authority(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::authority(options);
}

void Url::clear()
{
    QUrl::clear();
}

void Url::detach()
{
    QUrl::detach();
}

QString Url::errorString() const
{
    return QUrl::errorString();
}

QString Url::fileName(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::fileName(options);
}

QString Url::fragment(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::fragment(options);
}

bool Url::hasFragment() const
{
    return QUrl::hasFragment();
}

bool Url::hasQuery() const
{
    return QUrl::hasQuery();
}

QString Url::host(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::host(options);
}

bool Url::isDetached() const
{
    return QUrl::isDetached();
}

bool Url::isEmpty() const
{
    return QUrl::isEmpty();
}

bool Url::isLocalFile() const
{
    return QUrl::isLocalFile();
}

bool Url::isParentOf(const QUrl &url) const
{
    return QUrl::isParentOf(url);
}

bool Url::isRelative() const
{
    return QUrl::isRelative();
}

bool Url::isValid() const
{
    return QUrl::isValid();
}

bool Url::matches(const QUrl &url, QUrl::FormattingOptions options) const
{
    return QUrl::matches(url, options);
}

QString Url::password(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::password(options);
}

QString Url::path(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::path(options);
}

int Url::port(int defaultPort) const
{
    return QUrl::port(defaultPort);
}

QString Url::query(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::query(options);
}

QUrl Url::resolved(const QUrl &relative) const
{
    return QUrl::resolved(relative);
}

QString Url::scheme() const
{
    return QUrl::scheme();
}

void Url::setAuthority(const QString &authority, QUrl::ParsingMode mode)
{
    QUrl::setAuthority(authority, mode);
}

void Url::setFragment(const QString &fragment, QUrl::ParsingMode mode)
{
    QUrl::setFragment(fragment, mode);
}

void Url::setHost(const QString &host, QUrl::ParsingMode mode)
{
    QUrl::setHost(host, mode);
}

void Url::setPassword(const QString &password, QUrl::ParsingMode mode)
{
    QUrl::setPassword(password, mode);
}

void Url::setPath(const QString &path, QUrl::ParsingMode mode)
{
    QUrl::setPath(path, mode);
}

void Url::setPort(int port)
{
    QUrl::setPort(port);
}

void Url::setQuery(const QString &query, QUrl::ParsingMode mode)
{
    QUrl::setQuery(query, mode);
}

void Url::setQuery(const QUrlQuery &query)
{
    QUrl::setQuery(query);
}

void Url::setScheme(const QString &scheme)
{
    QUrl::setScheme(scheme);
}

void Url::setUrl(const QString &url, QUrl::ParsingMode mode)
{
    QUrl::setUrl(url, mode);
}

void Url::setUserInfo(const QString &userInfo, QUrl::ParsingMode mode)
{
    QUrl::setUserInfo(userInfo, mode);
}

void Url::setUserName(const QString &userName, QUrl::ParsingMode mode)
{
    QUrl::setUserName(userName, mode);
}

void Url::swap(QUrl &other)
{
    QUrl::swap(other);
}

QString Url::toDisplayString(QUrl::FormattingOptions options) const
{
    return QUrl::toDisplayString(options);
}

QByteArray Url::toEncoded(QUrl::FormattingOptions options) const
{
    return QUrl::toEncoded(options);
}

QString Url::toLocalFile() const
{
    return QUrl::toLocalFile();
}

QString Url::toString(QUrl::FormattingOptions options) const
{
    return QUrl::toString(options);
}

QString Url::url(QUrl::FormattingOptions options) const
{
    return QUrl::url(options);
}

QString Url::userInfo(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::userInfo(options);
}

QString Url::userName(QUrl::ComponentFormattingOptions options) const
{
    return QUrl::userName(options);
}

QString Url::fromAce(const QByteArray &domain)
{
    return QUrl::fromAce(domain);
}

QUrl Url::fromEncoded(const QByteArray &url, QUrl::ParsingMode mode)
{
    return QUrl::fromEncoded(url,mode);
}

QUrl Url::fromLocalFile(const QString &localfile)
{
    return QUrl::fromLocalFile(localfile);
}

QString Url::fromPercentEncoding(const QByteArray &input)
{
    return QUrl::fromPercentEncoding(input);
}

QList<QUrl> Url::fromStringList(const QStringList &uris, QUrl::ParsingMode mode)
{
    return QUrl::fromStringList(uris,mode);
}

QUrl Url::fromUserInput(const QString &userInput)
{
    return QUrl::fromUserInput(userInput);
}

QUrl Url::fromUserInput(const QString &userInput, const QString &workingDirectory,
                        QUrl::UserInputResolutionOptions options)
{
    return QUrl::fromUserInput(userInput,workingDirectory,options);
}

QStringList Url::idnWhitelist()
{
    return QUrl::idnWhitelist();
}

void Url::setIdnWhitelist(const QStringList &list)
{
    QUrl::setIdnWhitelist(list);
}

QByteArray Url::toAce(const QString &domain)
{
    return QUrl::toAce(domain);
}

QByteArray Url::toPercentEncoding(const QString &input, const QByteArray &exclude,
                                  const QByteArray &include)
{
    return QUrl::toPercentEncoding(input,exclude,include);
}

QStringList Url::toStringList(const QList<QUrl> &uris, QUrl::FormattingOptions options)
{
    return QUrl::toStringList(uris,options);
}

Url &Url::operator=(const Url &other)
{
    if (this != &other)
        QUrl::operator=(other);
    return *this;
}
