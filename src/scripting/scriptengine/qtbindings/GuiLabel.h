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

#ifndef GUILABEL_H
#define GUILABEL_H

#include "QtBinding.h"

#include <QLabel>
#include <QPicture>

namespace QtBindings {
    namespace  Gui
    {
        class Label : public QLabel, public QtBindings::Base<Label>
        {
            Q_OBJECT
        public:
            Q_INVOKABLE Label(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
            Q_INVOKABLE Label(const Label &other);
            Q_INVOKABLE Label(const QString &text, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
            Q_INVOKABLE ~Label();
            Label &operator=(const Label &other);
        public slots:
            Qt::Alignment alignment() const;
            QWidget *buddy() const;
            void clear();
            bool hasScaledContents() const;
            bool hasSelectedText() const;
            int indent() const;
            int margin() const;
            QMovie *movie() const;
            bool openExternalLinks() const;
            QPicture picture() const;
            QPixmap pixmap() const;
            QString selectedText() const;
            int selectionStart() const;
            void setAlignment(Qt::Alignment);
            void setBuddy(QWidget *buddy);
            void setIndent(int);
            void setMargin(int);
            void setMovie(QMovie *movie);
            void setNum(double num);
            void setNum(int num);
            void setOpenExternalLinks(bool open);
            void setPicture(const QPicture &picture);
            void setPixmap(const QPixmap &);
            void setScaledContents(bool);
            void setSelection(int start, int length);
            void setText(const QString &);
            void setTextFormat(Qt::TextFormat);
            void setTextInteractionFlags(Qt::TextInteractionFlags flags);
            void setWordWrap(bool on);
            QString text() const;
            Qt::TextFormat textFormat() const;
            Qt::TextInteractionFlags textInteractionFlags() const;
            bool wordWrap() const;
            // QWidget
            bool close();
            void hide();
            void lower();
            void raise();
            void repaint();
            void setDisabled(bool disable);
            void setEnabled(bool);
            void setFocus();
            void setHidden(bool hidden);
            void setStyleSheet(const QString &styleSheet);
            virtual void setVisible(bool visible) override;
            void setWindowModified(bool);
            void setWindowTitle(const QString &);
            void show();
            void showFullScreen();
            void showMaximized();
            void showMinimized();
            void showNormal();
            void update();
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Gui::Label)
#endif //GUILABEL_H
