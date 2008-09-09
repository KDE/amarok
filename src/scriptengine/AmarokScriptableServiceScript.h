/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_SCRIPTABLE_SERVICE_SCRIPT_H
#define AMAROK_SCRIPTABLE_SERVICE_SCRIPT_H

#include <QObject>
#include <QtScript>

class StreamItem : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY( QString itemName WRITE setItemName READ itemName )
	Q_PROPERTY( QString infoHtml WRITE setInfoHtml READ infoHtml )
	Q_PROPERTY( QString playableUrl WRITE setPlayableUrl READ playableUrl )
    Q_PROPERTY( QString callbackData WRITE setCallbackData READ callbackData )
    Q_PROPERTY( int level WRITE setLevel READ level )

    public:
        StreamItem( QScriptEngine *engine );
        ~StreamItem();

        QString itemName() const;
        QString infoHtml() const;
        QString playableUrl() const;
        QString callbackData() const;
        int level() const;
        void setItemName( QString name );
        void setInfoHtml( QString infoHtml );
        void setPlayableUrl( QString playableUrl );
        void setCallbackData( QString callbackData );
        void setLevel( int level );
    private:
        QString m_name;
        QString m_infoHtml;
        QString m_playableUrl;
        QString m_callbackData;
        int m_level;
};

class ScriptableServiceScript : public QObject, public QScriptable
{
    Q_OBJECT
	
    public:
        ScriptableServiceScript( QScriptEngine* engine );
        ~ScriptableServiceScript();
        void slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter );

    public slots:
        int insertItem( StreamItem* item );
        int donePopulating() const;

    private:
        QScriptEngine* m_scriptEngine;
        int m_currentId;
        QString m_serviceName;
        static QScriptValue ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine );
        static QScriptValue ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine );

    signals:
        void populate( int, QString, QString );
};

Q_DECLARE_METATYPE( StreamItem* )
Q_DECLARE_METATYPE( ScriptableServiceScript* )

#endif
