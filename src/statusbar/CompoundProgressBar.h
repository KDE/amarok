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

#ifndef COMPOUNDPROGRESSBAR_H
#define COMPOUNDPROGRESSBAR_H

#include "statusbar/ProgressBar.h"
#include "statusbar/PopupWidget.h"

#include <QList>
#include <QMap>
#include <QMouseEvent>
#include <QRecursiveMutex>

/**
 * A progress bar that wraps a number of simple progress bars and displays their 
 * overall progress. Also features an expanded mode that allows the user to view 
 * and canceled individual operations
 */
class AMAROK_EXPORT CompoundProgressBar : public ProgressBar
{
    Q_OBJECT
public:
    explicit CompoundProgressBar( QWidget *parent );

    ~CompoundProgressBar() override;

    void addProgressBar( ProgressBar *progressBar, QObject *owner );

    void incrementProgress( const QObject *owner );
    void setProgressTotalSteps( const QObject *owner, int value );
    void setProgressStatus( const QObject *owner, const QString &text );
    void setProgress( const QObject *owner, int steps );

    /* reimplemented from QWidget to open/close the details widget */
    void mousePressEvent( QMouseEvent *event ) override;

public Q_SLOTS:
    void endProgressOperation( QObject *owner );
    void slotIncrementProgress();

Q_SIGNALS:
    void allDone();

protected Q_SLOTS:
    void cancelAll();
    void toggleDetails();

    void childPercentageChanged( );
    void childBarCancelled( ProgressBar *progressBar );
    void childBarComplete( ProgressBar *progressBar );

    void slotObjectDestroyed( QObject *object );

private:
    void showDetails();
    void hideDetails();

    void childBarFinished( ProgressBar *bar );

    int calcCompoundPercentage();

    QMap< const QObject *, ProgressBar *> m_progressMap;
    PopupWidget *m_progressDetailsWidget;
    QRecursiveMutex m_mutex; // protecting m_progressMap consistency
};

#endif
