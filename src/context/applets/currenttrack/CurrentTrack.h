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

#include <KDialog>

class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;

class CurrentTrack : public Context::Applet
{
    Q_OBJECT
public:
    CurrentTrack( QObject* parent, const QVariantList& args );
    ~CurrentTrack();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    QSizeF contentSizeHint() const;
    void constraintsUpdated();

    void setGeometry( const QRectF& geometry );
    
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();


private slots:
    void configAccepted();

private:
    QString truncateTextToFit( QString text, const QFont& font, const QRectF& bounds );
    QFont shrinkTextSizeToFit( const QString& text, const QRectF& bounds );
    void resize( qreal newWidth, qreal aspectRatio );

    KDialog* m_config;
    QHBoxLayout* m_configLayout;
    QSpinBox* m_spinWidth;
    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_theme;
    QSizeF m_size;

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
