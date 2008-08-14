/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>         *
* copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>   *
* copyright            : (C) 2008 William Viana Soares <vianasw@gmail.com> *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef COLUMN_CONTAINMENT_H
#define COLUMN_CONTAINMENT_H

#include "AmarokToolBox.h"
#include "Containment.h"
#include "ContextView.h"

#include <QObject>

#include <plasma/containment.h>
#include <plasma/animator.h>

#include <KConfigGroup>

#include <QGraphicsGridLayout>
#include <QGraphicsSceneMouseEvent>
#include <QQueue>

#define MAX_ROWS 10
#define MAX_COLUMNS 10

class QAction;
namespace Context
{
    
class ColumnContainment : public Context::Containment
{
    Q_OBJECT

public:
    ColumnContainment( QObject *parent, const QVariantList &args );
    ~ColumnContainment();

    void constraintsEvent( Plasma::Constraints constraints );

    QList<QAction*> contextualActions();

    virtual void paintInterface(QPainter *painter,
                                const QStyleOptionGraphicsItem *option,
                                const QRect& contentsRect);

    virtual void saveToConfig( KConfigGroup &conf );
    virtual void loadConfig( const KConfigGroup &conf );

    virtual bool hasPlaceForApplet( int rowSpan );

    virtual void setTitle( QString title );
    virtual void showTitle();
    virtual void hideTitle();
    virtual void addCurrentTrack();
    virtual void setView( ContextView *newView );

    virtual ContextView *view();

protected:
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    
protected slots:
    void recalculate();
    
public slots:
    Applet* addApplet( Plasma::Applet* applet, const QPointF & );
    virtual void addToolBox();
    virtual void correctToolBoxPos();
    
private slots:
    void appletRemoved( Plasma::Applet * );
    
private:
    void rearrangeApplets( int starRow, int startColumn );
    bool insertInGrid( Plasma::Applet* applet );
    void loadInitialConfig();
    
    QList<QAction*> *m_actions;
    
    QGraphicsGridLayout *m_grid;
    
    int m_currentRows;
    int m_currentColumns;

    int m_minColumnWidth;
    int m_maxColumnWidth;
    int m_defaultRowHeight;

    bool m_gridFreePositions[MAX_ROWS][MAX_COLUMNS];
    
    QHash< Plasma::Applet*, QList<int> > m_appletsPositions;
    QHash< Plasma::Applet*, int > m_appletsIndexes;

    qreal m_width;
    qreal m_aspectRatio;
    
    QGraphicsSimpleTextItem *m_title;
    Context::Svg *m_header;
    
    bool m_paintTitle;
    bool m_manageCurrentTrack;
    int m_appletsFromConfigCount;

    QQueue<QString> m_pendingApplets;

    AmarokToolBox *m_toolBox;

    ContextView *m_view;
//     bool m_gridFreePositions[MAX_ROWS][MAX_COLUMNS];
};

K_EXPORT_PLASMA_APPLET( context, ColumnContainment )

}
#endif
