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
#include <KUrl>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(KUrl*)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 1)
    {
        QString url = ctx->argument(0).toString();
        return qScriptValueFromValue(eng, KUrl(url));
    }

    return qScriptValueFromValue(eng, KUrl());
}

static QScriptValue toString(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(KUrl, toString);
    return QScriptValue(eng, self->prettyUrl());
}

QScriptValue constructKUrlClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, KUrl());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;

    proto.setProperty("toString", eng->newFunction(toString), getter);

    eng->setDefaultPrototype(qMetaTypeId<KUrl>(), proto);

    return eng->newFunction(ctor, proto);
}
