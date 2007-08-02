/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

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
#include <context/Svg.h>
#include <context/widgets/TextWidget.h>

#include <QGraphicsPixmapItem>

class CurrentTrack : public Context::Applet
{
    Q_OBJECT
public:
    CurrentTrack( QObject* parent, const QStringList& args );
    ~CurrentTrack();
    
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
    
    QSizeF contentSize() const;
    void constraintsUpdated();
    
public slots:
    void updated( const QString& name, const Plasma::DataEngine::Data &data );
    
private:
    
    QSizeF m_size;
    Context::Svg* m_theme;
    
    QGraphicsSimpleTextItem* m_title;
    QGraphicsSimpleTextItem* m_artist;
    QGraphicsSimpleTextItem* m_album;
    QGraphicsSimpleTextItem* m_score;
    QGraphicsSimpleTextItem* m_numPlayed;
    QGraphicsSimpleTextItem* m_playedLast;
    
    int m_rating;
    int m_trackLength;
    
    QGraphicsPixmapItem* m_albumCover;

};

K_EXPORT_AMAROK_APPLET( currenttrack, CurrentTrack )

#endif
