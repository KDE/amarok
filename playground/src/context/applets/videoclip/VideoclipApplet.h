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

#ifndef VIDEOCLIP_APPLET_H
#define VIDEOCLIP_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"
#include "EngineObserver.h"


#include <KDialog>
#include <Phonon/VideoWidget>
#include <Phonon/VideoPlayer>
#include <Phonon/Path>

#include <Plasma/Label>
#include <Plasma/GroupBox>

#include <QGraphicsProxyWidget>
#include <QTimeLine>
#include <QWidget>


class QGraphicsPixmapItem;
class QGraphicsLinearLayout;
class QGraphicsProxyWidget;
class QLabel;
class QHBoxLayout;
class QSpinBox;
class QCheckBox;
class QGraphicsWidget;


class VideoclipApplet : public Context::Applet, public EngineObserver
{
    Q_OBJECT

public:
    VideoclipApplet( QObject* parent, const QVariantList& args );
    ~VideoclipApplet();

    void init();
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );    
    void connectSource( const QString &source );
    
protected:
    void EngineNewTrackPlaying();

private:
    QGraphicsLinearLayout *m_layout;
    Phonon::MediaObject *m_mediaObject;
    Phonon::VideoWidget *m_videoWidget;
    Phonon::Path path;

    QGraphicsSimpleTextItem *m_headerText;
    QGraphicsWidget         *m_widget;

//    QList<Plasma::Label>m_labelUrl;
    QList<Plasma::GroupBox *>m_vidGroup;
    QStringList m_titleList, m_idList, m_coverList, m_durationList, m_descList;


};

K_EXPORT_AMAROK_APPLET( videoclip, VideoclipApplet )

#endif

