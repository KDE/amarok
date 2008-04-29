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

#ifndef COLUMN_APPLET_H
#define COLUMN_APPLET_H

#include "TheInstances.h"

#include "amarok_export.h"
#include "context/Applet.h"
#include "context/Containment.h"
#include "context/layouts/ContextLayout.h"
#include "context/Svg.h"

#include "plasma/appletbrowser.h"
#include "plasma/animator.h"
#include "SvgHandler.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QList>
#include <QImage>
#include <QPixmapCache>

#include <KTemporaryFile>
#include <threadweaver/Job.h>
#include <KSvgRenderer>


namespace Context
{
    class SvgRenderJob;

class /*AMAROK_EXPORT*/ ColumnApplet : public Containment
{
    Q_OBJECT
public:
    ColumnApplet( QObject *parent, const QVariantList &args );
    ~ColumnApplet() {}

    virtual QRectF boundingRect() const;

    void saveToConfig( KConfig& conf );
    void loadConfig( KConfig& conf );

    void updateSize();

    QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const;

    virtual void paintInterface(QPainter *painter,
                                const QStyleOptionGraphicsItem *option,
                                const QRect& contentsRect);

    QList<QAction*> contextualActions();



public slots:
    void appletRemoved( QObject* object );
    Applet* addApplet( Plasma::Applet* applet );


protected:
//     virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
//     virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * event );
//     bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

protected slots:
    void launchAppletBrowser();
    void recalculate();
    void paletteChange();

private slots:
    void destroyApplet();
    void appletDisappearComplete(QGraphicsItem *item, Plasma::Animator::Animation anim);

    void jobDone();
private:
    QAction* m_appletBrowserAction;
    Plasma::AppletBrowser* m_appletBrowser;

    QRectF m_geometry;

    QList<QAction*> *m_actions;

    ContextLayout* m_columns;
    int m_defaultColumnSize;
    Plasma::Svg* m_background;
    Plasma::Svg* m_logo;
    qreal m_width;
    qreal m_aspectRatio;

    KTemporaryFile m_tintedSvg;
    QImage m_masterImage;
    QPixmapCache m_cache;

    SvgRenderJob *m_job;

    //HACK to make the applet browser not be empty untill it has been resized...
    bool m_appletBrowserHasBeenKicked;

    //KSvgRenderer * m_renderer;
};

K_EXPORT_AMAROK_APPLET( context, ColumnApplet )


class SvgRenderJob : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        SvgRenderJob( const QString &file )
                : ThreadWeaver::Job()
                , m_file( file ) {}

        virtual void run();

        QImage m_image;
    private:
        QString m_file;
};

} // namespace

#endif
