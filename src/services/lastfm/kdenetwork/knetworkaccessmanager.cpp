/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2007 Trolltech ASA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "knetworkaccessmanager.h"

#include "knetworkreply.h"

#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <KDebug>
#include <kio/job.h>

class KNetworkAccessManager::KNetworkAccessManagerPrivate
{
public:
    KNetworkAccessManagerPrivate() {}
};

KNetworkAccessManager::KNetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent), d(new KNetworkAccessManager::KNetworkAccessManagerPrivate())
{
}

KNetworkAccessManager::~KNetworkAccessManager()
{
    delete d;
}

QNetworkReply *KNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    KIO::Job *kioJob = 0;

    switch (op) {
        case HeadOperation: {
            kDebug() << "HeadOperation:" << req.url();

            kioJob = KIO::mimetype(req.url(), KIO::HideProgressInfo);

            break;
        }
        case GetOperation: {
            kDebug() << "GetOperation:" << req.url();

            kioJob = KIO::get(req.url(), KIO::NoReload, KIO::HideProgressInfo);

            break;
        }
        case PutOperation: {
            kDebug() << "PutOperation:" << req.url();

            kioJob = KIO::put(req.url(), -1, KIO::HideProgressInfo);

            break;
        }
        case PostOperation: {
            kDebug() << "PostOperation:" << req.url();
            kioJob = KIO::http_post(req.url(), outgoingData->readAll(), KIO::HideProgressInfo);
            kioJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded" );
            break;            
        }
        default:
            kDebug() << "Unknown operation";
            return 0;
    }

    KNetworkReply *reply = new KNetworkReply(req, kioJob, this);

    kioJob->addMetaData(metaDataForRequest(req));

    connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        reply, SLOT(appendData(KIO::Job *, const QByteArray &)));
    connect(kioJob, SIGNAL(result(KJob *)), reply, SLOT(jobDone(KJob *)));
    connect(kioJob, SIGNAL(mimetype(KIO::Job *, const QString&)),
        reply, SLOT(setMimeType(KIO::Job *, const QString&)));

    return reply;
}


KIO::MetaData KNetworkAccessManager::metaDataForRequest(QNetworkRequest request)
{
    KIO::MetaData metaData;

    metaData.insert("PropagateHttpHeader", "true");

    metaData.insert("UserAgent", request.rawHeader("User-Agent"));
    request.setRawHeader("User-Agent", QByteArray());

    metaData.insert("accept", request.rawHeader("Accept"));
    request.setRawHeader("Accept", QByteArray());

    request.setRawHeader("content-length", QByteArray());
    request.setRawHeader("Connection", QByteArray());

    QString additionHeaders;
    Q_FOREACH(const QByteArray &headerKey, request.rawHeaderList()) {
        const QByteArray value = request.rawHeader(headerKey);
        if (value.isNull())
            continue;

        if (additionHeaders.length() > 0) {
            additionHeaders += "\r\n";
        }
        additionHeaders += headerKey + ": " + value;
    }
    metaData.insert("customHTTPHeader", additionHeaders);

    return metaData;
}

#include "knetworkaccessmanager.moc"
