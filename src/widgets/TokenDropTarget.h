/****************************************************************************************
 * Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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
    explicit TokenDropTarget( QWidget *parent = nullptr );
    ~TokenDropTarget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /** Removes all tokens from the drop target. */
    void clear();

    /** Returns the total number of all tokens contained in this drop traget. */
    int count() const;

    /** Returns the row and column position of the \p token. */
    QPoint index( Token* token ) const;

    /** Returns the row of the given \p Token or -1 if not found. */
    int row ( Token* ) const;

    /** Returns the number of rows that this layout has. */
    uint rows() const { return m_rows; }

    /** Returns the maximum allowed number of rows.
        A number of 0 means that the row count is not limited at all.
    */
    uint rowLimit() const { return m_rowLimit; }
    void setRowLimit( uint r );

    /** Set a custom factory that creates tokens.
        The default factory is the one that creates normal tokens.
        LayoutEditWidget set's this for the factory that creates StyledTokens.

        The factory will be deleted by the TokenDropTarget.
    */
    void setCustomTokenFactory( TokenFactory * factory );

    void setVerticalStretch( bool value );

    /** Returns all the tokens from the specified row.
        If row == -1 returns all tokens. */
    QList< Token *> tokensAtRow( int row = -1 );

public Q_SLOTS:
    /** Insert the token at the given row and col position.
        The token will be reparented for the TokenDropTarget.
    */
    void insertToken( Token*, int row, int col );
    void appendToken( Token *token ) { insertToken( token, -1, -1 ); } // -1 -> append to last row
    void removeToken( Token* );

    void deleteEmptyRows();
Q_SIGNALS:
    void changed();

    /** Emitted if a new token got the focus. */
    void tokenSelected( Token* );

protected:
    void dragEnterEvent( QDragEnterEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;

    /** Draws a "drop here" text if empty */
    void paintEvent(QPaintEvent *) override;

    /** Return the enclosing box layout and the row and column position of the widget \p w.  */
    QBoxLayout *rowBox( QWidget *w, QPoint *idx = nullptr ) const;

    /** Return the box layout at the position \p pt. */
    QBoxLayout *rowBox( const QPoint &pt ) const;

private:
    QHBoxLayout *appendRow();

    /** Returns the token at the given global position */
    Token* tokenAt( const QPoint &pos ) const;

    void drop( Token*, const QPoint &pos = QPoint(0,0) );

private:
    /** Maximum number of allowed rows.
        If 0 the number is unlimited. */
    uint m_rowLimit;

    /** contains the number of real rows
        (using the layout is not very practical in that since it seems that the layout
        adds at least one empty entry by itself if it's empty) */
    uint m_rows;

    /** True if a stretch is inserted as a last row.
        For now we always have a vertical stretch if the m_rowLimit > 1 */
    bool m_verticalStretch;

    TokenFactory *m_tokenFactory;
};

#endif
