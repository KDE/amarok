/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "amarok_export.h"

#include <KHBox>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>

#define POST_COMPLETION_DELAY 2000

/**
 * A widget that encapsulates a progress bar, a description string and a cancel button.
 */
class AMAROK_EXPORT ProgressBar : public QFrame
{
    Q_OBJECT

    public:
        ProgressBar( QWidget *parent );
        ~ProgressBar();

        void setDescription( const QString &description );
        ProgressBar *setAbortSlot( QObject *receiver, const char *slot, Qt::ConnectionType type = Qt::AutoConnection );

        QToolButton *cancelButton() { return m_cancelButton; }
        QProgressBar *progressBar() { return m_progressBar;  }
        QLabel *descriptionLabel()  { return m_descriptionLabel; }
        KHBox* extrabuttonSpace()   { return m_extraButtonSpace; }

        int maximum()               { return  m_progressBar->maximum(); }
        void setMaximum( int max )  { m_progressBar->setMaximum( max ); }
        int value()                 { return m_progressBar->value(); }
        void setValue( int value );
        int percentage();

    public slots:
        void cancel();
        void delayedDone();

    signals:
        void cancelled( ProgressBar* );
        void cancelled();
        void complete( ProgressBar* );
        void percentageChanged( int );

    private:
        QToolButton *m_cancelButton;
        QProgressBar *m_progressBar;
        QLabel *m_descriptionLabel;
        KHBox *m_extraButtonSpace;
};

#endif
