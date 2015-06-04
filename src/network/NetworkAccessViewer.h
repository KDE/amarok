/***************************************************************************
 *   Copyright (C) 2009, 2010 by Richard J. Moore <rich@kde.org>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef NETWORKACCESSVIEWER_H
#define NETWORKACCESSVIEWER_H

#include "ui_NetworkRequests.h"

#include <qobject.h>
#include <qmap.h>
#include <qpair.h>
#include <qlist.h>
#include <qbytearray.h>
#include <qnetworkaccessmanager.h>

class QDialog;
class QTreeWidgetItem;
class QSignalMapper;

class NetworkAccessViewer : public QObject
{
    Q_OBJECT

public:
    NetworkAccessViewer( QWidget *parent = 0 );
    ~NetworkAccessViewer();

    void addRequest( QNetworkAccessManager::Operation op, const QNetworkRequest&req, QIODevice *outgoingData, QNetworkReply *reply );

    void show();
    void hide();

private Q_SLOTS:
    void clear();
    void requestFinished( QObject *replyObject );
    void showItemDetails( QTreeWidgetItem *item );

private:
    QDialog *dialog;
    Ui::NetworkRequestsDialog *networkRequestsDialog;
    QMap<QNetworkReply *, QNetworkRequest> requestMap;
    QMap<QTreeWidgetItem *, QNetworkRequest> itemRequestMap;
    QMap<QNetworkReply *, QTreeWidgetItem *> itemMap;
    QMap<QTreeWidgetItem *, QPair< QList<QByteArray>, QList<QByteArray> > > itemReplyMap;
    QSignalMapper *mapper;
};

#endif // NETWORKACCESSVIEWER_H
