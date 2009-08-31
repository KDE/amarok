/****************************************************************************************
 * Copyright (c) 2008 Urs Wolfer <uwolfer@kde.org>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef KNETWORKACCESSMANAGER_H
#define KNETWORKACCESSMANAGER_H

#include <kdemacros.h>

#include <KDE/KIO/MetaData>

#include <QNetworkAccessManager>

class KDE_EXPORT KNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    KNetworkAccessManager(QObject *parent);
    virtual ~KNetworkAccessManager();

    static KIO::MetaData metaDataForRequest(QNetworkRequest request);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private:
    class KNetworkAccessManagerPrivate;
    KNetworkAccessManagerPrivate* const d;
};

#endif // KNETWORKACCESSMANAGER_H
