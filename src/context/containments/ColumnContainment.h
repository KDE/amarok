/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
* copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>  *
***************************************************************************/

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

#include "plasma/animator.h"
#include "SvgHandler.h"

#include <QGraphicsGridLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QList>
#include <QImage>
#include <QPixmapCache>

#include <KTemporaryFile>
#include <threadweaver/Job.h>
#include <KSvgRenderer>

#define MAX_ROWS 10
#define MAX_COLUMNS 10

namespace Context
{
    class SvgRenderJob;

class /*AMAROK_EXPORT*/ ColumnContainment : public Containment
{
    Q_OBJECT
public:
    ColumnContainment( QObject *parent, const QVariantList &args );
    ~ColumnContainment();

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
    Applet* addApplet( Plasma::Applet* applet, const QPointF &);


protected:
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
//     virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * event );
//     bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

protected slots:
    void recalculate();
    void paletteChange();

private slots:

    void appletRemoved( Plasma::Applet * );
    void jobDone();
    void showAddWidgetsInterface();

private:
    void rearrangeApplets( int starRow, int startColumn );
    bool insertInGrid( Plasma::Applet* applet );
    
    QAction* m_appletBrowserAction;

    QList<QAction*> *m_actions;

//     ContextLayout* m_columns;
    QGraphicsGridLayout* m_grid;
    
    int m_currentRows;
    int m_currentColumns;
    
    int m_minColumnWidth;
    int m_maxColumnWidth;
    int m_defaultRowHeight;
    bool m_gridFreePositions[MAX_ROWS][MAX_COLUMNS];
    
    QHash< Plasma::Applet*, QList<int> > m_appletsPositions;
    QHash< Plasma::Applet*, int > m_appletsIndexes;
    
    Plasma::Svg* m_background;
    Plasma::Svg* m_logo;
    qreal m_width;
    qreal m_aspectRatio;

    KTemporaryFile m_tintedSvg;
    QImage m_masterImage;
    QPixmapCache m_cache;

    SvgRenderJob *m_job;

    //KSvgRenderer * m_renderer;
};

K_EXPORT_AMAROK_APPLET( context, ColumnContainment )


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
