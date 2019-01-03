/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include "amarok_export.h"
#include "ComboBox.h"

#include <QWidget>
#include <QTimer>

class QToolBar;
class QPushButton;
// A Custom Widget that can be used globally to implement
// searching a treeview.

class AMAROK_EXPORT SearchWidget : public QWidget
{
    Q_OBJECT
    public:
        /** Creates a search widget.
         * @param parent The parent widget
         * @param advanced If true generates a button that opens a edit filter dialog.
        */
        explicit SearchWidget( QWidget *parent = Q_NULLPTR, bool advanced = true );

        QString currentText() const { return m_sw->currentText(); }
        Amarok::ComboBox *comboBox() { return m_sw; }

        /**
         * Sets the timout length after which the filterChanged() signal will be fired automatically.
         * @param newTimeout timeout in milliseconds.
         */
        void setTimeout( quint16 newTimeout );

        QToolBar* toolBar();

        void showAdvancedButton( bool show );

        /**
         * Sets the string that will be visible when the ComboBox's edit text is empty.
         * @param message the string that will be visible when the ComboBox's edit text is empty.
         */
        void setClickMessage( const QString &message );

    public Q_SLOTS:
        void setSearchString( const QString &searchString = QString() );
        void emptySearchString() { setSearchString( QString() ); }

        /**
         * Tells the widget that a search operation has started. As a consequence the
         * "search" icon changes to a progress animation.
         *
         * Note: You can call this slot several times if you have several search operations
         * simultaneously. The widget has an internal counter to track them.
         */
        void searchStarted();

        /**
         * Tells the widget that a search operation has ended. As a consequence the
         * progress animation will be changed back to a search icon iff no other search
         * operation is in progress.
         */
        void searchEnded();

    Q_SIGNALS:
        /**
         * Emitted when the filter value was changed.
         * Note: This signal might be delayed while the user is typing
         */
        void filterChanged( const QString &filter );

        /**
         * Emitted when the user hits enter after after typing in the filter. It is
         * guaranteed that filterChanged() with the current text was emitted previously.
         */
        void returnPressed();

    private Q_SLOTS:
        void resetFilterTimeout();
        void filterNow();
        void advanceFocus();

        void addCompletion( const QString &text );
        void nextAnimationTick();
        void onComboItemActivated( int index );
        void slotShowFilterEditor();
        void slotFilterEditorFinished( int result );

    private:
        Amarok::ComboBox *m_sw;
        QAction          *m_filterAction;
        QToolBar         *m_toolBar;
        QTimer            m_animationTimer;
        QTimer            m_filterTimer;
        quint16           m_timeout;
        bool              m_currentFrame;
        unsigned int      m_runningSearches;

        // required to save/restore line edit status
        QString m_text;
        int     m_cursorPosition;
        bool    m_hasSelectedText;
        int     m_selectionStart;
        int     m_selectionLength;

        /**
         * Restore the status of the internal line edit (text, selection, cursor position).
         * Crete a snapshot with saveLineEditStatus() before using this method.
         * Required to keep user changes during animations.
         */
        void restoreLineEditStatus();

        /**
         * Save the status of the internal line edit (text, selection, cursor position) to
         * restore it later with restoreLineEditStatus().
         * Required to keep user changes during animations.
         */
        void saveLineEditStatus();
};

#endif
