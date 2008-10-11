/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef PROGRESSBARNG_H
#define PROGRESSBARNG_H

#include "amarok_export.h"

#include <KHBox>

#include <QLabel>
#include <QProgressBar>
#include <QToolButton>


#define POST_COMPLETION_DELAY 2000


/**
A widget that encapsulates a progress bar, a description string and a cancel button.

	@author
*/
class AMAROK_EXPORT ProgressBarNG : public KHBox
{

    Q_OBJECT
public:
    ProgressBarNG( QWidget * parent );

    ~ProgressBarNG();

    void setDescription( const QString &description );
    ProgressBarNG * setAbortSlot( QObject *receiver, const char *slot );

    QToolButton *cancelButton()
    {
        return m_cancelButton;
    }
    QProgressBar *progresBar()
    {
        return m_progresBar;
    }
    QLabel *descriptionLabel()
    {
        return m_descriptionLabel;
    }
    KHBox* extrabuttonSpace()
    {
        return m_extraButtonSpace;
    }

    void setValue( int value );
    void setMaximum( int max )
    {
        m_progresBar->setMaximum( max );
    }
    int maximum()
    {
        return  m_progresBar->maximum();
    }
    int percentage();
    int value()
    {
        return m_progresBar->value();
    }


public slots:

    void cancel();
    void delayedDone();

signals:
    void cancelled( ProgressBarNG * );
    void cancelled();
    void complete( ProgressBarNG * );
    void percentageChanged( int );



private:
    QToolButton *m_cancelButton;
    QProgressBar *m_progresBar;
    QLabel *m_descriptionLabel;
    KHBox *m_extraButtonSpace;;

};

#endif
