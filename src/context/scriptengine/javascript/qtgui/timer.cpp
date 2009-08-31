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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptable>
#include <QtCore/QTimer>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QTimer*)

static QScriptValue newTimer(QScriptEngine *eng, QTimer *timer)
{
    return eng->newQObject(timer, QScriptEngine::AutoOwnership);
}

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    return newTimer(eng, new QTimer(qscriptvalue_cast<QObject*>(ctx->argument(0))));
}

static QScriptValue toString(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QTimer, toString);
    return QScriptValue(eng, QString::fromLatin1("QTimer(interval=%0)")
                        .arg(self->interval()));
}

QScriptValue constructTimerClass(QScriptEngine *eng)
{
    QScriptValue proto = newTimer(eng, new QTimer());
    ADD_METHOD(proto, toString);
    eng->setDefaultPrototype(qMetaTypeId<QTimer*>(), proto);

    return eng->newFunction(ctor, proto);
}
