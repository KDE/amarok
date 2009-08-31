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

#ifndef SCRIPT_H
#define SCRIPT_H

#include <QScriptValue>

#include <Plasma/AppletScript>
#include <Plasma/DataEngine>

#include "uiloader.h"

class QScriptEngine;
class QScriptContext;

class AppletInterface;

class SimpleJavaScriptApplet : public Plasma::AppletScript
{
    Q_OBJECT

public:
    SimpleJavaScriptApplet( QObject *parent, const QVariantList &args );
    ~SimpleJavaScriptApplet();
    bool init();

    void reportError();

    void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
    QList<QAction*> contextualActions();
    void constraintsEvent(Plasma::Constraints constraints);

    Q_INVOKABLE QString findDataResource( const QString &filename );
    Q_INVOKABLE void debug( const QString &msg );

    QScriptValue variantToScriptValue(QVariant var);

public slots:
    void dataUpdated( const QString &name, const Plasma::DataEngine::Data &data );
    void configChanged();
    void executeAction(const QString &name);

private:
    void importExtensions();
    void setupObjects();
    void callFunction(const QString &functionName, const QScriptValueList &args = QScriptValueList());

    static QString findSvg(QScriptEngine *engine, const QString &file);
    static QScriptValue jsi18n(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsi18nc(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsi18np(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsi18ncp(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue dataEngine(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue service(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue loadui(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue newPlasmaSvg(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue newPlasmaFrameSvg(QScriptContext *context, QScriptEngine *engine);

    void installWidgets( QScriptEngine *engine );
    static QScriptValue createWidget(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue notSupported(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue print(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue createPrototype( QScriptEngine *engine, const QString &name );


private:
    static KSharedPtr<UiLoader> s_widgetLoader;
    QScriptEngine *m_engine;
    QScriptValue m_self;
    QVariantList m_args;
    AppletInterface *m_interface;
    friend class AppletInterface;
};


#endif // SCRIPT_H

