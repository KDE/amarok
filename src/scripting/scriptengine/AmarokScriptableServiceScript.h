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
#include <QJSValue>

class StreamItem;
class QPixmap;
class QJSEngine;

namespace AmarokScript
{
    // SCRIPTDOX: ScriptableServiceScript
    /**
     * Usage: First create the scriptable service using a call to
     * ScriptableServiceScript( string name, int levels, string shortDescription,  string rootHtml, bool showSearchBar )
     */
    class ScriptableServiceScript : public QObject
    {
        Q_OBJECT

        public:
            explicit ScriptableServiceScript( QJSEngine* engine );

            void slotPopulate(const QString &name, int level, int parent_id, const QString &callbackData, const QString &filter );
            void slotRequestInfo(const QString &name, int level, const QString &callbackData );

            void slotCustomize( const QString &name );

            /** Script Invokable **/
            Q_INVOKABLE int insertItem( StreamItem* item );
            Q_INVOKABLE void setCurrentInfo( const QString &infoHtml );

            Q_INVOKABLE int donePopulating() const;

            Q_INVOKABLE void setIcon( const QPixmap &icon );
            Q_INVOKABLE void setEmblem( const QPixmap &icon );
            Q_INVOKABLE void setScalableEmblem( const QString &emblemPath );

        private:
            QJSEngine* m_scriptEngine;
            int m_currentId;
            QString m_serviceName;
            Q_INVOKABLE QJSValue ScriptableServiceScript_prototype_ctor( const QJSValueList argument );
            static QJSValue ScriptableServiceScript_prototype_populate( QJSEngine *engine );

        Q_SIGNALS:
            void populate( int, const QString&, const QString& );
            void fetchInfo( int, const QString& );
            void customize();
    };
}

Q_DECLARE_METATYPE( AmarokScript::ScriptableServiceScript* )

#endif
