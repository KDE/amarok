/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
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

#ifndef APPLET_EXPLORER_H
#define APPLET_EXPLORER_H

#include "amarok_export.h"

#include <QGraphicsWidget>
#include <QPainter>

class QAction;
class QStyleOptionGraphicsItem;

namespace Plasma {
    class IconWidget;
    class ScrollWidget;
}

namespace Context
{
class AppletIconWidget;
class Containment;

class AMAROK_EXPORT AppletExplorer: public QGraphicsWidget
{
    Q_OBJECT

public:
    AppletExplorer( QGraphicsItem *parent = 0 );
    virtual ~AppletExplorer();

    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    void setContainment( Containment *containment );
    Containment *containment() const;

Q_SIGNALS:
    void addAppletToContainment( const QString &pluginName, const int loc );
    void appletExplorerHid();

protected:
    QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const;

private Q_SLOTS:
    void addApplet( const QString &name );
    void hideMenu();
    void scrollLeft();
    void scrollRight();

private:
    void init();
    QList<AppletIconWidget*> listAppletWidgets();
    Containment *m_containment;
    Plasma::ScrollWidget *m_scrollWidget;
    Q_DISABLE_COPY( AppletExplorer )
};

} // namespace Context

#endif
