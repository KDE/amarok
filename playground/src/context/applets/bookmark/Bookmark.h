/*****************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>          *
 *                      : (C) 2008 William Viana Soares <vianasw@gmail.com>  *
 *                  : (C) 2008 Nikolaj Hald Nielsen <nhnFreespiri@gmail.com> *
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKMARK_APPLET_H
#define BOOKMARK_APPLET_H

#include <context/Applet.h>
#include <context/DataEngine.h>
#include "context/Svg.h"
#include <context/widgets/TrackWidget.h>
#include <meta/Meta.h>
#include "amarokurls/BookmarkManagerWidget.h"


#include <QAction>
#include <QList>
#include <QGraphicsProxyWidget>

class RatingWidget;
class QCheckBox;
class QGraphicsPixmapItem;
class QHBoxLayout;
class QLabel;
class QSpinBox;

namespace Plasma { class DataEngine; }

static const int MAX_PLAYED_TRACKS = 5;

class Bookmark : public Context::Applet
{
    Q_OBJECT

public:
    Bookmark( QObject* parent, const QVariantList& args );
    ~Bookmark();

    virtual void init();

    virtual void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

protected:
    virtual void constraintsEvent( Plasma::Constraints constraints );

private slots:
    void paletteChanged( const QPalette & palette );

private:

    QGraphicsProxyWidget * m_proxyWidget;
    BookmarkManagerWidget * m_bookmarkWidget;

    int m_width;

    qreal m_aspectRatio;

    Context::Svg* m_theme;

};

K_EXPORT_AMAROK_APPLET( bookmark, Bookmark )

#endif
