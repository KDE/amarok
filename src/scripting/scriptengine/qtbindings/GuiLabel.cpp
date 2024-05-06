/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GuiLabel.h"

using namespace QtBindings::Gui;

Label::Label(QWidget *parent, Qt::WindowFlags f) : QLabel(parent, f)
{
}

Label::Label(const Label &other) : QLabel (other.text())
{
    *this=other;
}

Label::Label(const QString &text, QWidget *parent,
                              Qt::WindowFlags f) : QLabel(text, parent, f)
{
}

Label::~Label()
{
}

Qt::Alignment Label::alignment() const
{
    return QLabel::alignment();
}

QWidget *Label::buddy() const
{
    return QLabel::buddy();
}

void Label::clear()
{
    QLabel::clear();
}

bool Label::hasScaledContents() const
{
    return QLabel::hasScaledContents();
}

bool Label::hasSelectedText() const
{
    return QLabel::hasSelectedText();
}

int Label::indent() const
{
    return QLabel::indent();
}

int Label::margin() const
{
    return QLabel::margin();
}

QMovie *Label::movie() const
{
    return QLabel::movie();
}

bool Label::openExternalLinks() const
{
    return QLabel::openExternalLinks();
}

QPicture Label::picture() const
{
    return QLabel::picture( Qt::ReturnByValue );
}

QPixmap Label::pixmap() const
{
    return QLabel::pixmap( Qt::ReturnByValue );
}

QString Label::selectedText() const
{
    return QLabel::selectedText();
}

int Label::selectionStart() const
{
    return QLabel::selectionStart();
}

void Label::setAlignment(Qt::Alignment alignment)
{
    QLabel::setAlignment(alignment);
}

void Label::setBuddy(QWidget *buddy)
{
    QLabel::setBuddy(buddy);
}

void Label::setIndent(int indent)
{
    QLabel::setIndent(indent);
}

void Label::setMargin(int margin)
{
    QLabel::setMargin(margin);
}

void Label::setMovie(QMovie *movie)
{
    QLabel::setMovie(movie);
}

void Label::setNum(double num)
{
    QLabel::setNum(num);
}

void Label::setNum(int num)
{
    QLabel::setNum(num);
}

void Label::setOpenExternalLinks(bool open)
{
    QLabel::setOpenExternalLinks(open);
}

void Label::setPicture(const QPicture &picture)
{
    QLabel::setPicture(picture);
}

void Label::setPixmap(const QPixmap &pixMap)
{
    QLabel::setPixmap(pixMap);
}

void Label::setScaledContents(bool y)
{
    QLabel::setScaledContents(y);
}

void Label::setSelection(int start, int length)
{
    QLabel::setSelection(start, length);
}

void Label::setText(const QString &text)
{
    QLabel::setText(text);
}

void Label::setTextFormat(Qt::TextFormat textFormat)
{
    QLabel::setTextFormat(textFormat);
}

void Label::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    QLabel::setTextInteractionFlags(flags);
}

void Label::setWordWrap(bool on)
{
    QLabel::setWordWrap(on);
}

QString Label::text() const
{
    return QLabel::text();
}

Qt::TextFormat Label::textFormat() const
{
    return QLabel::textFormat();
}

Qt::TextInteractionFlags Label::textInteractionFlags() const
{
    return QLabel::textInteractionFlags();
}

bool Label::wordWrap() const
{
    return QLabel::wordWrap();
}

bool Label::close()
{
    return QWidget::close();
}

void Label::hide()
{
    QWidget::hide();
}

void Label::lower()
{
    QWidget::lower();
}

void Label::raise()
{
    QWidget::raise();
}

void Label::repaint()
{
    QWidget::repaint();
}

void Label::setDisabled(bool disable)
{
    QWidget::setDisabled(disable);
}

void Label::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
}

void Label::setFocus()
{
    QWidget::setFocus();
}

void Label::setHidden(bool hidden)
{
    QWidget::setHidden(hidden);
}

void Label::setStyleSheet(const QString &styleSheet)
{
    QWidget::setStyleSheet(styleSheet);
}

void Label::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}

void Label::setWindowModified(bool windowModified)
{
    QWidget::setWindowModified(windowModified);
}

void Label::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
}

void Label::show()
{
    QWidget::show();
}

void Label::showFullScreen()
{
    QWidget::showFullScreen();
}

void Label::showMaximized()
{
    QWidget::showMaximized();
}

void Label::showMinimized()
{
    QWidget::showMinimized();
}

void Label::showNormal()
{
    QWidget::showNormal();
}

void Label::update()
{
    QWidget::update();
}

Label &Label::operator=(const Label &other)
{
    if (this != &other) {
        this->setEnabled( other.isEnabled() ) ;
        this->setBuddy( other.buddy() );
        this->setIndent( other.indent() );
        this->setMargin( other.margin() );
        this->setMovie( other.movie() );
        this->setOpenExternalLinks( other.openExternalLinks() );
        this->setPicture( other.picture() ) ;
        this->setPixmap( other.pixmap() );
        this->setScaledContents( other.hasScaledContents() );
        this->setSelection( other.selectionStart(), other.selectedText().length() );
        this->setText( other.text() );
        this->setTextFormat( other.textFormat() );
        this->setTextInteractionFlags( other.textInteractionFlags() );
        this->setWordWrap( other.wordWrap() );
    }
    return *this;
}

