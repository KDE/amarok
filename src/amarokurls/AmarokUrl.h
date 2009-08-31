/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef AMAROKURL_H
#define AMAROKURL_H

#include "amarok_export.h"
#include "BookmarkViewItem.h"
#include "BookmarkGroup.h"

#include <QString>
#include <QStringList>


class AMAROK_EXPORT AmarokUrl : public BookmarkViewItem
{
public:
    AmarokUrl();
    explicit AmarokUrl( const QString & urlString, BookmarkGroupPtr parent = BookmarkGroupPtr() );
    explicit AmarokUrl( const QStringList & resultRow, BookmarkGroupPtr parent  = BookmarkGroupPtr() );

    ~AmarokUrl();

    void reparent( BookmarkGroupPtr parent );
    void initFromString( const QString & urlString );

    QString command() const;
    QString path() const;
    QMap<QString, QString> args() const;
    
    void setCommand( const QString &command );
    void setPath( const QString &path );
    void appendArg( const QString &name, const QString &value );

    void setName( const QString &name );
    void setDescription( const QString &description );

    void setCustomValue( const QString &custom );
    QString customValue() const;

    bool run();

    QString url() const;

    bool saveToDb();

    void setId( int id ) { m_id = id; }
    int id() { return m_id; }

    bool isNull();

    virtual QString name() const;
    virtual QString description() const;
    virtual BookmarkGroupPtr parent() const { return m_parent; }
    virtual void removeFromDb();
    virtual void rename( const QString &name );

    static QString escape( const QString &in );
    static QString unescape( const QString &in );

private:
    
    QString m_command;
    QString m_path;
    QMap<QString, QString> m_arguments;

    int m_id;
    BookmarkGroupPtr m_parent;
    QString m_description;
    QString m_name;

    //this value is used for storing application specific inoformation that should not be made user visible.
    QString m_customValue;
};

#endif
