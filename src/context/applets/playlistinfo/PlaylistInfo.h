/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#ifndef PLAYLIST_INFO_APPLET_H
#define PLAYLIST_INFO_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include <context/Svg.h>

#include <KDialog>

class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;

class PlayistInfo : public Context::Applet
{
    Q_OBJECT
public:
    PlayistInfo( QObject* parent, const QStringList& args );
    ~PlayistInfo();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent();
    QSizeF contentSizeHint() const { return m_size; }

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );
    void showConfigurationInterface();


private slots:
    void configAccepted();

private:
    void resize( qreal newWidth, qreal aspectRatio );

    KDialog* m_config;
    QHBoxLayout* m_configLayout;
    QSpinBox* m_spinWidth;
    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_theme;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_totalTracks;
    QGraphicsSimpleTextItem* m_totalTime;
    QGraphicsSimpleTextItem* m_totalSize;

    int m_rating;
    int m_trackLength;

    QGraphicsPixmapItem* m_albumCover;

};

K_EXPORT_AMAROK_APPLET( playlistinfo, PlayistInfo )

#endif
