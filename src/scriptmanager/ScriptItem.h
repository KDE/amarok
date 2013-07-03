/****************************************************************************************
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_SCRIPTITEM_H
#define AMAROK_SCRIPTITEM_H

#include "amarok_export.h"
#include "scriptengine/AmarokScriptableServiceScript.h"
#include "statusbar/PopupWidget.h"

#include <KPluginInfo>
#include <KUrl>

#include <QScriptValue>

namespace AmarokScript {
    class AmarokScript;
}
class ScriptItem;
class QScriptContext;
class QScriptEngine;
class QTimerEvent;

class ScriptTerminatorWidget : public PopupWidget
{
    Q_OBJECT
public:
    ScriptTerminatorWidget( const QString &message );

signals:
    void terminate();
};

class ScriptItem : public QObject
{
    Q_OBJECT
public:
    ScriptItem( QObject *parent, const QString &name, const QString &path, const KPluginInfo &info );

    QScriptEngine* engine() { return m_engine.data(); }
    ScriptableServiceScript* service() { return m_service.data(); }
    KUrl url() const{ return m_url; }
    KPluginInfo info() const { return m_info; }
    bool running() const { return m_running; }
    QString specPath() const;

    bool start( bool silent );

public slots:
    void stop();

private slots:
        void timerEvent ( QTimerEvent *event );

signals:
    void signalHandlerException(QScriptValue);

private:
    QString                                         m_name;
    KUrl                                            m_url;
    KPluginInfo                                     m_info;
    QWeakPointer<QScriptEngine>                     m_engine;
    /** Currently activated in the Script Manager */
    bool                                            m_running;
    bool                                            m_evaluating;
    QWeakPointer<ScriptableServiceScript>           m_service;
    QStringList                                     m_log;
    int                                             m_runningTime;
    int                                             m_timerId;
    QWeakPointer<ScriptTerminatorWidget>            m_popupWidget;

    /**
     * Initialize QScriptEngine and load wrapper classes
     */
    void initializeScriptEngine();
};

#endif /* AMAROK_SCRIPTMANAGER_H */
