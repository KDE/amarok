/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef TOKENWITHLAYOUT_H
#define TOKENWITHLAYOUT_H

#include <Token.h>


class TokenWithLayoutFactory : public TokenFactory
{
public:
    virtual Token * createToken( const QString &text, const QString &iconName, int value, QWidget *parent = 0 );
};

/**
An extended Token with controls for layouting the token and getting layout values for use outside the Token.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class TokenWithLayout : public Token
{
    Q_OBJECT
public:
    TokenWithLayout( const QString &text, const QString &iconName, int value, QWidget *parent = 0 );
    ~TokenWithLayout();

    Qt::Alignment alignment();
    void setAlignment( Qt::Alignment alignment );

    bool bold();
    void setBold( bool bold );
    qreal size();

    
public slots:

    void setSize( int size );

protected:
    virtual void contextMenuEvent( QContextMenuEvent * event );

private:

    Qt::Alignment m_alignment;
    bool m_bold;
    qreal m_size;

};

#endif
