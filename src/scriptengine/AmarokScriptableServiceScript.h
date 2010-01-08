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

#include <QObject>
#include <QPixmap>
#include <QtScript>

class StreamItem : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY( QString itemName WRITE setItemName READ itemName )
	Q_PROPERTY( QString infoHtml WRITE setInfoHtml READ infoHtml )
	Q_PROPERTY( QString playableUrl WRITE setPlayableUrl READ playableUrl )
    Q_PROPERTY( QString callbackData WRITE setCallbackData READ callbackData )
    Q_PROPERTY( int level WRITE setLevel READ level )
            

    Q_PROPERTY( QString album WRITE setAlbum READ album )
    Q_PROPERTY( QString artist WRITE setArtist READ artist )
    Q_PROPERTY( QString genre WRITE setGenre READ genre )
    Q_PROPERTY( QString composer WRITE setComposer READ composer )
    Q_PROPERTY( int year WRITE setYear READ year )
    Q_PROPERTY( QString coverUrl WRITE setCoverUrl READ coverUrl )
            

    public:
        StreamItem( QScriptEngine *engine );
        ~StreamItem();

        QString itemName() const;
        QString infoHtml() const;
        QString playableUrl() const;
        QString callbackData() const;
        int level() const;

        QString album() const;
        QString artist() const;
        QString genre() const;
        QString composer() const;
        int year() const;
        QString coverUrl();

        void setItemName( QString name );
        void setInfoHtml( QString infoHtml );
        void setPlayableUrl( QString playableUrl );
        void setCallbackData( QString callbackData );
        void setLevel( int level );

        void setAlbum( QString album );
        void setArtist( QString artist );
        void setGenre( QString genre );
        void setComposer( QString composer );
        void setYear( int year );
        void setCoverUrl( QString url );
  
        
    private:
        QString m_name;
        QString m_infoHtml;
        QString m_playableUrl;
        QString m_callbackData;
        int m_level;

        //these are not required but can be used to override what is shown in the playlist and elsewhere.
        QString m_album;
        QString m_artist;
        QString m_genre;
        QString m_composer;
        int m_year;
        QString m_coverUrl;

};

class ScriptableServiceScript : public QObject, public QScriptable
{
    Q_OBJECT
	
    public:
        ScriptableServiceScript( QScriptEngine* engine );
        ~ScriptableServiceScript();
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

Q_DECLARE_METATYPE( StreamItem* )
Q_DECLARE_METATYPE( ScriptableServiceScript* )

#endif
