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

#include <QtGui/QPainter>
#include <QtGui/QPicture>
#include <QtGui/QPolygonF>
#include <QtGui/QWidget>

#include <QtCore/qdebug.h>

#include "../backportglobal.h"

Q_DECLARE_METATYPE(QPolygonF)
Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(QPainterPath*)
Q_DECLARE_METATYPE(QPicture)
Q_DECLARE_METATYPE(QVector<QRectF>)
Q_DECLARE_METATYPE(QPaintDevice*)
Q_DECLARE_METATYPE(QPaintEngine*)

DECLARE_POINTER_METATYPE(QPainter)

static QScriptValue newPainter(QScriptEngine *eng, QPainter *p)
{
    return QScript::wrapPointer(eng, p);
}

/////////////////////////////////////////////////////////////

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() > 0) {
        QPaintDevice *device = qscriptvalue_cast<QPaintDevice*>(ctx->argument(0));
        return newPainter(eng, new QPainter(device));
    } else {
        return newPainter(eng, new QPainter());
    }
}

/////////////////////////////////////////////////////////////

static QScriptValue background(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, background);
    return qScriptValueFromValue(eng, self->background());
}

/////////////////////////////////////////////////////////////

static QScriptValue backgroundMode(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, backgroundMode);
    return QScriptValue(eng, static_cast<int>(self->backgroundMode()));
}

/////////////////////////////////////////////////////////////

static QScriptValue begin(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, begin);
    QWidget *device = qscriptvalue_cast<QWidget*>(ctx->argument(0));
    if (!device) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QPainter.prototype.begin: argument is not a QWidget");
    }
    return QScriptValue(eng, self->begin(device));
}

/////////////////////////////////////////////////////////////

static QScriptValue boundingRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, boundingRect);
    QRect result;
    if (ctx->argumentCount() == 3) {
        result = self->boundingRect(qscriptvalue_cast<QRect>(ctx->argument(0)),
                                    ctx->argument(1).toInt32(),
                                    ctx->argument(2).toString());
    } else if (ctx->argumentCount() == 6) {
        result = self->boundingRect(ctx->argument(0).toInt32(),
                                    ctx->argument(1).toInt32(),
                                    ctx->argument(2).toInt32(),
                                    ctx->argument(3).toInt32(),
                                    ctx->argument(4).toInt32(),
                                    ctx->argument(5).toString());
    }
    return qScriptValueFromValue(eng, result);
}

/////////////////////////////////////////////////////////////

static QScriptValue brush(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, brush);
    return qScriptValueFromValue(eng, self->brush());
}

/////////////////////////////////////////////////////////////

static QScriptValue brushOrigin(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, brushOrigin);
    return qScriptValueFromValue(eng, self->brushOrigin());
}

/////////////////////////////////////////////////////////////

static QScriptValue clipPath(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, clipPath);
    return qScriptValueFromValue(eng, self->clipPath());
}

/////////////////////////////////////////////////////////////

static QScriptValue clipRegion(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, clipRegion);
    return qScriptValueFromValue(eng, self->clipRegion());
}

/////////////////////////////////////////////////////////////

static QScriptValue combinedMatrix(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, combinedMatrix);
    return qScriptValueFromValue(eng, self->combinedMatrix());
}

/////////////////////////////////////////////////////////////

static QScriptValue combinedTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, combinedTransform);
    return qScriptValueFromValue(eng, self->combinedTransform());
}

/////////////////////////////////////////////////////////////

static QScriptValue compositionMode(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, compositionMode);
    return QScriptValue(eng, static_cast<int>(self->compositionMode()));
}

/////////////////////////////////////////////////////////////

static QScriptValue device(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, device);
    return qScriptValueFromValue(eng, self->device());
}

/////////////////////////////////////////////////////////////

static QScriptValue deviceMatrix(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, deviceMatrix);
    return qScriptValueFromValue(eng, self->deviceMatrix());
}

/////////////////////////////////////////////////////////////

static QScriptValue deviceTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, deviceTransform);
    return qScriptValueFromValue(eng, self->deviceTransform());
}

/////////////////////////////////////////////////////////////

static QScriptValue drawArc(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawArc);
    if (ctx->argumentCount() == 6) {
        // drawArc(x, y, height, width, startAngle, spanAngle)
        self->drawArc(ctx->argument(0).toInt32(),
                      ctx->argument(1).toInt32(),
                      ctx->argument(2).toInt32(),
                      ctx->argument(3).toInt32(),
                      ctx->argument(4).toInt32(),
                      ctx->argument(5).toInt32());
    } else if (ctx->argumentCount() == 3) {
        // drawArc(rectangle, startAngle, spanAngle)
        self->drawArc(qscriptvalue_cast<QRectF>(ctx->argument(0)),
                      ctx->argument(1).toInt32(),
                      ctx->argument(2).toInt32());
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawChord(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawChord);
    if (ctx->argumentCount() == 6) {
        // x, y, height, width, startAngle, spanAngle
        self->drawChord(ctx->argument(0).toInt32(),
                        ctx->argument(1).toInt32(),
                        ctx->argument(2).toInt32(),
                        ctx->argument(3).toInt32(),
                        ctx->argument(4).toInt32(),
                        ctx->argument(5).toInt32());
    } else if (ctx->argumentCount() == 3) {
        // rectangle, startAngle, spanAngle
        self->drawChord(qscriptvalue_cast<QRectF>(ctx->argument(0)),
                        ctx->argument(1).toInt32(),
                        ctx->argument(2).toInt32());
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawConvexPolygon(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawConvexPolygon);
    self->drawConvexPolygon(qscriptvalue_cast<QPolygonF>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawEllipse(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawEllipse);
    if (ctx->argumentCount() == 4) {
        // drawEllipse(x, y, width, height)
        self->drawEllipse(ctx->argument(0).toInt32(),
                          ctx->argument(1).toInt32(),
                          ctx->argument(2).toInt32(),
                          ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // drawEllipse(rect)
        self->drawEllipse(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawImage(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawImage);
    if (ctx->argumentCount() == 2) {
        // target, image
        QScriptValue arg0 = ctx->argument(0);
        QImage image = qscriptvalue_cast<QImage>(ctx->argument(1));
        if (arg0.property("width").isValid()) {
            self->drawImage(qscriptvalue_cast<QRectF>(arg0), image);
        } else {
            self->drawImage(qscriptvalue_cast<QPointF>(arg0), image);
        }
    } else if (ctx->argumentCount() == 3) {
        // x, y, image
        self->drawImage(ctx->argument(0).toInt32(),
                        ctx->argument(1).toInt32(),
                        qscriptvalue_cast<QImage>(ctx->argument(2)));
    } else if (ctx->argumentCount() == 5) {
        // x, y, width, height, image
        self->drawImage(QRect(ctx->argument(0).toInt32(),
                              ctx->argument(1).toInt32(),
                              ctx->argument(2).toInt32(),
                              ctx->argument(3).toInt32()),
                        qscriptvalue_cast<QImage>(ctx->argument(4)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawLine(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawLine);
    if (ctx->argumentCount() == 4) {
        // x1, y1, x2, y2
        self->drawLine(ctx->argument(0).toInt32(),
                       ctx->argument(1).toInt32(),
                       ctx->argument(2).toInt32(),
                       ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 2) {
        // p1, p2
        self->drawLine(qscriptvalue_cast<QPointF>(ctx->argument(0)),
                       qscriptvalue_cast<QPointF>(ctx->argument(1)));
    } else if (ctx->argumentCount() == 1) {
        // line
        self->drawLine(qscriptvalue_cast<QLineF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawLines(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawLines);
    return ctx->throwError("QPainter.prototype.drawLines is not implemented");
//    self->drawLines(qscriptvalue_cast<QVector<QLineF> >(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPath(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPath);
    self->drawPath(qscriptvalue_cast<QPainterPath>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPicture(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPicture);
    if (ctx->argumentCount() == 2) {
        self->drawPicture(qscriptvalue_cast<QPointF>(ctx->argument(0)),
                          qscriptvalue_cast<QPicture>(ctx->argument(1)));
    } else if (ctx->argumentCount() == 3) {
        self->drawPicture(ctx->argument(0).toInt32(),
                          ctx->argument(1).toInt32(),
                          qscriptvalue_cast<QPicture>(ctx->argument(2)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPie(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPie);
    if (ctx->argumentCount() == 6) {
        // x, y, height, width, startAngle, spanAngle
        self->drawPie(ctx->argument(0).toInt32(),
                      ctx->argument(1).toInt32(),
                      ctx->argument(2).toInt32(),
                      ctx->argument(3).toInt32(),
                      ctx->argument(4).toInt32(),
                      ctx->argument(5).toInt32());
    } else if (ctx->argumentCount() == 3) {
        // rectangle, startAngle, spanAngle
        self->drawPie(qscriptvalue_cast<QRectF>(ctx->argument(0)),
                      ctx->argument(1).toInt32(),
                      ctx->argument(2).toInt32());
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPixmap(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPixmap);
    if (ctx->argumentCount() == 2) {
        // target, pixmap
        QScriptValue arg0 = ctx->argument(0);
        QPixmap pixmap = qscriptvalue_cast<QPixmap>(ctx->argument(1));
        if (arg0.property("width").isValid()) {
            self->drawPixmap(qscriptvalue_cast<QRectF>(arg0), pixmap,
                             QRectF(0, 0, pixmap.width(), pixmap.height()));
        } else {
            self->drawPixmap(qscriptvalue_cast<QPointF>(arg0), pixmap);
        }
    } else if (ctx->argumentCount() == 3) {
        // x, y, pixmap
        self->drawPixmap(ctx->argument(0).toInt32(),
                         ctx->argument(1).toInt32(),
                         qscriptvalue_cast<QPixmap>(ctx->argument(2)));
    } else if (ctx->argumentCount() == 5) {
        // x, y, width, height, pixmap
        self->drawPixmap(ctx->argument(0).toInt32(),
                         ctx->argument(1).toInt32(),
                         ctx->argument(2).toInt32(),
                         ctx->argument(3).toInt32(),
                         qscriptvalue_cast<QPixmap>(ctx->argument(4)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPoint(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPoint);
    if (ctx->argumentCount() == 2) {
        // drawPoint(x, y)
        self->drawPoint(ctx->argument(0).toInt32(),
                        ctx->argument(1).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // drawPoint(point)
        self->drawPoint(qscriptvalue_cast<QPointF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPoints(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPoints);
    self->drawPoints(qscriptvalue_cast<QPolygonF>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPolygon(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPolygon);
    // ### fillRule (2nd argument)
    self->drawPolygon(qscriptvalue_cast<QPolygonF>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawPolyline(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawPolyline);
    self->drawPolyline(qscriptvalue_cast<QPolygonF>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawRect);
    if (ctx->argumentCount() == 4) {
        // x, y, width, height
        self->drawRect(ctx->argument(0).toInt32(),
                       ctx->argument(1).toInt32(),
                       ctx->argument(2).toInt32(),
                       ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // rect
        self->drawRect(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawRects(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawRects);
    self->drawRects(qscriptvalue_cast<QVector<QRectF> >(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawRoundRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawRoundRect);
    // ### xRnd, yRnd
    if (ctx->argumentCount() >= 4) {
        self->drawRoundRect(ctx->argument(0).toInt32(),
                            ctx->argument(1).toInt32(),
                            ctx->argument(2).toInt32(),
                            ctx->argument(3).toInt32());
    } else {
        self->drawRoundRect(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawText(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawText);
    if (ctx->argumentCount() == 3) {
        // x, y, text
        self->drawText(ctx->argument(0).toInt32(),
                       ctx->argument(1).toInt32(),
                       ctx->argument(2).toString());
    } else if (ctx->argumentCount() == 2) {
        QScriptValue arg0 = ctx->argument(0);
        if (arg0.property("width").isValid()) {
            self->drawText(qscriptvalue_cast<QRectF>(arg0),
                           ctx->argument(1).toString());
        } else {
            self->drawText(qscriptvalue_cast<QPointF>(arg0),
                           ctx->argument(1).toString());
        }
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue drawTiledPixmap(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, drawTiledPixmap);
    if (ctx->argumentCount() >= 5) {
        // x, y, width, height, pixmap, sx, sy
        self->drawTiledPixmap(ctx->argument(0).toInt32(),
                              ctx->argument(1).toInt32(),
                              ctx->argument(2).toInt32(),
                              ctx->argument(3).toInt32(),
                              qscriptvalue_cast<QPixmap>(ctx->argument(4)),
                              ctx->argument(5).toInt32(),
                              ctx->argument(6).toInt32());
    } else {
        // rect, pixmap, position
        self->drawTiledPixmap(qscriptvalue_cast<QRectF>(ctx->argument(0)),
                              qscriptvalue_cast<QPixmap>(ctx->argument(1)),
                              qscriptvalue_cast<QPointF>(ctx->argument(2)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue end(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, end);
    return QScriptValue(eng, self->end());
}

/////////////////////////////////////////////////////////////

static QScriptValue eraseRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, eraseRect);
    if (ctx->argumentCount() == 4) {
        // x, y, width, height
        self->eraseRect(ctx->argument(0).toInt32(),
                        ctx->argument(1).toInt32(),
                        ctx->argument(2).toInt32(),
                        ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // rect
        self->eraseRect(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue fillPath(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, fillPath);
    QPainterPath *path = qscriptvalue_cast<QPainterPath*>(ctx->argument(0));
    if (!path) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QPainter.prototype.fillPath: argument is not a PainterPath");
    }
    self->fillPath(*path, qscriptvalue_cast<QBrush>(ctx->argument(1)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue fillRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, fillRect);
    if (ctx->argumentCount() == 5) {
        // x, y, width, height, brush
        self->fillRect(ctx->argument(0).toInt32(),
                       ctx->argument(1).toInt32(),
                       ctx->argument(2).toInt32(),
                       ctx->argument(3).toInt32(),
                       qscriptvalue_cast<QBrush>(ctx->argument(4)));
    } else if (ctx->argumentCount() == 2) {
        // rect, brush
        self->fillRect(qscriptvalue_cast<QRectF>(ctx->argument(0)),
                       qscriptvalue_cast<QBrush>(ctx->argument(1)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue font(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, font);
    return qScriptValueFromValue(eng, self->font());
}

/////////////////////////////////////////////////////////////

static QScriptValue fontInfo(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QPainter, fontInfo);
    return ctx->throwError("QPainter.prototype.fontInfo is not implemented");
}

/////////////////////////////////////////////////////////////

static QScriptValue fontMetrics(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QPainter, fontMetrics);
    return ctx->throwError("QPainter.prototype.fontMetrics is not implemented");
}

/////////////////////////////////////////////////////////////

static QScriptValue hasClipping(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, hasClipping);
    return QScriptValue(eng, self->hasClipping());
}

/////////////////////////////////////////////////////////////

static QScriptValue initFrom(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, initFrom);
    QWidget *widget = qscriptvalue_cast<QWidget*>(ctx->argument(0));
    if (!widget) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QPainter.prototype.initFrom: argument is not a Widget");
    }
    self->initFrom(widget);
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue isActive(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, isActive);
    return QScriptValue(eng, self->isActive());
}

/////////////////////////////////////////////////////////////

static QScriptValue layoutDirection(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, layoutDirection);
    return QScriptValue(eng, static_cast<int>(self->layoutDirection()));
}

/////////////////////////////////////////////////////////////

static QScriptValue opacity(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, opacity);
    return QScriptValue(eng, self->opacity());
}

/////////////////////////////////////////////////////////////

static QScriptValue paintEngine(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, paintEngine);
    return qScriptValueFromValue(eng, self->paintEngine());
}

/////////////////////////////////////////////////////////////

static QScriptValue pen(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, pen);
    return qScriptValueFromValue(eng, self->pen());
}

/////////////////////////////////////////////////////////////

static QScriptValue renderHints(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, renderHints);
    return QScriptValue(eng, static_cast<int>(self->renderHints()));
}

/////////////////////////////////////////////////////////////

static QScriptValue resetMatrix(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, resetMatrix);
    self->resetMatrix();
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue resetTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, resetTransform);
    self->resetTransform();
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue restore(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, restore);
    self->restore();
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue rotate(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, rotate);
    self->rotate(ctx->argument(0).toNumber());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue save(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, save);
    self->save();
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue scale(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, scale);
    self->scale(ctx->argument(0).toNumber(),
                ctx->argument(1).toNumber());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setBackground(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setBackground);
    self->setBackground(qscriptvalue_cast<QBrush>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setBackgroundMode(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setBackgroundMode);
    self->setBackgroundMode(static_cast<Qt::BGMode>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setBrush(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setBrush);
    self->setBrush(qscriptvalue_cast<QBrush>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setBrushOrigin(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setBrushOrigin);
    self->setBrushOrigin(qscriptvalue_cast<QPointF>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setClipPath(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setClipPath);
    // ### ClipOperation
    self->setClipPath(qscriptvalue_cast<QPainterPath>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setClipRect(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setClipRect);
    // ### ClipOperation
    if (ctx->argumentCount() >= 4) {
        // x, y, width, height [, operation]
        self->setClipRect(ctx->argument(0).toInt32(),
                          ctx->argument(1).toInt32(),
                          ctx->argument(2).toInt32(),
                          ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() >= 1) {
        // rect [, operation]
        self->setClipRect(qscriptvalue_cast<QRectF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setClipRegion(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setClipRegion);
    // ### ClipOperation
    self->setClipRegion(qscriptvalue_cast<QRegion>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setClipping(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setClipping);
    self->setClipping(ctx->argument(0).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setCompositionMode(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setCompositionMode);
    self->setCompositionMode(static_cast<QPainter::CompositionMode>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setFont(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setFont);
    self->setFont(qscriptvalue_cast<QFont>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setLayoutDirection(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setLayoutDirection);
    self->setLayoutDirection(static_cast<Qt::LayoutDirection>(ctx->argument(0).toInt32()));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setOpacity(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setOpacity);
    self->setOpacity(ctx->argument(0).toNumber());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setPen(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setPen);
    self->setPen(qscriptvalue_cast<QPen>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setRenderHint(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setRenderHint);
    self->setRenderHint(static_cast<QPainter::RenderHint>(ctx->argument(0).toInt32()),
                        ctx->argument(1).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setRenderHints(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setRenderHints);
    self->setRenderHints(static_cast<QPainter::RenderHints>(ctx->argument(0).toInt32()),
                         ctx->argument(1).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setTransform);
    self->setTransform(qscriptvalue_cast<QTransform>(ctx->argument(0)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setViewTransformEnabled(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setViewTransformEnabled);
    self->setViewTransformEnabled(ctx->argument(0).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setViewport(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setViewport);
    if (ctx->argumentCount() == 4) {
        // x, y, width, height
        self->setViewport(ctx->argument(0).toInt32(),
                          ctx->argument(1).toInt32(),
                          ctx->argument(2).toInt32(),
                          ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // rect
        self->setViewport(qscriptvalue_cast<QRect>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setWindow(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setWindow);
    if (ctx->argumentCount() == 4) {
        // x, y, width, height
        self->setWindow(ctx->argument(0).toInt32(),
                        ctx->argument(1).toInt32(),
                        ctx->argument(2).toInt32(),
                        ctx->argument(3).toInt32());
    } else if (ctx->argumentCount() == 1) {
        // rect
        self->setWindow(qscriptvalue_cast<QRect>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setWorldMatrix(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setWorldMatrix);
    self->setWorldMatrix(qscriptvalue_cast<QMatrix>(ctx->argument(0)),
                         ctx->argument(1).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setWorldMatrixEnabled(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setWorldMatrixEnabled);
    self->setWorldMatrixEnabled(ctx->argument(0).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue setWorldTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, setWorldTransform);
    self->setWorldTransform(qscriptvalue_cast<QTransform>(ctx->argument(0)),
                            ctx->argument(1).toBoolean());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue shear(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, shear);
    self->shear(ctx->argument(0).toNumber(),
                     ctx->argument(1).toNumber());
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue strokePath(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, strokePath);
    QPainterPath *path = qscriptvalue_cast<QPainterPath*>(ctx->argument(0));
    if (!path) {
        return ctx->throwError(QScriptContext::TypeError,
                               "QPainter.prototype.strokePath: argument is not a PainterPath");
    }
    self->strokePath(*path, qscriptvalue_cast<QPen>(ctx->argument(1)));
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue testRenderHint(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, testRenderHint);
    return QScriptValue(eng, self->testRenderHint(static_cast<QPainter::RenderHint>(ctx->argument(0).toInt32())));
}

/////////////////////////////////////////////////////////////

static QScriptValue transform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, transform);
    return qScriptValueFromValue(eng, self->transform());
}

/////////////////////////////////////////////////////////////

static QScriptValue translate(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, translate);
    if (ctx->argumentCount() == 2) {
        // dx, dy
        self->translate(ctx->argument(0).toNumber(),
                             ctx->argument(1).toNumber());
    } else if (ctx->argumentCount() == 1) {
        // offset
        self->translate(qscriptvalue_cast<QPointF>(ctx->argument(0)));
    }
    return eng->undefinedValue();
}

/////////////////////////////////////////////////////////////

static QScriptValue viewTransformEnabled(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, viewTransformEnabled);
    return QScriptValue(eng, self->viewTransformEnabled());
}

/////////////////////////////////////////////////////////////

static QScriptValue viewport(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, viewport);
    return qScriptValueFromValue(eng, self->viewport());
}

/////////////////////////////////////////////////////////////

static QScriptValue window(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, window);
    return qScriptValueFromValue(eng, self->window());
}

/////////////////////////////////////////////////////////////

static QScriptValue worldMatrix(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, worldMatrix);
    return qScriptValueFromValue(eng, self->worldMatrix());
}

/////////////////////////////////////////////////////////////

static QScriptValue worldMatrixEnabled(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, worldMatrixEnabled);
    return QScriptValue(eng, self->worldMatrixEnabled());
}

/////////////////////////////////////////////////////////////

static QScriptValue worldTransform(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, worldTransform);
    return qScriptValueFromValue(eng, self->worldTransform());
}

/////////////////////////////////////////////////////////////

static QScriptValue toString(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QPainter, toString);
    return QScriptValue(eng, "QPainter");
}

/////////////////////////////////////////////////////////////

QScriptValue constructPainterClass(QScriptEngine *eng)
{
    QScriptValue proto = newPainter(eng, new QPainter());
    ADD_METHOD(proto, background);
    ADD_METHOD(proto, backgroundMode);
    ADD_METHOD(proto, begin);
    ADD_METHOD(proto, boundingRect);
    ADD_METHOD(proto, brush);
    ADD_METHOD(proto, brushOrigin);
    ADD_METHOD(proto, clipPath);
    ADD_METHOD(proto, clipRegion);
    ADD_METHOD(proto, combinedMatrix);
    ADD_METHOD(proto, combinedTransform);
    ADD_METHOD(proto, compositionMode);
    ADD_METHOD(proto, device);
    ADD_METHOD(proto, deviceMatrix);
    ADD_METHOD(proto, deviceTransform);
    ADD_METHOD(proto, drawChord);
    ADD_METHOD(proto, drawConvexPolygon);
    ADD_METHOD(proto, drawArc);
    ADD_METHOD(proto, drawEllipse);
    ADD_METHOD(proto, drawImage);
    ADD_METHOD(proto, drawLine);
    ADD_METHOD(proto, drawLines);
    ADD_METHOD(proto, drawPath);
    ADD_METHOD(proto, drawPicture);
    ADD_METHOD(proto, drawPie);
    ADD_METHOD(proto, drawPixmap);
    ADD_METHOD(proto, drawPoint);
    ADD_METHOD(proto, drawPoints);
    ADD_METHOD(proto, drawPolygon);
    ADD_METHOD(proto, drawPolyline);
    ADD_METHOD(proto, drawRect);
    ADD_METHOD(proto, drawRects);
    ADD_METHOD(proto, drawRoundRect);
    ADD_METHOD(proto, drawText);
    ADD_METHOD(proto, drawTiledPixmap);
    ADD_METHOD(proto, end);
    ADD_METHOD(proto, eraseRect);
    ADD_METHOD(proto, fillPath);
    ADD_METHOD(proto, fillRect);
    ADD_METHOD(proto, font);
    ADD_METHOD(proto, fontInfo);
    ADD_METHOD(proto, fontMetrics);
    ADD_METHOD(proto, hasClipping);
    ADD_METHOD(proto, initFrom);
    ADD_METHOD(proto, isActive);
    ADD_METHOD(proto, layoutDirection);
    ADD_METHOD(proto, opacity);
    ADD_METHOD(proto, paintEngine);
    ADD_METHOD(proto, pen);
    ADD_METHOD(proto, renderHints);
    ADD_METHOD(proto, resetMatrix);
    ADD_METHOD(proto, resetTransform);
    ADD_METHOD(proto, restore);
    ADD_METHOD(proto, rotate);
    ADD_METHOD(proto, save);
    ADD_METHOD(proto, scale);
    ADD_METHOD(proto, setBackground);
    ADD_METHOD(proto, setBackgroundMode);
    ADD_METHOD(proto, setBrush);
    ADD_METHOD(proto, setBrushOrigin);
    ADD_METHOD(proto, setClipPath);
    ADD_METHOD(proto, setClipRect);
    ADD_METHOD(proto, setClipRegion);
    ADD_METHOD(proto, setClipping);
    ADD_METHOD(proto, setCompositionMode);
    ADD_METHOD(proto, setFont);
    ADD_METHOD(proto, setLayoutDirection);
    ADD_METHOD(proto, setOpacity);
    ADD_METHOD(proto, setPen);
    ADD_METHOD(proto, setRenderHint);
    ADD_METHOD(proto, setRenderHints);
    ADD_METHOD(proto, setTransform);
    ADD_METHOD(proto, setViewTransformEnabled);
    ADD_METHOD(proto, setViewport);
    ADD_METHOD(proto, setWindow);
    ADD_METHOD(proto, setWorldMatrix);
    ADD_METHOD(proto, setWorldMatrixEnabled);
    ADD_METHOD(proto, setWorldTransform);
    ADD_METHOD(proto, shear);
    ADD_METHOD(proto, strokePath);
    ADD_METHOD(proto, testRenderHint);
    ADD_METHOD(proto, toString);
    ADD_METHOD(proto, transform);
    ADD_METHOD(proto, translate);
    ADD_METHOD(proto, viewTransformEnabled);
    ADD_METHOD(proto, viewport);
    ADD_METHOD(proto, window);
    ADD_METHOD(proto, worldMatrix);
    ADD_METHOD(proto, worldMatrixEnabled);
    ADD_METHOD(proto, worldTransform);

    QScript::registerPointerMetaType<QPainter>(eng, proto);

    qScriptRegisterSequenceMetaType<QVector<QRectF> >(eng);

    QScriptValue ctorFun = eng->newFunction(ctor, proto);
    ADD_ENUM_VALUE(ctorFun, QPainter, Antialiasing);
    ADD_ENUM_VALUE(ctorFun, QPainter, TextAntialiasing);
    ADD_ENUM_VALUE(ctorFun, QPainter, SmoothPixmapTransform);
    ADD_ENUM_VALUE(ctorFun, QPainter, HighQualityAntialiasing);

    eng->setDefaultPrototype(qMetaTypeId<QPainter*>(), proto);

    return ctorFun;
}
