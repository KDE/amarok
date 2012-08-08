/****************************************************************************************
 * Copyright (c) 2009 Thomas Lbking <thomas.luebking@web.de>                            *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef TOKENDROPTARGET_H
#define TOKENDROPTARGET_H

#include <QWidget>

class QBoxLayout;
class QHBoxLayout;
class QDropEvent;
class Token;
class TokenDragger;
class TokenFactory;

/** A widget that accepts dragging Tokens into it.
    Used in several dialogs within Amarok e.g. the FilenameLayoutWidget and the
    LayoutEditWidget.

    The DropTarget can have one or more rows, limited by the rowLimit.
*/
class TokenDropTarget : public QWidget
{
    Q_OBJECT
public:
    explicit TokenDropTarget( const QString &mimeType, QWidget *parent = 0);

    /** Removes all tokens from the drop target. */
    void clear();

    /** Returns the total number of all tokens contained in this drop traget. */
    int count() const;

    /** Returns the row and column position of the \p token. */
    QPoint index( Token* token ) const;

    /** Returns the row of the given \p Token or -1 if not found. */
    int row ( Token* ) const;

    /** Returns the number of rows that this layout has. */
    int rows() const { return m_rows; };

    /** Returns the maximum allowed number of rows.
        A number of 0 means that the row count is not limited at all.
    */
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

    /** Return the enclosing box layout and the row and column position of the widget \p w.  */
    QBoxLayout *rowBox( QWidget *w, QPoint *idx = 0 ) const;

    /** Return the box layout at the position \p pt. */
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

    int m_rows; // contains the number of real rows (using the layout is not very practical in that since it seems that the layout adds at least one empty entry by itself)
    bool m_horizontalStretch;
};

#endif
