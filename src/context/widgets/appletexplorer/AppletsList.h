/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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


#ifndef APPLETS_LIST_H
#define APPLETS_LIST_H

#include "amarok_export.h"
#include "AppletIcon.h"
#include "AppletItemModel.h"

#include "plasma/widgets/iconwidget.h"

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QPainter>
#include <QWeakPointer>

namespace Plasma
{
    class Animation;
}

namespace Context
{

class AMAROK_EXPORT AppletsListWidget: public QGraphicsWidget
{
    Q_OBJECT

public:
    AppletsListWidget( QGraphicsItem *parent = 0 );
    virtual ~AppletsListWidget();

    void setModel( QStandardItemModel *model );

protected:
    virtual void resizeEvent( QGraphicsSceneResizeEvent *event );

signals:
    void appletClicked( AppletItem *appletItem );

private slots:
    void appletIconClicked();
    void scrollLeft();
    void scrollRight();

private:
    void init();

    AppletIconWidget *createAppletIcon( AppletItem *appletItem );
    void insertAppletIcon( AppletIconWidget *appletIcon );
    void updateList();
    int maximumVisibleAppletsOnList() const;
    QRectF visibleListRect() const;
    int findFirstVisibleAppletIdx() const;
    int findLastVisibleAppletIdx() const;

    QGraphicsLinearLayout *m_mainLayout;
    QGraphicsLinearLayout *m_appletsListLayout;
    QStandardItemModel *m_model;
    QHash< QString, AppletIconWidget *> *m_applets;
    QGraphicsWidget *m_appletsListWidget;
    QGraphicsWidget *m_appletsListWindow;

    Plasma::IconWidget *m_leftArrow;
    Plasma::IconWidget *m_rightArrow;
    QWeakPointer<Plasma::Animation> m_slideAnimation;
};

} //namespace Context

#endif
