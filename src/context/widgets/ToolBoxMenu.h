/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

#ifndef AMAROK_TOOLBOX_MENU_H
#define AMAROK_TOOLBOX_MENU_H

#include "amarok_export.h"
#include "Containment.h"
#include "ToolBoxIcon.h"

#include <QList>
#include <QMap>
#include <QHash>
#include <QTimer>
#include <QStack>

namespace Context
{
    
class AMAROK_EXPORT AmarokToolBoxMenu : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    enum ScrollDirection
    {
        ScrollDown = 0,
        ScrollUp
    };

    explicit AmarokToolBoxMenu( QGraphicsItem *parent = 0, bool runningAppletsOnly = false );
    
    ~AmarokToolBoxMenu();
    QRectF boundingRect() const;

    Containment *containment() const;
    void setContainment( Containment *newContainment );
    bool showing() const;

public slots:
    void show( bool refreshApplets = true );
    void hide();

Q_SIGNALS:
    void menuHidden();
    void addAppletToContainment( const QString& );
    void installApplets();
    
protected:
    virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    virtual void wheelEvent( QGraphicsSceneWheelEvent *event );

private slots:
    void removeApplet( const QString &pluginName );
    void appletAdded( Plasma::Applet *applet );
    void appletRemoved( Plasma::Applet *applet );
    void delayedScroll();
    void scrollDown();
    void scrollUp();
    void timeToHide();
    void addApplet( const QString &pluginName );
    
private:
    void init( QMap< QString, QString > allAppletsList, QStringList appletsToShow );
    void createArrow( ToolBoxIcon *arrow, const QString &direction );
    void initRunningApplets();
    void populateMenu();
    void repopulateMenu(); // used to refresh current applets
    void setupMenuEntry( ToolBoxIcon *entry, const QString &appletName );

    QMap<QString, QString> m_appletsList;
    QHash<Plasma::Applet *, QString> m_appletNames;

    Containment *m_containment;

    bool m_removeApplets;
    
    int m_menuSize;

    QStack<QString> m_bottomMenu;
    QStack<QString> m_topMenu;
    QList<ToolBoxIcon *> m_currentMenu;

    ToolBoxIcon *m_hideIcon;
    ToolBoxIcon *m_upArrow;
    ToolBoxIcon *m_downArrow;
    ToolBoxIcon* m_installScriptedApplet;
    
    QMap<Plasma::Containment *, QStringList> m_runningApplets;

    QTimer *m_timer;
    QTimer *m_scrollDelay;
    QList<ScrollDirection> m_pendingScrolls;
    bool m_showing;
    int m_delay;
};

}

#endif
