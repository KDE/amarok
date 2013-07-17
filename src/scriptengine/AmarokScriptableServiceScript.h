/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_SCRIPTABLE_SERVICE_SCRIPT_H
#define AMAROK_SCRIPTABLE_SERVICE_SCRIPT_H

#include <QMetaType> // for Q_DECLARE_METATYPE
#include <QObject>
#include <QScriptable>
namespace AmarokScript{
    class StreamItem;
};
class QPixmap;
class QScriptContext;
class QScriptEngine;

namespace AmarokScript{

    class ScriptableServiceScript : public QObject, public QScriptable
    {
        Q_OBJECT

        public:
            ScriptableServiceScript( QScriptEngine* engine );

            void slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter );
            void slotRequestInfo( QString name, int level, QString callbackData );

            void slotCustomize( const QString &name );

        public slots:
            int insertItem( StreamItem* item );
            void setCurrentInfo( const QString &infoHtml );

            int donePopulating() const;

            void setIcon( const QPixmap &icon );
            void setEmblem( const QPixmap &icon );
            void setScalableEmblem( const QString &emblemPath );

        private:
            QScriptEngine* m_scriptEngine;
            int m_currentId;
            QString m_serviceName;
            static QScriptValue ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine );
            static QScriptValue ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine );

        signals:
            void populate( int, QString, QString );
            void fetchInfo( int, QString );
            void customize();
    };
}

Q_DECLARE_METATYPE( AmarokScript::ScriptableServiceScript* )

#endif
