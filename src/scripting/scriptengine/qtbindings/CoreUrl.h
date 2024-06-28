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

#ifndef COREURL_H
#define COREURL_H

#include "QtBinding.h"

#include <QObject>
#include <QUrl>

namespace QtBindings
{
    namespace Core
    {
        class Url : public QObject, public QUrl, public QtBindings::Base<Url>
        {
        Q_OBJECT
        public:
            Q_INVOKABLE Url();
            Q_INVOKABLE explicit Url(const QUrl &copy);
            Q_INVOKABLE Url(const Url &copy);
            Q_INVOKABLE explicit Url(const QString &url, ParsingMode mode = TolerantMode);
            Q_INVOKABLE explicit Url(QUrl &&other);
            Q_INVOKABLE static QString fromAce(const QByteArray &domain);
            Q_INVOKABLE static QUrl fromEncoded(const QByteArray &url, ParsingMode mode = TolerantMode);
            Q_INVOKABLE static QUrl fromLocalFile(const QString &localfile);
            Q_INVOKABLE static QString fromPercentEncoding(const QByteArray &input);
            Q_INVOKABLE static QList<QUrl> fromStringList(const QStringList &uris, ParsingMode mode = TolerantMode);
            Q_INVOKABLE static QUrl fromUserInput(const QString &userInput);
            Q_INVOKABLE static QUrl fromUserInput(const QString &userInput, const QString &workingDirectory, UserInputResolutionOptions options = DefaultResolution);
            Q_INVOKABLE static QStringList idnWhitelist();
            Q_INVOKABLE static void setIdnWhitelist(const QStringList &list);
            Q_INVOKABLE static QByteArray toAce(const QString &domain);
            Q_INVOKABLE static QByteArray toPercentEncoding(const QString &input, const QByteArray &exclude = QByteArray(), const QByteArray &include = QByteArray());
            Q_INVOKABLE static QStringList toStringList(const QList<QUrl> &uris, FormattingOptions options = FormattingOptions(PrettyDecoded));
            Url &operator=(const Url &other);
        public Q_SLOTS:
            QUrl adjusted(FormattingOptions options) const;
            QString authority(ComponentFormattingOptions options = PrettyDecoded) const;
            void clear();
            void detach();
            QString errorString() const;
            QString fileName(ComponentFormattingOptions options = FullyDecoded) const;
            QString fragment(ComponentFormattingOptions options = PrettyDecoded) const;
            bool hasFragment() const;
            bool hasQuery() const;
            QString host(ComponentFormattingOptions options = FullyDecoded) const;
            bool isDetached() const;
            bool isEmpty() const;
            bool isLocalFile() const;
            bool isParentOf(const QUrl &url) const;
            bool isRelative() const;
            bool isValid() const;
            bool matches(const QUrl &url, FormattingOptions options) const;
            QString password(ComponentFormattingOptions options = FullyDecoded) const;
            QString path(ComponentFormattingOptions options = FullyDecoded) const;
            int port(int defaultPort = -1) const;
            QString query(ComponentFormattingOptions options = PrettyDecoded) const;
            QUrl resolved(const QUrl &relative) const;
            QString scheme() const;
            void setAuthority(const QString &authority, ParsingMode mode = TolerantMode);
            void setFragment(const QString &fragment, ParsingMode mode = TolerantMode);
            void setHost(const QString &host, ParsingMode mode = DecodedMode);
            void setPassword(const QString &password, ParsingMode mode = DecodedMode);
            void setPath(const QString &path, ParsingMode mode = DecodedMode);
            void setPort(int port);
            void setQuery(const QString &query, ParsingMode mode = TolerantMode);
            void setQuery(const QUrlQuery &query);
            void setScheme(const QString &scheme);
            void setUrl(const QString &url, ParsingMode mode = TolerantMode);
            void setUserInfo(const QString &userInfo, ParsingMode mode = TolerantMode);
            void setUserName(const QString &userName, ParsingMode mode = DecodedMode);
            void swap(QUrl &other);
            QString toDisplayString(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
            QByteArray toEncoded(FormattingOptions options = FullyEncoded) const;
            QString toLocalFile() const;
            QString toString(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
            QString url(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
            QString userInfo(ComponentFormattingOptions options = PrettyDecoded) const;
            QString userName(ComponentFormattingOptions options = FullyDecoded) const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Core::Url)
#endif //COREURL_H
