/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
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

#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include <QImage>
#include <QPixmap>
#include <QWidget>

class IconButton : public QWidget
{
    Q_OBJECT

public:
    IconButton( QWidget *parent = 0 );
    virtual QSize sizeHint() const;
    void setIcon( const QImage &img, int steps = 0 );

signals:
    void clicked();

protected:
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent ( QTimerEvent * );

    /**
     Reload the content for the given size
     The iconbutton preserves a square size, so sz.width() == sz.height()
    */
    virtual void reloadContent( const QSize &sz );

private:
    void updateIconBuffer();

    bool m_isClick;
    struct
    {
        int step;
        int steps;
        int timer;
    } m_anim;

    struct
    {
        QImage image;
        QPixmap pixmap;
    } m_buffer;

    QImage m_icon, m_oldIcon;
};


#endif  // end include guard
