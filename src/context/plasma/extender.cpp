/*
 * Copyright 2008 by Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "extender.h"

#include <QAction>
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>

#include "applet.h"
#include "containment.h"
#include "corona.h"
#include "extenderitem.h"
#include "framesvg.h"
#include "popupapplet.h"
#include "svg.h"
#include "widgets/label.h"

#include "private/applet_p.h"
#include "private/extender_p.h"
#include "private/extenderitem_p.h"

namespace Plasma
{

Extender::Extender(Applet *applet)
        : QGraphicsWidget(applet),
          d(new ExtenderPrivate(applet, this))
{
    //At multiple places in the extender code, we make the assumption that an applet doesn't have
    //more then one extender. If a second extender is created, destroy the first one to avoid leaks.
    if (applet->d->extender) {
        kWarning() << "Applet already has an extender, and can have only one extender."
                   << "The previous extender will be destroyed.";
        delete applet->d->extender;
    }
    applet->d->extender = this;

    setContentsMargins(0, 0, 0, 0);
    d->layout = new QGraphicsLinearLayout(this);
    d->layout->setOrientation(Qt::Vertical);
    d->layout->setContentsMargins(0, 0, 0, 0);
    d->layout->setSpacing(0);
    setLayout(d->layout);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    d->emptyExtenderLabel = new Label(this);
    d->emptyExtenderLabel->setText(d->emptyExtenderMessage);
    d->emptyExtenderLabel->setMinimumSize(QSizeF(150, 64));
    d->emptyExtenderLabel->setPreferredSize(QSizeF(200, 64));
    d->emptyExtenderLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    d->layout->addItem(d->emptyExtenderLabel);

    d->loadExtenderItems();
}

Extender::~Extender()
{
    d->applet->d->extender = 0;
    delete d;
}

void Extender::setEmptyExtenderMessage(const QString &message)
{
    d->emptyExtenderMessage = message;

    if (d->emptyExtenderLabel) {
        d->emptyExtenderLabel->setText(message);
    }
}

QString Extender::emptyExtenderMessage() const
{
    return d->emptyExtenderMessage;
}

QList<ExtenderItem*> Extender::items() const
{
    QList<ExtenderItem*> result;

    //FIXME: a triple nested loop ... ew. there should be a more efficient way to do this
    //iterate through all extenders we can find and check each extenders source applet.
    foreach (Containment *c, d->applet->containment()->corona()->containments()) {
        foreach (Applet *applet, c->applets()) {
            if (applet->d->extender) {
                foreach (ExtenderItem *item, applet->d->extender->attachedItems()) {
                    if (item->d->sourceApplet == d->applet) {
                        result.append(item);
                    }
                }
            }
        }
    }

    return result;
}

QList<ExtenderItem*> Extender::attachedItems() const
{
    return d->attachedExtenderItems;
}

QList<ExtenderItem*> Extender::detachedItems() const
{
    QList<ExtenderItem*> result;

    foreach (ExtenderItem *item, items()) {
        if (item->isDetached()) {
            result.append(item);
        }
    }

    return result;
}

ExtenderItem *Extender::item(const QString &name) const
{
    foreach (ExtenderItem *item, items()) {
        if (item->name() == name) {
            return item;
        }
    }

    return 0;
}

void Extender::setExtenderAppearance(Appearance appearance)
{
    if (d->appearance == appearance) {
        return;
    }

    d->appearance = appearance;
    d->updateBorders();
}

Extender::Appearance Extender::extenderAppearance() const
{
    return d->appearance;
}

void Extender::saveState()
{
    foreach (ExtenderItem *item, attachedItems()) {
        item->config().writeEntry("extenderItemPosition", item->geometry().y());
    }
}

QVariant Extender::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        d->adjustSize();
    }

    return QGraphicsWidget::itemChange(change, value);
}

void Extender::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);
    emit geometryChanged();
}

void Extender::itemAddedEvent(ExtenderItem *item, const QPointF &pos)
{
    //this is a sane size policy imo.
    item->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    if (pos == QPointF(-1, -1)) {
        d->layout->addItem(item);
    } else {
        d->layout->insertItem(d->insertIndexFromPos(pos), item);
    }

    //remove the empty extender message if needed.
    if (d->emptyExtenderLabel) {
        d->layout->removeItem(d->emptyExtenderLabel);
        d->emptyExtenderLabel->hide();
    }

    d->adjustSize();
}

void Extender::itemRemovedEvent(ExtenderItem *item)
{
    d->layout->removeItem(item);

    //add the empty extender message if needed.
    if (!attachedItems().count() && !d->spacerWidget) {
        d->emptyExtenderLabel->show();
        d->emptyExtenderLabel->setMinimumSize(item->size());
        //just in case:
        d->layout->removeItem(d->emptyExtenderLabel);
        d->layout->addItem(d->emptyExtenderLabel);
    }

    d->adjustSize();
}

void Extender::itemHoverEnterEvent(ExtenderItem *item)
{
    itemHoverMoveEvent(item, QPointF(0, 0));
}

void Extender::itemHoverMoveEvent(ExtenderItem *item, const QPointF &pos)
{
    int insertIndex = d->insertIndexFromPos(pos);

    if ((insertIndex == d->currentSpacerIndex) || (insertIndex == -1)) {
        //relayouting is resource intensive, so don't do that when not necesarry.
        return;
    }

    //Make sure we remove any spacer that might already be in the layout.
    itemHoverLeaveEvent(item);

    d->currentSpacerIndex = insertIndex;

    //Create a widget that functions as spacer, and add that to the layout.
    QGraphicsWidget *widget = new QGraphicsWidget(this);
    widget->setMinimumSize(QSizeF(item->minimumSize().width(), item->size().height()));
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    d->spacerWidget = widget;
    d->layout->insertItem(insertIndex, widget);

    //Make sure we remove any 'no detachables' label that might be there, and update the layout.
    //XXX: duplicated from itemAttachedEvent.
    if (d->emptyExtenderLabel) {
        d->layout->removeItem(d->emptyExtenderLabel);
        d->emptyExtenderLabel->hide();
    }

    d->adjustSize();
}

void Extender::itemHoverLeaveEvent(ExtenderItem *item)
{
    Q_UNUSED(item);

    if (d->spacerWidget) {
        //Remove any trace of the spacer widget.
        d->layout->removeItem(d->spacerWidget);
        delete d->spacerWidget;
        d->spacerWidget = 0;

        d->currentSpacerIndex = -1;

        //Make sure we add a 'no detachables' label when the layout is empty.
        if (!attachedItems().count()) {
            d->emptyExtenderLabel->show();
            d->emptyExtenderLabel->setMinimumSize(item->size());
            d->layout->removeItem(d->emptyExtenderLabel);
            d->layout->addItem(d->emptyExtenderLabel);
        }

        d->adjustSize();
    }
}

FrameSvg::EnabledBorders Extender::enabledBordersForItem(ExtenderItem *item) const
{
    ExtenderItem *topItem = dynamic_cast<ExtenderItem*>(d->layout->itemAt(0));
    ExtenderItem *bottomItem = dynamic_cast<ExtenderItem*>(d->layout->itemAt(d->layout->count() - 1));
    if (d->appearance == TopDownStacked && bottomItem != item) {
        return FrameSvg::LeftBorder | FrameSvg::BottomBorder | FrameSvg::RightBorder;
    } else if (d->appearance == BottomUpStacked && topItem != item) {
        return FrameSvg::LeftBorder | FrameSvg::TopBorder | FrameSvg::RightBorder;
    } else if (d->appearance != NoBorders) {
        return FrameSvg::LeftBorder | FrameSvg::RightBorder;
    } else {
        return 0;
    }
}

ExtenderPrivate::ExtenderPrivate(Applet *applet, Extender *extender) :
    q(extender),
    applet(applet),
    currentSpacerIndex(-1),
    spacerWidget(0),
    emptyExtenderMessage(i18n("no items")),
    emptyExtenderLabel(0),
    popup(false),
    appearance(Extender::NoBorders)
{
}

ExtenderPrivate::~ExtenderPrivate()
{
}

void ExtenderPrivate::addExtenderItem(ExtenderItem *item, const QPointF &pos)
{
    attachedExtenderItems.append(item);
    q->itemHoverLeaveEvent(item);
    q->itemAddedEvent(item, pos);
    updateBorders();
    emit q->itemAttached(item);
}

void ExtenderPrivate::removeExtenderItem(ExtenderItem *item)
{
    attachedExtenderItems.removeOne(item);

    //collapse the popupapplet if the last item is removed.
    if (!q->attachedItems().count()) {
        PopupApplet *popupApplet = qobject_cast<PopupApplet*>(applet);
        if (popupApplet) {
            popupApplet->hidePopup();
        }
    }

    q->itemRemovedEvent(item);
    updateBorders();
}

int ExtenderPrivate::insertIndexFromPos(const QPointF &pos) const
{
    int insertIndex = -1;

    //XXX: duplicated from panel
    if (pos != QPointF(-1, -1)) {
        for (int i = 0; i < layout->count(); ++i) {
            QRectF siblingGeometry = layout->itemAt(i)->geometry();
            qreal middle = (siblingGeometry.top() + siblingGeometry.bottom()) / 2.0;
            if (pos.y() < middle) {
                insertIndex = i;
                break;
            } else if (pos.y() <= siblingGeometry.bottom()) {
                insertIndex = i + 1;
                break;
            }
        }
    }

    return insertIndex;
}

void ExtenderPrivate::loadExtenderItems()
{
    KConfigGroup cg = applet->config("ExtenderItems");

    //first create a list of extenderItems, and then sort them on their position, so the items get
    //recreated in the correct order.
    //TODO: this restoring of the correct order should now be done in itemAddedEvent instead of
    //here, to allow easy subclassing of Extender.
    QList<QPair<int, QString> > groupList;
    foreach (const QString &extenderItemId, cg.groupList()) {
        KConfigGroup dg = cg.group(extenderItemId);
        groupList.append(qMakePair(dg.readEntry("extenderItemPosition", 0), extenderItemId));
    }
    qSort(groupList);

    //iterate over the extender items
    for (int i = 0; i < groupList.count(); i++) {
        QPair<int, QString> pair = groupList[i];
        KConfigGroup dg = cg.group(pair.second);

        //load the relevant settings.
        QString extenderItemId = dg.name();
        QString extenderItemName = dg.readEntry("extenderItemName", "");
        QString appletName = dg.readEntry("sourceAppletPluginName", "");
        uint sourceAppletId = dg.readEntry("sourceAppletId", 0);

        bool temporarySourceApplet = false;

        //find the source applet.
        Corona *corona = applet->containment()->corona();
        Applet *sourceApplet = 0;
        foreach (Containment *containment, corona->containments()) {
            foreach (Applet *applet, containment->applets()) {
                if (applet->id() == sourceAppletId) {
                    sourceApplet = applet;
                }
            }
        }

        //There is no source applet. We just instantiate one just for the sake of creating
        //detachables.
        if (!sourceApplet) {
            kDebug() << "creating a temporary applet as factory";
            sourceApplet = Applet::load(appletName);
            temporarySourceApplet = true;
            //TODO: maybe add an option to applet to indicate that it shouldn't be deleted after
            //having used it as factory.
        }

        if (!sourceApplet) {
            kDebug() << "sourceApplet is null? appletName = " << appletName;
            kDebug() << "                      extenderItemId = " << extenderItemId;
        } else {
            ExtenderItem *item = new ExtenderItem(q, extenderItemId.toInt());
            item->setName(extenderItemName);
            sourceApplet->initExtenderItem(item);

            if (temporarySourceApplet) {
                delete sourceApplet;
            }
        }
    }

    adjustSize();
}

void ExtenderPrivate::updateBorders()
{
    foreach (ExtenderItem *item, q->attachedItems()) {
        if (item && (item->d->background->enabledBorders() != q->enabledBordersForItem(item))) {
            //call themeChanged to change the backgrounds enabled borders, and move all contained
            //widgets according to it's changed margins.
            item->d->themeChanged();
        }
    }
}

void ExtenderPrivate::adjustSize()
{
    if (q->layout()) {
        q->layout()->updateGeometry();
        //FIXME: for some reason the preferred size hint get's set correctly,
        //but the minimumSizeHint doesn't. Investigate why.
        q->setMinimumSize(q->layout()->preferredSize());
    }

    emit q->geometryChanged();
}

} // Plasma namespace

#include "extender.moc"
