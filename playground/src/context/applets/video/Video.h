/***************************************************************************
 *   Plasma applet for showing video in the context view.                  *
 *                                                                         *
 *   Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef VIDEO_APPLET_H
#define VIDEO_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

#include <KDialog>
#include <Phonon/VideoWidget>

#include <QGraphicsProxyWidget>
#include <QTimeLine>
#include <QWidget>

class QGraphicsPixmapItem;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;


class Video : public Context::Applet
{
    Q_OBJECT

public:
    Video( QObject* parent, const QVariantList& args );
    ~Video();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    QSizeF effectiveSizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

private:
    Phonon::VideoWidget* m_videoWidget;
};

K_EXPORT_AMAROK_APPLET( video, Video )

#endif

