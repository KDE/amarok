/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef AMAROKSTATUSBAR_H
#define AMAROKSTATUSBAR_H

#include <khbox.h>
#include "StatusBarBase.h"

class KUrl;
class StatusBarMessageLabel;
class QProgressBar;
class QLabel;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages and progress
 * information.
 */
class AMAROK_EXPORT AmarokStatusBar : public KHBox
{
    Q_OBJECT

public:
    static AmarokStatusBar* s_instance;

    /**
     * Describes the type of the message text. Dependent
     * from the type a corresponding icon and color is
     * used for the message text.
     */
    enum Type {
        Default,
        OperationCompleted,
        Information,
        Error
    };

    AmarokStatusBar(QWidget* parent);

    static AmarokStatusBar* instance() { return s_instance; }

    virtual ~AmarokStatusBar();

    /**
     * Sets the message text to \a msg. Dependant
     * from the given type \a type an icon is shown and
     * the color of the text is adjusted. The height of
     * the statusbar is automatically adjusted in a way,
     * that the full text fits into the available width.
     *
     * If a progress is ongoing and a message
     * with the type Type::Error is set, the progress
     * is cleared automatically.
     */
    void setMessage(const QString& msg, KDE::StatusBar::MessageType type);
    QString message() const;

    KDE::StatusBar::MessageType type() const;

    /**
     * Sets the text for the progress information.
     * The text is shown with a delay of 300 milliseconds:
     * if the progress set by DolphinStatusBar::setProgress()
     * does reach 100 % within 300 milliseconds,
     * the progress text is not shown at all. This assures that
     * no flickering occurs for showing a progress of fast
     * operations.
     */
    void setProgressText(const QString& text);
    QString progressText() const;

    /**
     * Sets the progress in percent (0 - 100). The
     * progress is shown with a delay of 300 milliseconds:
     * if the progress does reach 100 % within 300 milliseconds,
     * the progress is not shown at all. This assures that
     * no flickering occurs for showing a progress of fast
     * operations.
     */
    void setProgress(int percent);
    int progress() const
    {
        return m_progress;
    }

    /**
     * Clears the message text of the status bar by replacing
     * the message with the default text, which can be set
     * by DolphinStatusBar::setDefaultText(). The progress
     * information is not cleared.
     */
    void clear();

    /**
     * Sets the default text, which is shown if the status bar
     * is cleared by DolphinStatusBar::clear().
     */
    void setDefaultText(const QString& text);
    const QString& defaultText() const;

protected:
    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void updateProgressInfo();

private:
    StatusBarMessageLabel* m_messageLabel;

    QLabel* m_progressText;
    QProgressBar* m_progressBar;
    int m_progress;
};

namespace The
{
    inline AmarokStatusBar *amarokStatusBar() { return AmarokStatusBar::instance(); }
}

#endif
