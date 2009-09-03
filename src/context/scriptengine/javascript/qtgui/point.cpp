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
#include <QtCore/QPoint>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QPoint*)
Q_DECLARE_METATYPE(QPoint)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 2)
    {
        int x = ctx->argument(0).toInt32();
        int y = ctx->argument(1).toInt32();
        return qScriptValueFromValue(eng, QPoint(x, y));
    }

    return qScriptValueFromValue(eng, QPoint());
}

static QScriptValue isNull(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPoint, isNull);
    return QScriptValue(eng, self->isNull());
}

static QScriptValue manhattanLength(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPoint, manhattanLength);
    return QScriptValue(eng, self->manhattanLength());
}

static QScriptValue x(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPoint, x);
    return QScriptValue(eng, self->x());
}

static QScriptValue y(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPoint, y);
    return QScriptValue(eng, self->y());
}

static QScriptValue setX(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QPoint, setX);
    int x = ctx->argument(0).toInt32();
    self->setX(x);
    return QScriptValue();
}

static QScriptValue setY(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QPoint, setY);
    int y = ctx->argument(0).toInt32();
    self->setY(y);
    return QScriptValue();
}

QScriptValue constructQPointClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, QPoint());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;

    proto.setProperty("isNull", eng->newFunction(isNull));
    proto.setProperty("manhattanLength", eng->newFunction(manhattanLength));
    proto.setProperty("x", eng->newFunction(x));
    proto.setProperty("y", eng->newFunction(y));
    proto.setProperty("setX", eng->newFunction(setX));
    proto.setProperty("setY", eng->newFunction(setY));

    eng->setDefaultPrototype(qMetaTypeId<QPoint>(), proto);
    eng->setDefaultPrototype(qMetaTypeId<QPoint*>(), proto);

    return eng->newFunction(ctor, proto);
}
