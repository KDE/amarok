/****************************************************************************************
 * Copyright (c) 2010 Emmanuel Wagner <manu.wagner@sfr.fr>                              *                          
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

#include <QGraphicsTextItem>
class QKeyEvent;

class SearchBarTextItem : public QGraphicsTextItem

{
    Q_OBJECT
public:
    explicit SearchBarTextItem( QGraphicsItem * parent = 0, QGraphicsScene * scene = 0 );
Q_SIGNALS:
    void editionValidated( QString editioncontent );
protected:
    virtual void keyPressEvent( QKeyEvent* Event );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event ); 
private:
    QString m_content;
};
