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

#include "Containment.h"

#include "debug.h"

#include "plasma/corona.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsScene>
#include <KMenu>

namespace Context
{

Containment::Containment( QGraphicsItem* parent, const QString& serviceId, uint containmentId ) 
        : Plasma::Containment( parent, serviceId, containmentId )
{}

Containment::Containment( QObject* parent, const QVariantList& args )
        : Plasma::Containment( parent, args )
{}

Containment::~Containment()
{
}


// // reimplemented and modified from Plasma::Containment
// void Containment::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
// {
// 
//     QPointF point = event->screenPos();
// 
//     QGraphicsItem* item = scene()->itemAt(point);
//     if (item == this) {
//         item = 0;
//     }
//     
//     Applet* applet = 0;
//     
//     while (item) {
//         applet = qgraphicsitem_cast<Applet*>(item);
//         if (applet) {
//             break;
//         }
//         
//         item = item->parentItem();
//     }
//     
//     KMenu desktopMenu;
//     //kDebug() << "context menu event " << immutable;
//     if (!applet) {
//         if (!scene() || static_cast<Plasma::Corona*>(scene())->isImmutable()) {
//             QGraphicsItem::contextMenuEvent(event);
//             return;
//         }
//         
//         desktopMenu.addAction( m_appletBrowserAction );
//     } else if (applet->isImmutable()) {
//         QGraphicsItem::contextMenuEvent(event);
//         return;
//     } else {
//         bool hasEntries = false;
//         if (applet->hasConfigurationInterface()) {
//             QAction* configureApplet = new QAction(i18n("%1 Settings...", applet->name()), &desktopMenu);
//             connect(configureApplet, SIGNAL(triggered(bool)),
//                     applet, SLOT(showConfigurationInterface()));
//             desktopMenu.addAction(configureApplet);
//             hasEntries = true;
//         }
//         
//         if (scene() && !static_cast<Plasma::Corona*>(scene())->isImmutable()) {
//             QAction* closeApplet = new QAction(i18n("Remove this %1", applet->name()), &desktopMenu);
//             connect(closeApplet, SIGNAL(triggered(bool)),
//                     applet, SLOT(destroy()));
//             desktopMenu.addAction(closeApplet);
//             hasEntries = true;
//         }
//         
//         QList<QAction*> actions = applet->contextActions();
//         if (!actions.isEmpty()) {
//             desktopMenu.addSeparator();
//             desktopMenu.addAction( m_appletBrowserAction );
//             hasEntries = true;
//         }
//         
//         if (!hasEntries) {
//             QGraphicsItem::contextMenuEvent(event);
//             return;
//         }
//     }
//     
//     event->accept();
//     desktopMenu.exec(event->screenPos());
// }


} // namespace Context
#include "Containment.moc"
