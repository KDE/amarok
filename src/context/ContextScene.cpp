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

#include "ContextScene.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMimeData>

#include <KMimeType>

namespace Context
{

ContextScene::ContextScene(QObject * parent)
    : Plasma::Corona( parent )
{
}

ContextScene::ContextScene(const QRectF & sceneRect, QObject * parent )
    : Plasma::Corona( sceneRect, parent )
{
}

ContextScene::ContextScene(qreal x, qreal y, qreal width, qreal height, QObject * parent)
    : Plasma::Corona( x, y, width, height, parent )
{
}

ContextScene::~ContextScene()
{
    clear();
}

Applet* ContextScene::addApplet(const QString& name, const QStringList& args)
{
    Applet* applet = Corona::addApplet( name, args );
    loaded << applet;
    return applet;
}

void ContextScene::clear()
{    
    DEBUG_BLOCK
    foreach( Applet* applet, loaded )
        delete applet;
    loaded.clear();
}

void ContextScene::clear( const ContextState& state )
{
    QString name;
    if( state == Home )
        name = "home";
    else if( state == Current )
        name = "current";
    else
        return; // startup, or some other wierd case
    
    QStringList applets;
    foreach( Applet* applet, loaded )
    {
        QString key = QString( "%1_%2" ).arg( name, applet->name() );
        applets << applet->name();
        QStringList pos;
        pos << QString::number( applet->x() ) << QString::number( applet->y() );
        Amarok::config( "Context Applets" ).writeEntry( key, pos );
        debug() << "saved applet: " << key << " at position: " << pos << endl;
        applet->deleteLater();
    }
    debug() << "saved list of applets: " << applets << endl;
    Amarok::config( "Context Applets" ).writeEntry( name, applets );
    Amarok::config( "Context Applets" ).sync();
    loaded.clear();
}

    
/*
void ContextScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event)
{
    debug << "ContextScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)" << endl;
    if (event->mimeData()->hasFormat("text/x-amarokappletservicename") ||
        KUrl::List::canDecode(event->mimeData())) {
            event->acceptProposedAction();
        //TODO Create the applet, move to mouse position then send the 
        //     following event to lock it to the mouse
        //QMouseEvent event(QEvent::MouseButtonPress, event->pos(), Qt::LeftButton, event->mouseButtons(), 0);
        //QApplication::sendEvent(this, &event);
        }
    //TODO Allow dragging an applet from another Corona into this one while
    //     keeping its settings etc.
}

void ContextScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    debug << "ContextScene::dropEvent(QDropEvent* event)" << endl;
    if (event->mimeData()->hasFormat("text/x-amarokappletservicename")) {
        //TODO This will pretty much move into dragEnterEvent()
        QString plasmoidName;
        plasmoidName = event->mimeData()->data("text/x-amarokappletservicename");
        addPlasmoid(plasmoidName);
        d->applets.last()->setPos(event->pos());
        
        event->acceptProposedAction();
    } else if (KUrl::List::canDecode(event->mimeData())) {
        KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());	
        foreach (const KUrl& url, urls) {
            QStringList args;
            args << url.url();
            Applet* button = addPlasmoid("url", args);
            if (button) {
                //button->setSize(128,128);
                button->setPos(event->scenePos() - QPoint(button->boundingRect().width()/2,
                    button->boundingRect().height()/2));
            }
            addItem(button);
        }
        event->acceptProposedAction();
    }
}

void ContextScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
    
    QPointF point = contextMenuEvent->scenePos();
    /*
     * example for displaying the SuperKaramba context menu
    QGraphicsItem *item = itemAt(point);
    if(item) {
        QObject *object = dynamic_cast<QObject*>(item->parentItem());
        if(object && object->objectName().startsWith("karamba")) {
            QContextMenuEvent event(QContextMenuEvent::Mouse, point);
            contextMenuEvent(&event);
            return;
        }
    }
    *
    contextMenuEvent->accept();
    QGraphicsItem* item = itemAt(point);
    Applet* applet = 0;
    
    while (item) {
        applet = qgraphicsitem_cast<Applet*>(item);
        if (applet) {
            break;
        }
        
        item = item->parentItem();
    }
    
    KMenu desktopMenu;
    //debug << "context menu event " << d->immutable << endl;
    if (!applet) {
        if (d->immutable) {
            return;
        }
        
        desktopMenu.addAction(d->engineExplorerAction);
    } else if (applet->immutable()) {
        return;
    } else {
        //desktopMenu.addSeparator();
        bool hasEntries = false;
        if (applet->hasConfigurationInterface()) {
            QAction* configureApplet = new QAction(i18n("%1 Settings...", applet->name()), this);
            connect(configureApplet, SIGNAL(triggered(bool)),
                    applet, SLOT(showConfigurationInterface()));
            desktopMenu.addAction(configureApplet);
            hasEntries = true;
        }
        
        if (!d->immutable) {
            QAction* closeApplet = new QAction(i18n("Close this %1", applet->name()), this);
            connect(closeApplet, SIGNAL(triggered(bool)),
                    applet, SLOT(deleteLater()));
            desktopMenu.addAction(closeApplet);
            hasEntries = true;
        }
        
        if (!hasEntries) {
            return;
        }
    }
    desktopMenu.exec(point.toPoint());
} */

} // Context namespace

#include "ContextScene.moc"

