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
#include <QtCore/QRectF>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QRectF*)
Q_DECLARE_METATYPE(QRectF)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 4)
    {
        qreal x = ctx->argument(0).toNumber();
        qreal y = ctx->argument(1).toNumber();
        qreal width = ctx->argument(2).toNumber();
        qreal height = ctx->argument(3).toNumber();
        return qScriptValueFromValue(eng, QRectF(x, y, width, height));
    }

    return qScriptValueFromValue(eng, QRectF());
}

static QScriptValue adjust(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, adjust);
    qreal dx1 = ctx->argument(0).toNumber();
    qreal dy1 = ctx->argument(1).toNumber();
    qreal dx2 = ctx->argument(2).toNumber();
    qreal dy2 = ctx->argument(3).toNumber();

    self->adjust(dx1, dy1, dx2, dy2);
    return QScriptValue();
}

static QScriptValue adjusted(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, adjusted);
    qreal dx1 = ctx->argument(0).toNumber();
    qreal dy1 = ctx->argument(1).toNumber();
    qreal dx2 = ctx->argument(2).toNumber();
    qreal dy2 = ctx->argument(3).toNumber();

    QRectF tmp = self->adjusted(dx1, dy1, dx2, dy2);
    return qScriptValueFromValue(eng, tmp);
}

static QScriptValue bottom(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, bottom);
    return QScriptValue(eng, self->bottom());
}

static QScriptValue top(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, top);
    return QScriptValue(eng, self->top());
}

static QScriptValue contains(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, contains);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    return QScriptValue(eng, self->contains(x, y));
}

static QScriptValue height(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, height);
    return QScriptValue(eng, self->height());
}

static QScriptValue isEmpty(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, isEmpty);
    return QScriptValue(eng, self->isEmpty());
}

static QScriptValue isNull(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, isNull);
    return QScriptValue(eng, self->isNull());
}

static QScriptValue isValid(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, isValid);
    return QScriptValue(eng, self->isValid());
}

static QScriptValue left(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, left);
    return QScriptValue(eng, self->left());
}

static QScriptValue moveBottom(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveBottom);
    qreal bottom = ctx->argument(0).toNumber();
    self->moveBottom(bottom);
    return QScriptValue();
}

static QScriptValue moveLeft(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveLeft);
    qreal left = ctx->argument(0).toNumber();
    self->moveBottom(left);
    return QScriptValue();
}

static QScriptValue moveRight(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveRight);
    qreal right = ctx->argument(0).toNumber();
    self->moveBottom(right);
    return QScriptValue();
}


static QScriptValue moveTo(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveTo);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    self->moveTo(x, y);
    return QScriptValue();
}

static QScriptValue moveTop(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveTop);
    qreal top = ctx->argument(0).toNumber();
    self->moveTop(top);
    return QScriptValue();
}

static QScriptValue right(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, right);
    return QScriptValue(eng, self->right());
}

static QScriptValue setBottom(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setBottom);
    qreal bottom = ctx->argument(0).toNumber();
    self->setBottom(bottom);
    return QScriptValue();
}

static QScriptValue setCoords(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setCoords);
    qreal x1 = ctx->argument(0).toNumber();
    qreal y1 = ctx->argument(1).toNumber();
    qreal x2 = ctx->argument(2).toNumber();
    qreal y2 = ctx->argument(3).toNumber();
    self->setCoords(x1, y1, x2, y2);
    return QScriptValue();
}

static QScriptValue setHeight(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setHeight);
    qreal height = ctx->argument(0).toNumber();
    self->setHeight(height);
    return QScriptValue();
}

static QScriptValue setLeft(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setLeft);
    qreal left = ctx->argument(0).toNumber();
    self->setLeft(left);
    return QScriptValue();
}

static QScriptValue setRect(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setRect);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    qreal width = ctx->argument(2).toNumber();
    qreal height = ctx->argument(3).toNumber();
    self->setRect(x, y, width, height);
    return QScriptValue();
}

static QScriptValue setRight(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setRight);
    qreal right = ctx->argument(0).toNumber();
    self->setRight(right);
    return QScriptValue();
}

static QScriptValue setTop(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setTop);
    qreal top = ctx->argument(0).toNumber();
    self->setTop(top);
    return QScriptValue();
}

static QScriptValue setWidth(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setWidth);
    qreal width = ctx->argument(0).toNumber();
    self->setWidth(width);
    return QScriptValue();
}

static QScriptValue setX(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setX);
    qreal x = ctx->argument(0).toNumber();
    self->setX(x);
    return QScriptValue();
}

static QScriptValue setY(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setY);
    qreal y = ctx->argument(0).toNumber();
    self->setY(y);
    return QScriptValue();
}

static QScriptValue translate(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, translate);
    qreal dx = ctx->argument(0).toNumber();
    qreal dy = ctx->argument(1).toNumber();
    self->translate(dx, dy);
    return QScriptValue();
}

static QScriptValue width(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, width);
    return QScriptValue(eng, self->width());
}

static QScriptValue x(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, x);
    return QScriptValue(eng, self->x());
}

static QScriptValue y(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, y);
    return QScriptValue(eng, self->y());
}

/* Not Implemented Yet */
// QPointF bottomLeft () const
// QPointF bottomRight () const
// QPointF center () const
// bool contains ( const QPointF & point ) const
// bool contains ( const QRectF & rectangle ) const
// void getCoords ( qreal * x1, qreal * y1, qreal * x2, qreal * y2 ) const
// void getRect ( qreal * x, qreal * y, qreal * width, qreal * height ) const
// QRectF intersected ( const QRectF & rectangle ) const
// bool intersects ( const QRectF & rectangle ) const
// void moveBottomLeft ( const QPointF & position )
// void moveBottomRight ( const QPointF & position )
// void moveCenter ( const QPointF & position )
// void moveTo ( const QPointF & position )
// void moveTopLeft ( const QPointF & position )
// void moveTopRight ( const QPointF & position )
// QRectF normalized () const
// void setBottomLeft ( const QPointF & position )
// void setBottomRight ( const QPointF & position )
// void setSize ( const QSizeF & size )
// void setTopLeft ( const QPointF & position )
// void setTopRight ( const QPointF & position )
// QSizeF size () const
// QRect toAlignedRect () const
// QRect toRect () const
// QPointF topLeft () const
// QPointF topRight () const
// void translate ( const QPointF & offset )
// QRectF translated ( qreal dx, qreal dy ) const
// QRectF translated ( const QPointF & offset ) const
// QRectF united ( const QRectF & rectangle ) const

QScriptValue constructQRectFClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, QRectF());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;

    proto.setProperty("adjust", eng->newFunction(adjust));
    proto.setProperty("bottom", eng->newFunction(bottom));
    proto.setProperty("contains", eng->newFunction(contains));
    proto.setProperty("height", eng->newFunction(height));
    proto.setProperty("isEmpty", eng->newFunction(isEmpty));
    proto.setProperty("isNull", eng->newFunction(isNull));
    proto.setProperty("isValid", eng->newFunction(isValid));
    proto.setProperty("left", eng->newFunction(left));
    proto.setProperty("moveBottom", eng->newFunction(moveBottom));
    proto.setProperty("moveLeft", eng->newFunction(moveLeft));
    proto.setProperty("moveRight", eng->newFunction(moveRight));
    proto.setProperty("moveTo", eng->newFunction(moveTo));
    proto.setProperty("moveTop", eng->newFunction(moveTop));
    proto.setProperty("right", eng->newFunction(right));
    proto.setProperty("setBottom", eng->newFunction(setBottom));
    proto.setProperty("setCoords", eng->newFunction(setCoords));
    proto.setProperty("setHeight", eng->newFunction(setHeight));
    proto.setProperty("setLeft", eng->newFunction(setLeft));
    proto.setProperty("setRect", eng->newFunction(setRect));
    proto.setProperty("setRight", eng->newFunction(setRight));
    proto.setProperty("setTop", eng->newFunction(setTop));
    proto.setProperty("setWidth", eng->newFunction(setWidth));
    proto.setProperty("setX", eng->newFunction(setX));
    proto.setProperty("setY", eng->newFunction(setY));
    proto.setProperty("top", eng->newFunction(top));
    proto.setProperty("translate", eng->newFunction(translate));
    proto.setProperty("width", eng->newFunction(width));
    proto.setProperty("x", eng->newFunction(x));
    proto.setProperty("y", eng->newFunction(y));

    eng->setDefaultPrototype(qMetaTypeId<QRectF>(), proto);
    eng->setDefaultPrototype(qMetaTypeId<QRectF*>(), proto);

    return eng->newFunction(ctor, proto);
}
