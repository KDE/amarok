/*****************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>          *
 *                      : (C) 2008 William Viana Soares <vianasw@gmail.com>  *
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CURRENT_TRACK_APPLET_H
#define CURRENT_TRACK_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include "context/Svg.h"
#include <context/widgets/TrackWidget.h>
#include <meta/Meta.h>

#include <QAction>
#include <QList>

class RatingWidget;
class QCheckBox;
class QGraphicsPixmapItem;
class QHBoxLayout;
class QLabel;
class QSpinBox;

namespace Plasma { class DataEngine; }

static const int MAX_PLAYED_TRACKS = 5;

class CurrentTrack : public Context::Applet
{
    Q_OBJECT

public:
    CurrentTrack( QObject* parent, const QVariantList& args );
    ~CurrentTrack();

    virtual void init();

    virtual void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();

protected:
    virtual void constraintsEvent( Plasma::Constraints constraints );

private slots:
    void configAccepted();
    void changeTrackRating( int rating );
    void connectSource( const QString &source );
    void paletteChanged( const QPalette & palette );

private:
    QList<QAction*> contextualActions();

    QHBoxLayout* m_configLayout;
    QSpinBox* m_spinWidth;
    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_theme;

    QGraphicsSimpleTextItem* m_titleLabel;
    QGraphicsSimpleTextItem* m_artistLabel;
    QGraphicsSimpleTextItem* m_albumLabel;
    QGraphicsPixmapItem* m_scoreLabel;
    QGraphicsPixmapItem* m_numPlayedLabel;
    QGraphicsPixmapItem* m_playedLastLabel;

    QGraphicsSimpleTextItem* m_title;
    QGraphicsSimpleTextItem* m_artist;
    QGraphicsSimpleTextItem* m_album;
    QGraphicsSimpleTextItem* m_score;
    QGraphicsSimpleTextItem* m_numPlayed;
    QGraphicsSimpleTextItem* m_playedLast;
    QGraphicsSimpleTextItem* m_noTrack;

    QGraphicsRectItem* m_scoreIconBox;
    QGraphicsRectItem* m_numPlayedIconBox;
    QGraphicsRectItem* m_playedLastIconBox;

    int m_rating;
    int m_trackLength;

    QGraphicsPixmapItem* m_albumCover;
    QGraphicsPixmapItem* m_sourceEmblem;
    QPixmap m_bigCover;
    QPixmap m_sourceEmblemPixmap;

    RatingWidget* m_ratingWidget;

    QString m_noTrackText;

    bool resizeCover( QPixmap cover, qreal margin, qreal width );

    int m_maxTextWidth;


    //keep this safe as we might need it when resizing
    QVariantMap m_currentInfo;

    TrackWidget *m_lastTracks[MAX_PLAYED_TRACKS];
    Meta::TrackList m_tracks;
    int m_tracksToShow;

};

K_EXPORT_AMAROK_APPLET( currenttrack, CurrentTrack )

#endif
