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

class QMenu;

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

    bool bold() const;
    bool italic() const;
    inline qreal size() const { return width(); }
    qreal width() const;
    inline bool widthForced() const { return m_widthForced; }
    inline QString prefix() const { return m_prefix; }
    inline QString suffix() const { return m_suffix; }

public slots:
    void setAlignLeft( bool );
    void setAlignCenter( bool );
    void setAlignRight( bool );
    void setBold( bool bold );
    void setItalic( bool italic );
    void setPrefix( const QString& );
    void setSuffix( const QString& );
    void setWidth( int width );
    void setWidthForced( bool );

protected:
    virtual void fillMenu( QMenu * menu );
    virtual void menuExecuted( const QAction* action );
    virtual void contextMenuEvent( QContextMenuEvent * event );

private:

    Qt::Alignment m_alignment;
    bool m_bold;
    bool m_italic;
    bool m_widthForced;
    qreal m_width;
    QString m_prefix, m_suffix;

};

#endif
