/***************************************************************************
                     amarokbutton.h  -  description
                        -------------------
begin                : Oct 31 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKBUTTON_H
#define AMAROKBUTTON_H

#include <qlabel.h>
#include <qpixmap.h>

class QMouseEvent;
class QString;

/**
 * @brief: The "fake" button.
 */

class AmarokButton : public QLabel
{
    Q_OBJECT

    public:
        AmarokButton( QWidget *parent, QString activePixmap, QString inactivePixmap, bool toggleButton );
        ~AmarokButton();

        void setOn( bool enable );
        bool isOn();

        // ATTRIBUTES ------

    public slots:

    signals:
        void clicked();
        void toggled( bool on );

    private:
        void mousePressEvent( QMouseEvent *e );
        void mouseReleaseEvent( QMouseEvent *e );

        // ATTRIBUTES ------
        QPixmap m_activePixmap;
        QPixmap m_inactivePixmap;

        bool m_on;
        bool m_isToggleButton;
        bool m_clicked;
};
#endif
