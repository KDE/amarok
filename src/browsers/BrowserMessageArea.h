/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BROWSERMESSAGEAREA_H
#define BROWSERMESSAGEAREA_H

#include "core/logger/Logger.h"
#include "statusbar/CompoundProgressBar.h"
#include "statusbar/KJobProgressBar.h"
#include "statusbar/NetworkProgressBar.h"
#include "widgets/BoxWidget.h"

#include <QTimer>

class BrowserMessageArea : public BoxWidget, public Amarok::Logger
{
    Q_OBJECT

public:
    explicit BrowserMessageArea( QWidget *parent );

    ~BrowserMessageArea()
    {
    }

protected:
    /* Amarok::Logger virtual methods */
    void shortMessageImpl( const QString &text ) override;
    void longMessageImpl( const QString &text, MessageType type ) override;
    virtual void newProgressOperationImpl( KJob * job, const QString & text, QObject *context,
                                           const std::function<void ()> &function, Qt::ConnectionType type ) override;

    virtual void newProgressOperationImpl( QNetworkReply *reply, const QString &text, QObject *obj,
                                           const std::function<void ()> &function, Qt::ConnectionType type ) override;

    virtual void newProgressOperationImpl( QObject *sender, const QMetaMethod &increment, const QMetaMethod &end, const QString &text,
                                           int maximum, QObject *obj, const std::function<void ()> &function, Qt::ConnectionType type ) override;

Q_SIGNALS:
    void signalLongMessage( const QString & text, MessageType type );

private Q_SLOTS:
    void hideProgress();
    void nextShortMessage();
    void slotLongMessage( const QString &text, MessageType type = Information );

private:
    CompoundProgressBar *m_progressBar;
    QLabel *m_messageLabel;

    bool m_busy;
    QTimer *m_shortMessageTimer;
    QList<QString> m_shortMessageQueue;
};

#endif // BROWSERMESSAGEAREA_H
