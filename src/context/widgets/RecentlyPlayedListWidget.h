/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#ifndef RECENTLY_PLAYED_LIST_WIDGET_H
#define RECENTLY_PLAYED_LIST_WIDGET_H

#include "core/meta/forward_declarations.h"

#include <QIcon>
#include <QUrl>
#include <Plasma/ScrollWidget>

#include <QDateTime>
#include <QGraphicsWidget>
#include <QLabel>
#include <QQueue>

class QGraphicsLinearLayout;

class ClickableGraphicsWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit ClickableGraphicsWidget( const QString &url, QGraphicsItem *parent = 0,
                                      Qt::WindowFlags wFlags = 0 );
    ~ClickableGraphicsWidget();

signals:
    void leftClicked( const QString &url );
    void middleClicked( const QString &url );

protected:
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );

private:
    const QString m_url;
};

class TimeDifferenceLabel : public QLabel
{
    Q_OBJECT

public:
    explicit TimeDifferenceLabel( const QDateTime &eventTime, QWidget *parent = 0,
                                  Qt::WindowFlags wFlags = 0 );
    ~TimeDifferenceLabel();

public slots:
    void update();

private:
    const QDateTime m_eventTime;
};

class RecentlyPlayedListWidget : public Plasma::ScrollWidget
{
    Q_OBJECT

    struct RecentlyPlayedTrackData
    {
        QDateTime recentlyPlayed;
        QString displayName;
        QString trackUrl;
        QGraphicsWidget *widget;
    };

public:
    explicit RecentlyPlayedListWidget( QGraphicsWidget *parent = 0 );
    ~RecentlyPlayedListWidget();

private slots:
    void itemLeftClicked( const QString &url );
    void itemMiddleClicked( const QString &url );
    void trackChanged( const Meta::TrackPtr &track );

private:
    Q_DISABLE_COPY( RecentlyPlayedListWidget )

    void addTrack( const Meta::TrackPtr &track );
    void addTrack( const QDateTime &recentlyPlayed, const QString &displayName,
                   const QString &trackUrl );
    QGraphicsWidget *addWidgetItem( const RecentlyPlayedTrackData &data );

    Meta::TrackPtr m_currentTrack;
    QGraphicsLinearLayout *m_layout;
    QQueue<RecentlyPlayedTrackData> m_recentTracks;
    QIcon m_trackIcon;
    QTimer *m_updateTimer;
};

#endif // RECENTLY_PLAYED_LIST_WIDGET_H
