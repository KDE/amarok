/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *           (C) 2008 Seb Ruiz <ruiz@kde.org>                                 *
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

#ifndef AMAROK_TOKEN_H
#define AMAROK_TOKEN_H

#include <QFrame>
#include <QLabel>
#include <QPixmap>

class Token;

class TokenBuilder
{
    public:
        Token* buildToken( const QString &element ) const;

};

//Defines a part of a filename, drag&droppable in the FilenameLayoutWidget bar from the TokenListWidget list.
class Token : public QFrame
{
        Q_OBJECT

    public:
        enum Type { Unknown
                    , Ignore
                    , Track
                    , Title
                    , Artist
                    , Composer
                    , Year
                    , Album
                    , Comment
                    , Genre
                    , FileType
                    , Folder
                    , Initial
                    , DiscNumber
                    , Space
                    , Slash
                    , Dot
                    , Dash
                    , Underscore
                };

        explicit Token( Type type, QWidget *parent = 0 );

        QString tokenElement() const;
        static QString tokenElement( const Type type );

    private:
        void setIcon();
        QString iconName() const;
        QString text() const;
        QString prettyText() const;

        Type         m_type;
 
        QLabel      *m_iconContainer;
        QLabel      *m_label;
};

#endif // AMAROK_TOKEN_H

