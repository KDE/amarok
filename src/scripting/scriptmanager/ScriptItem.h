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
#include "scripting/scriptengine/AmarokScriptableServiceScript.h"
#include "scripting/scriptengine/ScriptingDefines.h"
#include "statusbar/PopupWidget.h"

#include <KPluginMetaData>

#include <QJSValue>
#include <QPointer>
#include <QUrl>

namespace AmarokScript {
    class AmarokScript;
}
class ScriptItem;
class QTimerEvent;

class ScriptTerminatorWidget : public PopupWidget
{
    Q_OBJECT
public:
    explicit ScriptTerminatorWidget( const QString &message );

Q_SIGNALS:
    void terminate();
};

class ScriptItem : public QObject
{
    Q_OBJECT
public:
    ScriptItem( QObject *parent, const QString &name, const QString &path, const KPluginMetaData &info );
    ~ScriptItem() override;

    QString name() const { return m_name; }
    AmarokScript::AmarokScriptEngine* engine() { return m_engine.data(); }
    AmarokScript::ScriptableServiceScript* service() { return m_service.data(); }
    QUrl url() const { return m_url; }
    KPluginMetaData info() const { return m_info; }
    bool running() const { return m_running; }
    QString metadataPath() const;
    QString output() const { return m_output.join(QStringLiteral("\n")); }
    QJSValue engineResult() const { return m_engineResult; };

    virtual bool start( bool silent );
    virtual void pause();
    void uninstall();

public Q_SLOTS:
    void stop();

    /**
     * Warn the user about scripts calling deprecated API calls
     */
    virtual void slotDeprecatedCall( const QString &call );

private Q_SLOTS:
    void timerEvent( QTimerEvent *event ) override;

Q_SIGNALS:
    void signalHandlerException( QJSValue );
    void evaluated( QString output );
    void uninstalled();

protected:
    /**
     * Initialize QJSEngine and load wrapper classes
     */
    virtual void initializeScriptEngine();
    virtual QString handleError( QJSValue *evalReturn );

private:
    QString                                             m_name;
    QUrl                                                m_url;
    KPluginMetaData                                     m_info;
    QPointer<AmarokScript::AmarokScriptEngine>          m_engine;
    QJSValue                                            m_engineResult;
    /** Currently activated in the Script Manager */
    bool                                                m_running;
    QStringList                                         m_log;
    QPointer<AmarokScript::ScriptableServiceScript>     m_service;
    QStringList                                         m_output;
    int                                                 m_runningTime;
    int                                                 m_timerId;
    QPointer<ScriptTerminatorWidget>                    m_popupWidget;
    bool                                                m_qtScriptCompat = true;
};

#endif /* AMAROK_SCRIPTITEM_H */
