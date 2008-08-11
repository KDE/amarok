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
		
		Q_PROPERTY( QString name WRITE setName READ name )
		Q_PROPERTY( QString infoHtml WRITE setInfoHtml READ infoHtml )
		Q_PROPERTY( QString playableUrl WRITE setPlayableUrl READ playableUrl )
		Q_PROPERTY( QString callbackData WRITE setCallbackData READ callbackData )
		
    public:
        StreamItem();
        ~StreamItem();
		
    private:
        QString name() const;
        QString infoHtml() const;
        QString playableUrl() const;
        QString callbackData() const;
        void setName( QString name );
        void setInfoHtml( QString infoHtml );
        void setPlayableUrl( QString playableUrl );
        void setCallbackData( QString callbackData );
		
        QString m_name;
        QString m_infoHtml;
        QString m_playableUrl;
        QString m_callbackData;
	};

class ScriptableServiceScript : public QObject, public QScriptable
{
    Q_OBJECT

	Q_PROPERTY( QString serviceName WRITE setServiceName READ serviceName )
	Q_PROPERTY( int levels WRITE setLevels READ levels )	
	Q_PROPERTY( QString shortDescription WRITE setShortDescription READ shortDescription )
	Q_PROPERTY( QString rootHtml WRITE setRootHtml READ rootHtml )
	Q_PROPERTY( bool showSearchBar WRITE setShowSearchBar READ showSearchBar )
	
public:
	ScriptableServiceScript( QScriptEngine* engine );
	~ScriptableServiceScript();
	
public:
	int insertItem( int level, const QString name, const QString infoHtml, const QString playableUrl, const QString callbackData );
	void slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter );

private:
	int m_currentId;
	QScriptEngine* m_scriptEngine;
	QString m_serviceName;
	int m_levels;
	QString m_shortDescription;
	QString m_rootHtml;
	bool m_showSearchBar;
	
	void setServiceName( QString name );
	QString serviceName() const;
	void setLevels( int levels );
	int levels() const;
	void setShortDescription( QString shortDescription );
	QString shortDescription() const;
	void setRootHtml( QString rootHtml );
	QString rootHtml() const;
	void setShowSearchBar( bool showSearchBar );
	bool showSearchBar() const; 
};

Q_DECLARE_METATYPE( StreamItem* )
Q_DECLARE_METATYPE( ScriptableServiceScript* )

#endif
