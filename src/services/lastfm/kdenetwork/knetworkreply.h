/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#ifndef KNETWORKREPLY_H
#define KNETWORKREPLY_H

#include <QtNetwork/QNetworkReply>

#include <kdemacros.h>

namespace KIO
{
    class Job;
}
class KJob;

class KDE_EXPORT KNetworkReply : public QNetworkReply
{
    Q_OBJECT
public:
    KNetworkReply(const QNetworkRequest &request, KIO::Job *kioJob, QObject *parent);

    virtual qint64 bytesAvailable() const;
    virtual void abort();

public Q_SLOTS:
    void appendData(KIO::Job *kioJob, const QByteArray &data);
    void setMimeType(KIO::Job *kioJob, const QString &mimeType);
    void jobDone(KJob *kJob);

protected:
    virtual qint64 readData(char *data, qint64 maxSize);
    
private:
    class KNetworkReplyPrivate;
    KNetworkReplyPrivate* const d;

};

#endif // KNETWORKREPLY_H
