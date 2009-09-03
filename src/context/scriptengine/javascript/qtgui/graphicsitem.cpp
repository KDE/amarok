/****************************************************************************************
 * Copyright (c) 2007 Richard J. Moore <rich@kde.org>                                   *
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

#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptContext>
#include <QtGui/QCursor>
#include <QtGui/QGraphicsItem>
#include <QtGui/QGraphicsScene>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QScript::Pointer<QGraphicsItem>::wrapped_pointer_type)
Q_DECLARE_METATYPE(QList<QGraphicsItem*>)
Q_DECLARE_METATYPE(QPainterPath)
#ifndef QT_NO_CURSOR
Q_DECLARE_METATYPE(QCursor)
#endif
Q_DECLARE_METATYPE(QGraphicsItemGroup*)
Q_DECLARE_METATYPE(QPainter*)
Q_DECLARE_METATYPE(QStyleOptionGraphicsItem*)

Q_DECLARE_METATYPE(QGraphicsPathItem*)
Q_DECLARE_METATYPE(QGraphicsRectItem*)
Q_DECLARE_METATYPE(QGraphicsEllipseItem*)
Q_DECLARE_METATYPE(QGraphicsPolygonItem*)
Q_DECLARE_METATYPE(QGraphicsLineItem*)
Q_DECLARE_METATYPE(QGraphicsPixmapItem*)
Q_DECLARE_METATYPE(QGraphicsTextItem*)
Q_DECLARE_METATYPE(QGraphicsSimpleTextItem*)

DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, acceptDrops, setAcceptDrops)
DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, acceptsHoverEvents, setAcceptsHoverEvents)
DECLARE_GET_METHOD(QGraphicsItem, boundingRect)
DECLARE_GET_METHOD(QGraphicsItem, children)
DECLARE_GET_METHOD(QGraphicsItem, childrenBoundingRect)
#ifndef QT_NO_CURSOR
DECLARE_GET_SET_METHODS(QGraphicsItem, QCursor, cursor, setCursor)
DECLARE_BOOLEAN_GET_METHOD(QGraphicsItem, hasCursor)
#endif
DECLARE_GET_SET_METHODS(QGraphicsItem, QGraphicsItemGroup*, group, setGroup)
DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, handlesChildEvents, setHandlesChildEvents)
DECLARE_BOOLEAN_GET_METHOD(QGraphicsItem, hasFocus)
DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, isEnabled, setEnabled)
DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, isSelected, setSelected)
DECLARE_BOOLEAN_GET_SET_METHODS(QGraphicsItem, isVisible, setVisible)
DECLARE_GET_METHOD(QGraphicsItem, opaqueArea)
DECLARE_GET_METHOD(QGraphicsItem, pos)
DECLARE_QOBJECT_GET_METHOD(QGraphicsItem, scene)
DECLARE_GET_METHOD(QGraphicsItem, sceneBoundingRect)
DECLARE_GET_METHOD(QGraphicsItem, scenePos)
DECLARE_GET_METHOD(QGraphicsItem, sceneTransform)
DECLARE_GET_METHOD(QGraphicsItem, shape)
#ifndef QT_NO_TOOLTIP
DECLARE_STRING_GET_SET_METHODS(QGraphicsItem, toolTip, setToolTip)
#endif
DECLARE_GET_METHOD(QGraphicsItem, topLevelItem)
DECLARE_GET_SET_METHODS(QGraphicsItem, QTransform, transform, setTransform)
DECLARE_NUMBER_GET_METHOD(QGraphicsItem, type)
DECLARE_NUMBER_GET_METHOD(QGraphicsItem, x)
DECLARE_NUMBER_GET_METHOD(QGraphicsItem, y)
DECLARE_NUMBER_GET_SET_METHODS(QGraphicsItem, zValue, setZValue)

DECLARE_BOOLEAN_1ARG_METHOD(QGraphicsItem, QPointF, contains)
DECLARE_VOID_METHOD(QGraphicsItem, clearFocus)
DECLARE_VOID_METHOD(QGraphicsItem, hide)
DECLARE_BOOLEAN_1ARG_METHOD(QGraphicsItem, QGraphicsItem*, isAncestorOf)
DECLARE_BOOLEAN_1ARG_METHOD(QGraphicsItem, QGraphicsItem*, isObscuredBy)
DECLARE_VOID_NUMBER_NUMBER_METHOD(QGraphicsItem, moveBy)
DECLARE_VOID_METHOD(QGraphicsItem, resetTransform)
#ifndef QT_NO_CURSOR
DECLARE_VOID_METHOD(QGraphicsItem, unsetCursor)
#endif
DECLARE_VOID_METHOD(QGraphicsItem, show)
DECLARE_VOID_NUMBER_NUMBER_METHOD(QGraphicsItem, translate)
DECLARE_VOID_NUMBER_NUMBER_METHOD(QGraphicsItem, scale)
DECLARE_VOID_NUMBER_NUMBER_METHOD(QGraphicsItem, shear)
DECLARE_VOID_1ARG_METHOD(QGraphicsItem, QGraphicsItem*, installSceneEventFilter)
DECLARE_VOID_1ARG_METHOD(QGraphicsItem, QGraphicsItem*, removeSceneEventFilter)
DECLARE_VOID_NUMBER_METHOD(QGraphicsItem, rotate)

/////////////////////////////////////////////////////////////

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QGraphicsItem cannot be instantiated");
}

BEGIN_DECLARE_METHOD(QGraphicsItem, acceptedMouseButtons) {
    return QScriptValue(eng, static_cast<int>(self->acceptedMouseButtons()));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, advance) {
    self->advance(ctx->argument(0).toInt32());
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, collidesWithItem) {
    QGraphicsItem *other = qscriptvalue_cast<QGraphicsItem*>(ctx->argument(0));
    if (!other) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QGraphicsItem.prototype.collidesWithItem: argument is not a GraphicsItem");
    }
    if (ctx->argument(1).isUndefined())
        return QScriptValue(eng, self->collidesWithItem(other));
    else
        return QScriptValue(eng, self->collidesWithItem(other, static_cast<Qt::ItemSelectionMode>(ctx->argument(1).toInt32())));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, collidesWithPath) {
    QPainterPath path = qscriptvalue_cast<QPainterPath>(ctx->argument(0));
    if (ctx->argument(1).isUndefined())
        return QScriptValue(eng, self->collidesWithPath(path));
    else
        return QScriptValue(eng, self->collidesWithPath(path, static_cast<Qt::ItemSelectionMode>(ctx->argument(1).toInt32())));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, collidingItems) {
    if (ctx->argument(0).isUndefined())
        return qScriptValueFromValue(eng, self->collidingItems());
    else
        return qScriptValueFromValue(eng, self->collidingItems(static_cast<Qt::ItemSelectionMode>(ctx->argument(0).toInt32())));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, data) {
    return eng->newVariant(self->data(ctx->argument(0).toInt32()));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, ensureVisible) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.ensureVisible is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, flags) {
    return QScriptValue(eng, static_cast<int>(self->flags()));
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, isObscured) {
    if (ctx->argumentCount() == 0) {
        return QScriptValue(eng, self->isObscured());
    } else if (ctx->argumentCount() > 1) {
        return QScriptValue(eng, self->isObscured(ctx->argument(0).toInt32(),
                                                  ctx->argument(1).toInt32(),
                                                  ctx->argument(2).toInt32(),
                                                  ctx->argument(3).toInt32()));
    } else {
        return QScriptValue(eng, self->isObscured(qscriptvalue_cast<QRectF>(ctx->argument(0))));
    }
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapFromItem) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapFromItem is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapFromParent) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapFromParent is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapFromScene) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapFromScene is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapToItem) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapToItem is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapToParent) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapToParent is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, mapToScene) {
    Q_UNUSED(eng);
    return ctx->throwError("QGraphicsItem.prototype.mapToScene is not implemented");
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, paint) {
    self->paint(qscriptvalue_cast<QPainter*>(ctx->argument(0)),
                qscriptvalue_cast<QStyleOptionGraphicsItem*>(ctx->argument(1)),
                qscriptvalue_cast<QWidget*>(ctx->argument(2)));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, parentItem) {
    QGraphicsItem *parent = self->parentItem();
    if (!parent)
        return eng->nullValue();
    QScriptValue ret = qScriptValueFromValue(eng, parent);
    QScriptValue proto;
    switch (parent->type()) {
    case 2:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsPathItem*>());
        break;
    case 3:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsRectItem*>());
        break;
    case 4:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsEllipseItem*>());
        break;
    case 5:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsPolygonItem*>());
        break;
    case 6:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsLineItem*>());
        break;
    case 7:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsPixmapItem*>());
        break;
    case 8:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsTextItem*>());
        break;
    case 9:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsSimpleTextItem*>());
        break;
    case 10:
        proto = eng->defaultPrototype(qMetaTypeId<QGraphicsItemGroup*>());
        break;
    }
    if (proto.isValid())
        ret.setPrototype(proto);
    return ret;
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setAcceptedMouseButtons) {
    self->setAcceptedMouseButtons(static_cast<Qt::MouseButtons>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setData) {
    self->setData(ctx->argument(0).toInt32(), ctx->argument(1).toVariant());
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setFlag) {
    QGraphicsItem::GraphicsItemFlag flag = static_cast<QGraphicsItem::GraphicsItemFlag>(ctx->argument(0).toInt32());
    if (ctx->argument(1).isUndefined())
        self->setFlag(flag);
    else
        self->setFlag(flag, ctx->argument(1).toBoolean());
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setFlags) {
    self->setFlags(static_cast<QGraphicsItem::GraphicsItemFlags>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setFocus) {
    if (ctx->argument(0).isUndefined())
        self->setFocus();
    else
        self->setFocus(static_cast<Qt::FocusReason>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setParentItem) {
    QScriptValue arg = ctx->argument(0);
    QGraphicsItem *item = qscriptvalue_cast<QGraphicsItem*>(arg);
    self->setParentItem(item);
    if (item)
        QScript::maybeReleaseOwnership(ctx->thisObject());
    else if (!self->scene())
        QScript::maybeTakeOwnership(ctx->thisObject());
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, setPos) {
    if (ctx->argumentCount() > 1)
        self->setPos(ctx->argument(0).toNumber(), ctx->argument(1).toNumber());
    else
        self->setPos(qscriptvalue_cast<QPointF>(ctx->argument(0)));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, update) {
    if (ctx->argumentCount() > 1) {
        self->update(ctx->argument(0).toNumber(),
                     ctx->argument(1).toNumber(),
                     ctx->argument(2).toNumber(),
                     ctx->argument(3).toNumber());
    } else {
        self->update(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, toString) {
    return QScriptValue(eng, "QGraphicsItem");
} END_DECLARE_METHOD

/////////////////////////////////////////////////////////////

class PrototypeGraphicsItem : public QGraphicsItem
{
public:
    PrototypeGraphicsItem()
    { }
    QRectF boundingRect() const
    { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
    { }
};

QScriptValue constructGraphicsItemClass(QScriptEngine *eng)
{
    QScriptValue proto = QScript::wrapGVPointer<QGraphicsItem>(eng, new PrototypeGraphicsItem());
    ADD_GET_SET_METHODS(proto, acceptDrops, setAcceptDrops);
    ADD_GET_SET_METHODS(proto, acceptsHoverEvents, setAcceptsHoverEvents);
    ADD_GET_METHOD(proto, boundingRect);
    ADD_GET_METHOD(proto, children);
    ADD_GET_METHOD(proto, childrenBoundingRect);
#ifndef QT_NO_CURSOR
    ADD_GET_SET_METHODS(proto, cursor, setCursor);
    ADD_GET_METHOD(proto, hasCursor);
#endif
    ADD_GET_SET_METHODS(proto, group, setGroup);
    ADD_GET_SET_METHODS(proto, handlesChildEvents, setHandlesChildEvents);
    ADD_GET_METHOD(proto, hasFocus);
    ADD_GET_SET_METHODS(proto, isEnabled, setEnabled);
    ADD_GET_SET_METHODS(proto, isSelected, setSelected);
    ADD_GET_SET_METHODS(proto, isVisible, setVisible);
    ADD_GET_METHOD(proto, opaqueArea);
    ADD_GET_METHOD(proto, pos);
    ADD_GET_METHOD(proto, scene);
    ADD_GET_METHOD(proto, sceneBoundingRect);
    ADD_GET_METHOD(proto, scenePos);
    ADD_GET_METHOD(proto, sceneTransform);
    ADD_GET_METHOD(proto, shape);
#ifndef QT_NO_TOOLTIP
    ADD_GET_SET_METHODS(proto, toolTip, setToolTip);
#endif
    ADD_GET_METHOD(proto, topLevelItem);
    ADD_GET_SET_METHODS(proto, transform, setTransform);
    ADD_GET_METHOD(proto, type);
    ADD_GET_METHOD(proto, x);
    ADD_GET_METHOD(proto, y);
    ADD_GET_SET_METHODS(proto, zValue, setZValue);

    ADD_METHOD(proto, acceptedMouseButtons);
    ADD_METHOD(proto, advance);
    ADD_METHOD(proto, clearFocus);
    ADD_METHOD(proto, collidesWithItem);
    ADD_METHOD(proto, collidesWithPath);
    ADD_METHOD(proto, collidingItems);
    ADD_METHOD(proto, contains);
    ADD_METHOD(proto, data);
    ADD_METHOD(proto, ensureVisible);
    ADD_METHOD(proto, flags);
    ADD_METHOD(proto, hide);
    ADD_METHOD(proto, installSceneEventFilter);
    ADD_METHOD(proto, isAncestorOf);
    ADD_METHOD(proto, isObscured);
    ADD_METHOD(proto, isObscuredBy);
    ADD_METHOD(proto, mapFromItem);
    ADD_METHOD(proto, mapFromParent);
    ADD_METHOD(proto, mapFromScene);
    ADD_METHOD(proto, mapToItem);
    ADD_METHOD(proto, mapToParent);
    ADD_METHOD(proto, mapToScene);
    ADD_METHOD(proto, moveBy);
    ADD_METHOD(proto, paint);
    ADD_METHOD(proto, parentItem);
    ADD_METHOD(proto, removeSceneEventFilter);
    ADD_METHOD(proto, resetTransform);
    ADD_METHOD(proto, rotate);
    ADD_METHOD(proto, scale);
    ADD_METHOD(proto, setAcceptedMouseButtons);
    ADD_METHOD(proto, setData);
    ADD_METHOD(proto, setFlag);
    ADD_METHOD(proto, setFlags);
    ADD_METHOD(proto, setFocus);
    ADD_METHOD(proto, setParentItem);
    ADD_METHOD(proto, setPos);
    ADD_METHOD(proto, shear);
    ADD_METHOD(proto, show);
    ADD_METHOD(proto, toString);
    ADD_METHOD(proto, translate);
#ifndef QT_NO_CURSOR
    ADD_METHOD(proto, unsetCursor);
#endif
    ADD_METHOD(proto, update);

    QScript::registerPointerMetaType<QGraphicsItem>(eng, proto);

    QScriptValue ctorFun = eng->newFunction(ctor, proto);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemIsMovable);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemIsSelectable);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemIsFocusable);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemClipsToShape);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemClipsChildrenToShape);
    ADD_ENUM_VALUE(ctorFun, QGraphicsItem, ItemIgnoresTransformations);

    return ctorFun;
}
