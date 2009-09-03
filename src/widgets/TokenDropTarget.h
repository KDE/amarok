/****************************************************************************************
 * Copyright (c) 2009 Thomas Lbking <thomas.luebking@web.de>                            *
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

#ifndef TOKENDROPTARGET_H
#define TOKENDROPTARGET_H

#include <QWidget>

class QBoxLayout;
class QHBoxLayout;
class QDropEvent;
class Token;
class TokenDragger;
class TokenFactory;

class TokenDropTarget : public QWidget
{
    Q_OBJECT
public:
    explicit TokenDropTarget( const QString &mimeType, QWidget *parent = 0);

    QWidget *childAt( const QPoint &pos ) const;
    void clear();
    virtual inline int count() const { return count( -1 ); }
    virtual int count ( int row ) const;
    QPoint index( Token* ) const;
    int row ( Token* ) const;
    int rows() const;
    inline uint rowLimit() const { return m_limits[1]; }
    inline void setRowLimit( uint r ) { m_limits[1] = r; }
    void setCustomTokenFactory( TokenFactory * factory );
    QList< Token *> drags( int row = -1 );

public slots:
    void insertToken( Token*, int row = -1, int col = -1 ); // -1 -> append to last row
//     inline uint columnLimit() const { return m_limits[0]; }
//     inline void setColumnLimit( uint c ) { m_limits[0] = c; }
signals:
    void changed();
    void focusReceived( QWidget* );

protected:
    bool eventFilter( QObject *, QEvent * );
    void paintEvent(QPaintEvent *);
    QBoxLayout *rowBox( QWidget *w, QPoint *idx = 0 ) const;
    QBoxLayout *rowBox( const QPoint &pt ) const;
protected:
    friend class TokenDragger;
    void deleteEmptyRows();

private:
    bool accept( QDropEvent* );
    QHBoxLayout *appendRow();
    void drop( Token*, const QPoint &pos = QPoint(0,0) );

private:
    uint m_limits[2];
    QString m_mimeType;
    TokenDragger *m_tokenDragger;
    TokenFactory *m_tokenFactory;
};

#endif
