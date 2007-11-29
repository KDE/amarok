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

#include "AmarokStatusBar.h"
#include "StatusBarMessageLabel.h"

#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtCore/QTimer>

#include <kiconloader.h>
#include <kvbox.h>

AmarokStatusBar* AmarokStatusBar::s_instance = 0;

AmarokStatusBar::AmarokStatusBar(QWidget* parent) :
    KHBox(parent),
    m_messageLabel(0),
    m_progressBar(0),
    m_progress(100)
{
    s_instance = this;

    setSpacing(4);

    m_messageLabel = new StatusBarMessageLabel(this);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    const int contentHeight = QFontMetrics(m_messageLabel->font()).height();
    const int barHeight = contentHeight + 8;

    setMinimumHeight(barHeight);
    m_messageLabel->setMinimumTextHeight(barHeight);
    m_progressBar->setFixedHeight(contentHeight);
    m_progressBar->setMaximumWidth(200);
}


AmarokStatusBar::~AmarokStatusBar()
{
}

void AmarokStatusBar::setMessage(const QString& msg, KDE::StatusBar::MessageType type)
{
    m_messageLabel->setMessage(msg, type);

    const int widthGap = m_messageLabel->widthGap();
    if (widthGap > 0) {
        m_progressBar->hide();
        m_progressText->hide();
    }
}

KDE::StatusBar::MessageType AmarokStatusBar::type() const
{
    return m_messageLabel->type();
}

QString AmarokStatusBar::message() const
{
    return m_messageLabel->text();
}

void AmarokStatusBar::setProgressText(const QString& text)
{
    m_progressText->setText(text);
}

QString AmarokStatusBar::progressText() const
{
    return m_progressText->text();
}

void AmarokStatusBar::setProgress(int percent)
{
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    m_progress = percent;
    if( m_messageLabel->type() == KDE::StatusBar::Error) {
        // don't update any widget or status bar text if an
        // error message is shown
        return;
    }

    m_progressBar->setValue(m_progress);
    if (!m_progressBar->isVisible() || (percent == 100)) {
        QTimer::singleShot(500, this, SLOT(updateProgressInfo()));
    }

    const QString& defaultText = m_messageLabel->defaultText();
    const QString msg(m_messageLabel->text());
    if ((percent == 0) && !msg.isEmpty()) {
        setMessage(QString(), KDE::StatusBar::None);
    } else if ((percent == 100) && (msg != defaultText)) {
        setMessage(defaultText, KDE::StatusBar::None);
    }
}

void AmarokStatusBar::clear()
{
    setMessage(m_messageLabel->defaultText(), KDE::StatusBar::None);
}

void AmarokStatusBar::setDefaultText(const QString& text)
{
    m_messageLabel->setDefaultText(text);
}

const QString& AmarokStatusBar::defaultText() const
{
    return m_messageLabel->defaultText();
}

void AmarokStatusBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QMetaObject::invokeMethod(this, "showSpaceInfo", Qt::QueuedConnection);
}

void AmarokStatusBar::updateProgressInfo()
{
    const bool isErrorShown = (m_messageLabel->type() == KDE::StatusBar::Error);
    if (m_progress < 100) {
        // show the progress information and hide the space information
        if (!isErrorShown) {
            m_progressText->show();
            m_progressBar->show();
        }
    } else {
        // hide the progress information and show the space information
        m_progressText->hide();
        m_progressBar->hide();
    }
}

#include "AmarokStatusBar.moc"
