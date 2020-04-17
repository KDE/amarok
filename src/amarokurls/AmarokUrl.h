/****************************************************************************************
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
    explicit AmarokUrl( const QString & urlString, const BookmarkGroupPtr &parent = BookmarkGroupPtr() );
    explicit AmarokUrl( const QStringList & resultRow, const BookmarkGroupPtr &parent  = BookmarkGroupPtr() );

    ~AmarokUrl() override;

    void reparent( const BookmarkGroupPtr &parent );
    void initFromString( const QString & urlString );

    QString command() const;
    QString prettyCommand() const;
    QString path() const;
    QMap<QString, QString> args() const;

    void setCommand( const QString &command );
    void setPath( const QString &path );

    /**
     * Sets the url argument named @param name to @param value . Overrides any possible
     * previous value.
     */
    void setArg( const QString &name, const QString &value );

    void setName( const QString &name );
    void setDescription( const QString &description ) override;

    void setCustomValue( const QString &custom );
    QString customValue() const;

    bool run();

    QString url() const;

    bool saveToDb();

    void setId( int id ) { m_id = id; }
    int id() const { return m_id; }

    bool isNull() const;

    QString name() const override;
    QString description() const override;
    BookmarkGroupPtr parent() const override { return m_parent; }
    void removeFromDb() override;
    void rename( const QString &name ) override;

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

    //this value is used for storing application specific information that should not be made user visible.
    QString m_customValue;
};

#endif
