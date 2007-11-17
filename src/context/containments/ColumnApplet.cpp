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

#include "ColumnApplet.h"

#include "ContextScene.h"
#include "debug.h"
#include "Svg.h"

#include "plasma/widgets/layoutanimator.h"
#include "plasma/phase.h"

#include <KAuthorized>
#include <KMenu>

#include <QAction>
#include <QGraphicsScene>
#include <QTimeLine>

namespace Context
{

ColumnApplet::ColumnApplet( QObject *parent, const QVariantList &args )
    : Context::Containment( parent, args )
    , m_defaultColumnSize( 450 )
    , m_actions( 0 )
{
    DEBUG_BLOCK
    m_columns = new Plasma::FlowLayout( this );
    m_columns->setColumnWidth( m_defaultColumnSize );
    
    m_background = new Svg( "widgets/amarok-wallpaper", this );
    m_logo = new Svg( "widgets/amarok-logo", this );
    m_logo->resize();
    m_width = 300; // TODO hardcoding for now, do we want this configurable?
    m_aspectRatio = (qreal)m_logo->size().height() / (qreal)m_logo->size().width();
    m_logo->resize( (int)m_width, (int)( m_width * m_aspectRatio ) );

    connect( this, SIGNAL( appletAdded( Plasma::Applet* ) ), this, SLOT( addApplet( Plasma::Applet* ) ) );


    m_appletBrowserAction = new QAction(i18n("Add applet"), this);
    connect(m_appletBrowserAction, SIGNAL(triggered(bool)), this, SLOT(launchAppletBrowser()));
    // set up default context menu actions
    m_actions = new QList<QAction*>();
    m_actions->append( m_appletBrowserAction );

    m_appletBrowser = new Plasma::AppletBrowser( this );
    m_appletBrowser->setApplication( "amarok" );
    m_appletBrowser->hide();
}

// fetches size from scene and creates columns according.
// should only be called *ONCE* by the ContextView once it
// has been added to the scene
// void ColumnApplet::init() // SLOT
// {
    // TODO wait until this is completely implemented in plasma
// and for it to not crash....
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
// 	
//         Plasma::LayoutAnimator* animator = new Plasma::LayoutAnimator;
// 	QTimeLine* timeLine = new QTimeLine;
// 	animator->setTimeLine(timeLine);
//         animator->setEffect( Plasma::LayoutAnimator::InsertedState , Plasma::LayoutAnimator::FadeInMoveEffect );
//         animator->setEffect( Plasma::LayoutAnimator::StandardState , Plasma::LayoutAnimator::MoveEffect );
//         animator->setEffect( Plasma::LayoutAnimator::RemovedState , Plasma::LayoutAnimator::FadeOutMoveEffect );
// 	column->setAnimator(animator);    
//     }
// }

void ColumnApplet::saveToConfig( KConfig& conf )
{
    DEBUG_BLOCK
    debug() << "number of m_columns:" << m_columns->count();
    for( int i = 0; i < m_columns->count(); i++ )
    {
        Applet* applet = dynamic_cast< Applet* >( m_columns->itemAt( i ) );
        debug() << "trying to save an applet";
        if( applet != 0 )
        {
            KConfigGroup cg( &conf, QString::number( applet->id() ) );
            debug() << "saving applet" << applet->name();
            cg.writeEntry( "plugin", applet->pluginName() );
        }
    }
    conf.sync();
    
        // TODO port
//     for( int i = 0; i < m_layout.size(); i++ )
//     {
//         for( int k = 0; k < m_layout[ i ]->count() ; k++ )
//         {
//             Applet* applet = dynamic_cast< Applet* >( m_layout[ i ]->itemAt( k ) );
//             if( applet != 0 )
//             {
//                 KConfigGroup cg( &conf, QString::number( applet->id() ) );
//                 debug() << "saving applet" << applet->name();
//                 cg.writeEntry( "plugin", applet->pluginName() );
//                 cg.writeEntry( "column", QString::number( i ) );
//                 cg.writeEntry( "position", QString::number( k ) );
//             }
//         }
//     }
//     conf.sync();
}

void ColumnApplet::loadConfig( KConfig& conf )
{
    DEBUG_BLOCK
    foreach( const QString& group, conf.groupList() )
    {
        KConfigGroup cg( &conf, group );
        QString plugin = cg.readEntry( "plugin", QString() );
        debug() << "loading applet:" << plugin
            << QStringList() << group.toUInt();
        if( plugin != QString() )
            Plasma::Containment::addApplet( plugin );
    }
        // TODO port
    /*
    m_layout.clear();
    init();
    foreach( const QString& group, conf.groupList() )
    {
        KConfigGroup cg( &conf, group );
        debug() << "loading applet:" << cg.readEntry( "plugin", QString() )
            << QStringList() << group.toUInt() << cg.readEntry( "column", QString() ) << cg.readEntry( "position", QString() );

        ContextScene* scene = qobject_cast< ContextScene* >( this->scene() );
        if( scene != 0 )
        {
            Applet* applet = scene->addApplet( cg.readEntry( "plugin", QString() ),
                              QVariantList(),
                              group.toUInt(),
                              QRectF() );
            int column = cg.readEntry( "column", 1000 );
            int pos = cg.readEntry( "position", 1000 );
            debug() << "restoring applet to column:" << column << "and position:" << pos;
            if( column >= m_layout.size() ) // was saved in a column that is not available now
                addApplet( applet );
            else
            {
                if( pos < m_layout[ column ]->count() ) // there is a position for it
                    m_layout[ column ]->insertItem( pos, applet );
                else // just append it, there is no position for it
                    m_layout[ column ]->addItem( applet );
            }
        }
    }
    resizeColumns();*/
}

QSizeF ColumnApplet::sizeHint() const
{
    debug() << "returning size hint:" << m_geometry.size();
    return m_geometry.size();
}

QRectF ColumnApplet::boundingRect() const 
{
//     qreal width = 2*m_padding;
//     qreal height = 0;
//     foreach( Plasma::VBoxLayout* column, m_layout )
//     {
//         width += column->sizeHint().width();

    return m_geometry;
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnApplet::updateSize() // SLOT
{
    DEBUG_BLOCK
    m_geometry = scene()->sceneRect();
    debug() << "setting geometry:" << scene()->sceneRect();
    if( scene() ) m_columns->setGeometry( scene()->sceneRect() );
    setGeometry( scene()->sceneRect() );
}

void ColumnApplet::paintInterface(QPainter *painter,
                    const QStyleOptionGraphicsItem *option,
                    const QRect& rect)
{
/*    debug() << "painting in:" << rect;*/
    painter->save();
    m_background->paint( painter, rect );
    painter->restore();
    QSize size = m_logo->size();
    
    QSize pos = rect.size() - size;
    qreal newHeight  = m_aspectRatio * m_width;
    m_logo->resize( QSize( (int)m_width, (int)newHeight ) );
    painter->save();
/*    debug() << "painting logo:" << QRectF( pos.width() - 10.0, pos.height() - 5.0, size.width(), size.height() );*/
    m_logo->paint( painter, QRectF( pos.width() - 10.0, pos.height() - 5.0, size.width(), size.height() ) );
    painter->restore();
    
}

void ColumnApplet::appletRemoved( QObject* object ) // SLOT
{
    Q_UNUSED( object )
    // basically we want to reshuffle the columns since we know something is gone
//     resizeColumns();
}
/*
// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    DEBUG_BLOCK
    if (event->button() == Qt::LeftButton && (flags() & ItemIsSelectable))
    {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (!multiSelect) {
            if (!isSelected()) {
                if (scene())
                    scene()->clearSelection();
                setSelected(true);
            }
        }
    } else if (!(flags() & ItemIsMovable)) {
        event->ignore();
    }
}

// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mouseMoveEvent( QGraphicsSceneMouseEvent * event )
{
//     debug() << "layout manager got a mouse event";
    if ( ( event->buttons() & Qt::LeftButton ) )
    {

        // Find the active view.
        QGraphicsView *view = 0;
        if (event->widget())
            view = qobject_cast<QGraphicsView *>(event->widget()->parentWidget());

        if ((flags() & ItemIsMovable) && (!parentItem() || !parentItem()->isSelected())) {
            QPointF diff;
            if (flags() & ItemIgnoresTransformations) {
                    // Root items that ignore transformations need to
                    // calculate their diff by mapping viewport coordinates to
                    // parent coordinates. Items whose ancestors ignore
                    // transformations can ignore this problem; their events
                    // are already mapped correctly.
                QTransform viewToParentTransform = (sceneTransform() * view->viewportTransform()).inverted();

                QTransform myTransform = transform().translate(pos().x(), pos().y());
                viewToParentTransform = myTransform * viewToParentTransform;

                diff = viewToParentTransform.map(QPointF(view->mapFromGlobal(event->screenPos())))
                    - viewToParentTransform.map(QPointF(view->mapFromGlobal(event->lastScreenPos())));
            } else
            {
                diff = mapToParent(event->pos()) - mapToParent(event->lastPos());
            }

            moveBy(diff.x(), diff.y());

            if (flags() & ItemIsSelectable)
                setSelected(true);
        }
    }
}

*/


Applet* ColumnApplet::addApplet( Applet* applet )
{
    DEBUG_BLOCK
    debug() << "m_columns:" << m_columns;
     m_columns->addItem( applet );

    connect( applet, SIGNAL( changed() ), this, SLOT( recalculate() ) );
    
    recalculate();
    return applet;
}

void ColumnApplet::recalculate()
{
    DEBUG_BLOCK
    debug() << "got child item that wants a recalculation";
    m_columns->setGeometry( m_columns->geometry() );
}

QList<QAction*> ColumnApplet::contextActions()
{
    DEBUG_BLOCK
    return *m_actions;
}

void ColumnApplet::launchAppletBrowser() // SLOT
{
    DEBUG_BLOCK
    m_appletBrowser->show();
}

void ColumnApplet::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    //kDebug() << "let's see if we manage to get a context menu here, huh";
    if (!scene() || !KAuthorized::authorizeKAction("desktop_contextmenu")) {
        QGraphicsItem::contextMenuEvent(event);
        return;
    }

    QPointF point = event->scenePos();
    QGraphicsItem* item = scene()->itemAt(point);
    if (item == this) {
        item = 0;
    }

    Applet* applet = 0;

    while (item) {
        applet = qgraphicsitem_cast<Applet*>(item);
        if (applet) {
            break;
        }

        item = item->parentItem();
    }

    KMenu desktopMenu;
    //kDebug() << "context menu event " << immutable;
    if (!applet) {
        if (!scene() || static_cast<ContextScene*>(scene())->isImmutable()) {
            kDebug() << "immutability";
            QGraphicsItem::contextMenuEvent(event);
            return;
        }

        //FIXME: change this to show this only in debug mode (or not at all?)
        //       before final release
        QList<QAction*> actions = contextActions();

        if (actions.count() < 1) {
            kDebug() << "no applet, but no actions";
            QGraphicsItem::contextMenuEvent(event);
            return;
        }

        foreach(QAction* action, actions) {
            desktopMenu.addAction(action);
        }
    } else if (applet->isImmutable()) {
        kDebug() << "immutable applet";
        QGraphicsItem::contextMenuEvent(event);
        return;
    } else {
        bool hasEntries = false;
        if (applet->hasConfigurationInterface()) {
            QAction* configureApplet = new QAction(i18n("%1 Settings...", applet->name()), &desktopMenu);
            connect(configureApplet, SIGNAL(triggered(bool)),
                    applet, SLOT(showConfigurationInterface()));
            desktopMenu.addAction(configureApplet);
            hasEntries = true;
        }

        if (scene() && !static_cast<ContextScene*>(scene())->isImmutable()) {
            QAction* closeApplet = new QAction(i18n("Remove this %1", applet->name()), &desktopMenu);
            QVariant appletV;
            appletV.setValue((QObject*)applet);
            closeApplet->setData(appletV);
            connect(closeApplet, SIGNAL(triggered(bool)),
                    this, SLOT(destroyApplet()));
            desktopMenu.addAction(closeApplet);
            hasEntries = true;
        }

        QList<QAction*> actions = applet->contextActions();
        if (!actions.isEmpty()) {
            desktopMenu.addSeparator();
            foreach(QAction* action, actions) {
                desktopMenu.addAction(action);
            }
            hasEntries = true;
        }

        if (!hasEntries) {
            QGraphicsItem::contextMenuEvent(event);
            kDebug() << "no entries";
            return;
        }
    }

    event->accept();
    kDebug() << "executing at" << event->screenPos();
    desktopMenu.exec(event->screenPos());
}


bool ColumnApplet::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Applet *applet = qgraphicsitem_cast<Applet*>(watched);
    //QEvent::GraphicsSceneHoverEnter

    // Otherwise we're watching something we shouldn't be...
    Q_ASSERT(applet!=0);

    return false;
}
void ColumnApplet::destroyApplet()
{
    QAction *action = qobject_cast<QAction*>(sender());

    if (!action) {
        return;
    }

    Applet *applet = qobject_cast<Applet*>(action->data().value<QObject*>());
    Plasma::Phase::self()->animateItem(applet, Plasma::Phase::Disappear);
}

void ColumnApplet::appletDisappearComplete(QGraphicsItem *item, Plasma::Phase::Animation anim)
{
    if (anim == Plasma::Phase::Disappear) {
        if (item->parentItem() == this) {
            Applet *applet = qgraphicsitem_cast<Applet*>(item);

            if (applet) {
                applet->destroy();
            }
        }
    }
}


} // Context namespace

#include "ColumnApplet.moc"
