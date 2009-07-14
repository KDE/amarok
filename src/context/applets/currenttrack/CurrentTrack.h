/****************************************************************************************
 * Copyright (c) 2007-2009 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef CURRENT_TRACK_APPLET_H
#define CURRENT_TRACK_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/widgets/TrackWidget.h>
#include <meta/Meta.h>

#include <QAction>
#include <QList>

class TextScrollingWidget;
class DropPixmapItem;
class RatingWidget;
class QCheckBox;
class QGraphicsPixmapItem;
class QGraphicsLinearLayout;
class QHBoxLayout;
class QLabel;
class QSpinBox;

namespace Plasma {
    class DataEngine;
    class TabBar;
}

static const int MAX_PLAYED_TRACKS = 5;

class CurrentTrack : public Context::Applet
{
    Q_OBJECT

public:
    CurrentTrack( QObject* parent, const QVariantList& args );
    ~CurrentTrack();

    virtual void init();
    virtual void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

protected:
    virtual void constraintsEvent( Plasma::Constraints constraints );

private slots:
    void changeTrackRating( int rating );
    void connectSource( const QString &source );
    void paletteChanged( const QPalette & palette );
    void tabChanged( int index );
    void coverDropped( QPixmap cover );

private:
    QList<QAction*> contextualActions();

    bool resizeCover( QPixmap cover, qreal width, QPointF albumCoverPos );
    // aligns the second QGI to be at the same level as the first (the bottom edges)
    void alignBottomToFirst( QGraphicsItem* a, QGraphicsItem* b );

    TextScrollingWidget* m_title;
    TextScrollingWidget* m_artist;
    TextScrollingWidget* m_album;
    QGraphicsSimpleTextItem* m_noTrack;
    QGraphicsSimpleTextItem* m_byText;
    QGraphicsSimpleTextItem* m_onText;
    int m_rating;
    int m_trackLength;

    DropPixmapItem* m_albumCover;
    QPixmap m_bigCover;
    QString m_sourceEmblemPath;

    RatingWidget* m_ratingWidget;

    QString m_noTrackText;
    QString m_playCountLabel;
    QString m_scoreLabel;
    QString m_lastPlayedLabel;
    QString m_score;
    QString m_numPlayed;
    QString m_playedLast;

    bool m_showStatistics;

    int m_maxTextWidth;

    //keep this safe as we might need it when resizing
    QVariantMap m_currentInfo;

    TrackWidget *m_tracks[MAX_PLAYED_TRACKS];
    Meta::TrackList m_lastTracks;
    Meta::TrackList m_favoriteTracks;
    int m_tracksToShow;

    Plasma::TabBar *m_tabBar;
};

K_EXPORT_AMAROK_APPLET( currenttrack, CurrentTrack )

#endif
