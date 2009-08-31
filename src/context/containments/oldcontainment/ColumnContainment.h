/****************************************************************************************
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef COLUMN_CONTAINMENT_H
#define COLUMN_CONTAINMENT_H

#include "Containment.h"
#include "ContextView.h"
#include "SmartPointerList.h"
#include "widgets/ContainmentSelectionLayer.h"
#include "widgets/ToolBoxMenu.h"

#include <plasma/animator.h>
#include <plasma/containment.h>
#include <plasma/widgets/iconwidget.h>

#include <KConfigGroup>

#include <QGraphicsGridLayout>
#include <QGraphicsSceneMouseEvent>
#include <QQueue>


class QAction;
class QApplication;

namespace Context
{

class ColumnContainment : public Containment
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

    virtual void setTitle( const QString& text );
    virtual void setFooter( const QString& text );
    virtual void showTitle();
    virtual void hideTitle();
    virtual void addCurrentTrack();
    virtual void setView( ContextView *newView );
    virtual void setZoomLevel( Plasma::ZoomLevel lvl );
    
    virtual ContextView *view();
    
Q_SIGNALS:
    void changeContainment( Plasma::Containment*, int );
    void zoomOut( Plasma::Containment* );
    void zoomIn( Plasma::Containment* );

protected:
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    
public slots:
    Applet* addApplet( Plasma::Applet* applet, const QPointF & );
    
private slots:
    void appletRemoved( Plasma::Applet * );
    void showRemoveAppletsMenu();
    void showAddAppletsMenu();
    
    void zoomOutRequested();
    void zoomInRequested();
    
private:
    void rearrangeApplets( int starRow, int startColumn );
    bool insertInGrid( Plasma::Applet* applet );
    void loadInitialConfig();   
    void setupControlButtons();
    void setupRemoveButton();
    void correctControlButtonPositions();
    
    Plasma::IconWidget* addAction( QAction* );
    
    SmartPointerList<QAction*> *m_actions;
    
    QGraphicsGridLayout *m_grid;
    
    int m_currentRows;
    int m_currentColumns;

    int m_minColumnWidth;
    int m_maxColumnWidth;
    int m_rowHeight;
    int m_preferredRowHeight;

    typedef bool* PositionsRow;
    PositionsRow *m_gridFreePositions;
    int m_maxRows;
    int m_maxColumns;
    
    QHash< Plasma::Applet*, QList<int> > m_appletsPositions;
    QHash< Plasma::Applet*, int > m_appletsIndexes;

    qreal m_width;
    qreal m_aspectRatio;
    
    QGraphicsSimpleTextItem *m_title;
    QGraphicsSimpleTextItem *m_footer;
    Context::Svg *m_header;
    
    bool m_paintTitle;
    bool m_manageCurrentTrack;
    int m_appletsFromConfigCount;

    QQueue<QString> m_pendingApplets;

    Plasma::ZoomLevel m_zoomLevel;
    
    Plasma::IconWidget *m_zoomInIcon;
    Plasma::IconWidget *m_zoomOutIcon;
    Plasma::IconWidget *m_addAppletsIcon;
    Plasma::IconWidget *m_removeAppletsIcon;
    Plasma::IconWidget *m_switchRightIcon;
    Plasma::IconWidget *m_switchLeftIcon;

    AmarokToolBoxMenu *m_addAppletsMenu;
    AmarokToolBoxMenu *m_removeAppletsMenu;

    ContextView *m_view;
    
    ContainmentSelectionLayer *m_selectionLayer;
    
//     bool m_gridFreePositions[MAX_ROWS][MAX_COLUMNS];
};

K_EXPORT_PLASMA_APPLET( context, ColumnContainment )

}
#endif
