/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef UPCOMINGEVENTSSTACKITEM_H
#define UPCOMINGEVENTSSTACKITEM_H 

#include <QGraphicsWidget>
#include <QIcon>

class QGraphicsSceneMouseEvent;
class UpcomingEventsStack;
class UpcomingEventsStackItemPrivate;

/**
 * A widget that mimics the look and feel of Plasma::ExtenderItem, to be used
 * within an UpcomingEventsStack. It has a toolbox area containing icons,
 * text, and push buttons for pleasure.
 */
class UpcomingEventsStackItem : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( QGraphicsWidget *widget READ widget WRITE setWidget )
    Q_PROPERTY( QString title READ title WRITE setTitle )
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( QIcon icon READ icon WRITE setIcon )
    Q_PROPERTY( UpcomingEventsStack *stack READ stack )
    Q_PROPERTY( bool collapsed READ isCollapsed WRITE setCollapsed NOTIFY collapseChanged )

public:
    explicit UpcomingEventsStackItem( const QString &name, UpcomingEventsStack *parent );
    ~UpcomingEventsStackItem();

    /**
     * @param widget The widget that should be wrapped into the stack item.
     *               It has to be a QGraphicsWidget.
     */
    void setWidget( QGraphicsWidget *widget );

    /**
     * @return The widget that is wrapped into the stack item.
     */
    QGraphicsWidget *widget() const;

    /**
     * @param title the title that will be shown in the stack item's dragger.
     */
    void setTitle( const QString &title );

    /**
     * @return the title shown in the stack item's dragger.
     */
    QString title() const;

    /**
     * You can assign names to stack items to look them up through the item() function.
     * Make sure you only use unique names.
     * @param name the name of the item.
     */
    void setName( const QString &name );

    /**
     * @return the name of the item.
     */
    QString name() const;

    /**
     * Adds custom actions to appear in the drag handle.
     * @param action the action to add. Actions will be displayed as an icon in the drag
     * handle.
     */
    void addAction( const QString &name, QAction *action );

    /**
     * @return the custom actions in the drag handle.
     */
    QHash<QString, QAction *> actions() const;

    /**
     * @return the QAction with the given name from our collection.
     */
    QAction *action( const QString &name ) const;

    /**
     * @param icon the icon to display in the stack item's drag handle.
     */
    void setIcon( const QIcon &icon );

    /**
     * @param icon the icon name to display in the stack item's drag handle.
     */
    void setIcon( const QString &icon );

    /**
     * @return the icon being displayed in the stack item's drag handle.
     */
    QIcon icon() const;

    /**
     * @return the stack this items belongs to.
     */
    UpcomingEventsStack *stack() const;

    /**
     * @return whether or not the stack item  is collapsed.
     */
    bool isCollapsed() const;

    QRectF boundingRect() const;

public Q_SLOTS:
    void setCollapsed( bool collapsed );
    void showCloseButton( bool show = true );

Q_SIGNALS:
    void collapseChanged( bool isCollapsed );

protected:
    void mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const;

private:
    UpcomingEventsStackItemPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( UpcomingEventsStackItem )
    Q_DISABLE_COPY( UpcomingEventsStackItem )

    Q_PRIVATE_SLOT( d_ptr, void _themeChanged() )
    Q_PRIVATE_SLOT( d_ptr, void _toggleCollapse() )
    Q_PRIVATE_SLOT( d_ptr, void _updateToolbox() )
};

#endif /* UPCOMINGEVENTSSTACKITEM_H */
