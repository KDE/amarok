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
#include <QtGui/QFont>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QFont*)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 0)
        return qScriptValueFromValue(eng, QFont());
    QString family = ctx->argument(0).toString();
    if (ctx->argumentCount() == 1) {
        QFont *other = qscriptvalue_cast<QFont*>(ctx->argument(0));
        if (other)
            return qScriptValueFromValue(eng, QFont(*other));
        return qScriptValueFromValue(eng, QFont(family));
    }
    int pointSize = ctx->argument(1).toInt32();
    if (ctx->argumentCount() == 2)
        return qScriptValueFromValue(eng, QFont(family, pointSize));
    int weight = ctx->argument(2).toInt32();
    if (ctx->argumentCount() == 3)
        return qScriptValueFromValue(eng, QFont(family, pointSize, weight));
    bool italic = ctx->argument(3).toBoolean();
    return qScriptValueFromValue(eng, QFont(family, pointSize, weight, italic));
}

static QScriptValue bold(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, bold);
    return QScriptValue(eng, self->bold());
}

static QScriptValue defaultFamily(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, defaultFamily);
    return QScriptValue(eng, self->defaultFamily());
}

static QScriptValue exactMatch(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, exactMatch);
    return QScriptValue(eng, self->exactMatch());
}

static QScriptValue family(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, family);
    return QScriptValue(eng, self->family());
}

static QScriptValue fixedPitch(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, fixedPitch);
    return QScriptValue(eng, self->fixedPitch());
}

static QScriptValue fromString(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, fromString);
    return QScriptValue(eng, self->fromString(ctx->argument(0).toString()));
}

static QScriptValue handle(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.prototype.handle is not implemented");
}

static QScriptValue isCopyOf(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, isCopyOf);
    QFont *other = qscriptvalue_cast<QFont*>(ctx->argument(0));
    if (!other) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QFont.prototype.isCopyOf: argument is not a Font");
    }
    return QScriptValue(eng, self->isCopyOf(*other));
}

static QScriptValue italic(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, italic);
    return QScriptValue(eng, self->italic());
}

static QScriptValue kerning(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, kerning);
    return QScriptValue(eng, self->kerning());
}

static QScriptValue key(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, key);
    return QScriptValue(eng, self->key());
}

static QScriptValue lastResortFamily(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, lastResortFamily);
    return QScriptValue(eng, self->lastResortFamily());
}

static QScriptValue lastResortFont(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, lastResortFont);
    return QScriptValue(eng, self->lastResortFont());
}

static QScriptValue overline(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, overline);
    return QScriptValue(eng, self->overline());
}

static QScriptValue pixelSize(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, pixelSize);
    return QScriptValue(eng, self->pixelSize());
}

static QScriptValue pointSize(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, pointSize);
    return QScriptValue(eng, self->pointSize());
}

static QScriptValue pointSizeF(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, pointSizeF);
    return QScriptValue(eng, self->pointSizeF());
}

static QScriptValue rawMode(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, rawMode);
    return QScriptValue(eng, self->rawMode());
}

static QScriptValue rawName(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, rawName);
    return QScriptValue(eng, self->rawName());
}

static QScriptValue resolve(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, resolve);
    QFont *other = qscriptvalue_cast<QFont*>(ctx->argument(0));
    if (!other) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QFont.prototype.isCopyOf: argument is not a Font");
    }
    return qScriptValueFromValue(eng, self->resolve(*other));
}

static QScriptValue setBold(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setBold);
    QScriptValue arg = ctx->argument(0);
    self->setBold(arg.toBoolean());
    return arg;
}

static QScriptValue setFamily(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setFamily);
    QScriptValue arg = ctx->argument(0);
    self->setFamily(arg.toString());
    return arg;
}

static QScriptValue setFixedPitch(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setFixedPitch);
    QScriptValue arg = ctx->argument(0);
    self->setFixedPitch(arg.toBoolean());
    return arg;
}

static QScriptValue setItalic(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setItalic);
    QScriptValue arg = ctx->argument(0);
    self->setItalic(arg.toBoolean());
    return arg;
}

static QScriptValue setKerning(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setKerning);
    QScriptValue arg = ctx->argument(0);
    self->setKerning(arg.toBoolean());
    return arg;
}

static QScriptValue setOverline(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setOverline);
    QScriptValue arg = ctx->argument(0);
    self->setOverline(arg.toBoolean());
    return arg;
}

static QScriptValue setPixelSize(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setPixelSize);
    QScriptValue arg = ctx->argument(0);
    self->setPixelSize(arg.toInt32());
    return arg;
}

static QScriptValue setPointSize(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setPointSize);
    QScriptValue arg = ctx->argument(0);
    self->setPointSize(arg.toInt32());
    return arg;
}

static QScriptValue setPointSizeF(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setPointSizeF);
    QScriptValue arg = ctx->argument(0);
    self->setPointSizeF(arg.toNumber());
    return arg;
}

static QScriptValue setRawMode(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setRawMode);
    QScriptValue arg = ctx->argument(0);
    self->setRawMode(arg.toBoolean());
    return arg;
}

static QScriptValue setRawName(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setRawName);
    QScriptValue arg = ctx->argument(0);
    self->setRawName(arg.toString());
    return arg;
}

static QScriptValue setStretch(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setStretch);
    QScriptValue arg = ctx->argument(0);
    self->setStretch(arg.toInt32());
    return arg;
}

static QScriptValue setStrikeOut(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setStrikeOut);
    QScriptValue arg = ctx->argument(0);
    self->setStrikeOut(arg.toBoolean());
    return arg;
}

static QScriptValue setStyle(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.setStyle");
}

static QScriptValue setStyleHint(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.setStyleHint");
}

static QScriptValue setStyleStrategy(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.setStyleStrategy");
}

static QScriptValue setUnderline(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setUnderline);
    QScriptValue arg = ctx->argument(0);
    self->setUnderline(arg.toBoolean());
    return arg;
}

static QScriptValue setWeight(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QFont, setWeight);
    QScriptValue arg = ctx->argument(0);
    self->setWeight(arg.toInt32());
    return arg;
}

static QScriptValue stretch(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, stretch);
    return QScriptValue(eng, self->stretch());
}

static QScriptValue strikeOut(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, strikeOut);
    return QScriptValue(eng, self->strikeOut());
}

static QScriptValue style(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.prototype.style is not implemented");
}

static QScriptValue styleHint(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.prototype.styleHint is not implemented");
}

static QScriptValue styleStrategy(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->throwError("QFont.prototype.styleStrategy is not implemented");
}

static QScriptValue toString(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, toString);
    return QScriptValue(eng, self->toString());
}

static QScriptValue underline(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, underline);
    return QScriptValue(eng, self->underline());
}

static QScriptValue weight(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QFont, weight);
    return QScriptValue(eng, self->weight());
}

QScriptValue constructFontClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, QFont());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;
    proto.setProperty("bold", eng->newFunction(bold), getter);
    proto.setProperty("defaultFamily", eng->newFunction(defaultFamily));
    proto.setProperty("exactMatch", eng->newFunction(exactMatch));
    proto.setProperty("family", eng->newFunction(family), getter);
    proto.setProperty("fixedPitch", eng->newFunction(fixedPitch), getter);
    proto.setProperty("fromString", eng->newFunction(fromString));
    proto.setProperty("handle", eng->newFunction(handle));
    proto.setProperty("isCopyOf", eng->newFunction(isCopyOf));
    proto.setProperty("italic", eng->newFunction(italic), getter);
    proto.setProperty("kerning", eng->newFunction(kerning), getter);
    proto.setProperty("key", eng->newFunction(key), getter);
    proto.setProperty("lastResortFamily", eng->newFunction(lastResortFamily));
    proto.setProperty("lastResortFont", eng->newFunction(lastResortFont));
    proto.setProperty("overline", eng->newFunction(overline), getter);
    proto.setProperty("pixelSize", eng->newFunction(pixelSize), getter);
    proto.setProperty("pointSize", eng->newFunction(pointSize), getter);
    proto.setProperty("pointSizeF", eng->newFunction(pointSizeF), getter);
    proto.setProperty("rawMode", eng->newFunction(rawMode), getter);
    proto.setProperty("rawName", eng->newFunction(rawName), getter);
    proto.setProperty("resolve", eng->newFunction(resolve));
    proto.setProperty("bold", eng->newFunction(setBold), setter);
    proto.setProperty("bamily", eng->newFunction(setFamily), setter);
    proto.setProperty("fixedPitch", eng->newFunction(setFixedPitch), setter);
    proto.setProperty("italic", eng->newFunction(setItalic), setter);
    proto.setProperty("kerning", eng->newFunction(setKerning), setter);
    proto.setProperty("overline", eng->newFunction(setOverline), setter);
    proto.setProperty("pixelSize", eng->newFunction(setPixelSize), setter);
    proto.setProperty("pointSize", eng->newFunction(setPointSize), setter);
    proto.setProperty("pointSizeF", eng->newFunction(setPointSizeF), setter);
    proto.setProperty("rawMode", eng->newFunction(setRawMode), setter);
    proto.setProperty("rawName", eng->newFunction(setRawName), setter);
    proto.setProperty("stretch", eng->newFunction(setStretch), setter);
    proto.setProperty("strikeOut", eng->newFunction(setStrikeOut), setter);
    proto.setProperty("setStyle", eng->newFunction(setStyle));
    proto.setProperty("setStyleHint", eng->newFunction(setStyleHint));
    proto.setProperty("setStyleStrategy", eng->newFunction(setStyleStrategy));
    proto.setProperty("underline", eng->newFunction(setUnderline), setter);
    proto.setProperty("weight", eng->newFunction(setWeight), setter);
    proto.setProperty("stretch", eng->newFunction(stretch), getter);
    proto.setProperty("strikeOut", eng->newFunction(strikeOut), getter);
    proto.setProperty("style", eng->newFunction(style));
    proto.setProperty("styleHint", eng->newFunction(styleHint));
    proto.setProperty("styleStrategy", eng->newFunction(styleStrategy));
    proto.setProperty("toString", eng->newFunction(toString));
    proto.setProperty("underline", eng->newFunction(underline), getter);
    proto.setProperty("weight", eng->newFunction(weight), getter);

    eng->setDefaultPrototype(qMetaTypeId<QFont>(), proto);
    eng->setDefaultPrototype(qMetaTypeId<QFont*>(), proto);

    return eng->newFunction(ctor, proto);
}
