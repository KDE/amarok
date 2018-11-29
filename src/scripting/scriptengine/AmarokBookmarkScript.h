/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef AMAROK_BOOKMARK_SCRIPT_H
#define AMAROK_BOOKMARK_SCRIPT_H

#include "amarokurls/BookmarkGroup.h"
#include "core/meta/Meta.h"

#include <QObject>
#include <QMap>
#include <QMetaType>

class QScriptContext;
class QScriptEngine;
class QScriptValue;

typedef QMap< QString, QString > StringMap;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Bookmark
    class AmarokBookmarkScript : public QObject
    {
        Q_OBJECT

        public:
            explicit AmarokBookmarkScript( QScriptEngine* scriptEngine );

            /**
             * @return bookmark for the current context view.
             */
            Q_INVOKABLE AmarokUrlPtr contextView();

            /**
             * @return bookmark for the current playlist view.
             */
            Q_INVOKABLE AmarokUrlPtr currentPlaylistView();

            /**
             * * @return bookmark for the current browser view.
             */
            Q_INVOKABLE AmarokUrlPtr browserView();

            /**
             * Bookmark the current track at the current position.
             */
            Q_INVOKABLE AmarokUrlPtr createCurrentTrackBookmark();
    };

    class BookmarkPrototype : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( int id READ id WRITE setId )
        Q_PROPERTY( BookmarkGroupPtr parent READ parent WRITE setParent )
        Q_PROPERTY( QString command READ command WRITE setCommand )
        Q_PROPERTY( QString name READ name WRITE setName )
        Q_PROPERTY( QString path READ path WRITE setPath )
        Q_PROPERTY( QString description READ description WRITE setDescription )
        Q_PROPERTY( bool isNull READ isNull )
        Q_PROPERTY( QString customValue READ customValue WRITE setCustomValue )
        Q_PROPERTY( QString url READ url )

        public:
            static QScriptValue bookmarkCtor( QScriptContext *context, QScriptEngine *engine );
            explicit BookmarkPrototype( AmarokUrlPtr bookmark );
            AmarokUrlPtr data() const { return m_url; }

            /**
             * Save the bookmark to db. Must not be null.
             */
            Q_INVOKABLE bool save();
            Q_INVOKABLE void initFromString( const QString & urlString );
            Q_INVOKABLE bool run();
            Q_INVOKABLE void removeFromDb();
            Q_INVOKABLE QString prettyCommand() const;
            Q_INVOKABLE StringMap args() const;

            /**
            * Sets the url argument named @p name to @p value. Overrides any possible
            * previous value.
            *
            * @param name The name of the url argument.
            * @param value The value.
            */
            Q_INVOKABLE void setArg( const QString &name, const QString &value );

        private:
            void setId( int id );
            int id() const;
            void setParent( BookmarkGroupPtr parent );
            QString command() const;
            void setCommand( const QString &command );
            void setName( const QString &name );
            QString name() const;
            QString path() const;
            void setPath( const QString &path );
            QString description() const;
            BookmarkGroupPtr parent() const;
            void setDescription( const QString &description );
            void setCustomValue( const QString &custom );
            QString customValue() const;
            QString url() const;
            bool isNull() const;

            AmarokUrlPtr m_url;
    };

    class BookmarkGroupPrototype : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( int id READ id )
        Q_PROPERTY( BookmarkGroupPtr parent READ parent WRITE setParent )
        Q_PROPERTY( QString name READ name WRITE setName )
        Q_PROPERTY( QString description READ description WRITE setDescription )
        Q_PROPERTY( int childCount READ childCount )

        public:
            explicit BookmarkGroupPrototype( BookmarkGroupPtr group );
            static QScriptValue bookmarkGroupCtor( QScriptContext *context, QScriptEngine *engine );
            BookmarkGroupPtr data() const { return m_group; }

            /**
            * Save the bookmark group to db.
            */
            Q_INVOKABLE void save();
            Q_INVOKABLE BookmarkGroupList childGroups() const;
            Q_INVOKABLE BookmarkList childBookmarks() const;
            Q_INVOKABLE void clear();
            Q_INVOKABLE void deleteChildBookmark( AmarokUrlPtr bookmark );
            Q_INVOKABLE void deleteChildBookmarkgroup( BookmarkGroupPtr bookmarkGroup );

        private:
            int id() const;
            QString name() const;
            QString description() const;
            int childCount() const;
            void setName( const QString &name );
            void setDescription( const QString &description );
            BookmarkGroupPtr parent() const;
            void setParent( BookmarkGroupPtr parent );

            BookmarkGroupPtr m_group;
    };
}

#endif
