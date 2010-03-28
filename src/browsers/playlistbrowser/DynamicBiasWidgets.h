/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_DYNAMICBIASWIDGETS_H
#define AMAROK_DYNAMICBIASWIDGETS_H

#include "Bias.h"

#include <QWidget>

class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QToolButton;
class KComboBox;
class KToolBar;
class KVBox;

namespace Amarok
{
    class Slider;
}

namespace PlaylistBrowserNS
{
    class BiasBoxWidget : public QWidget
    {
        Q_OBJECT

        public:
            BiasBoxWidget(QWidget* parent = 0 );
            virtual ~BiasBoxWidget() {}

            bool alternate() { return m_alternate; }
            void setAlternate( bool alternate ) { m_alternate = alternate; }
            void toggleAlternate() { m_alternate = !m_alternate; }

        signals:
            void widgetChanged( QWidget* );

        protected:
            void resizeEvent( QResizeEvent* );

        private:
            void paintEvent( QPaintEvent* );
            bool m_alternate;

    };

    class BiasAddWidget : public BiasBoxWidget
    {
        Q_OBJECT

        public:
            BiasAddWidget( const QString& caption, const QString& description, QWidget* parent = 0 );
            
        private slots:
            void slotClicked();

        signals:
            void addBias();
            void clicked();

        protected:
            virtual void mousePressEvent( QMouseEvent* event );

        private:
           // KToolBar*    m_addToolbar;
            QToolButton* m_addButton;
            QLabel*      m_addLabel;
    };


    class BiasWidget : public BiasBoxWidget
    {
        Q_OBJECT

        public:
            explicit BiasWidget( Dynamic::Bias*, QWidget* parent = 0 );

        signals:
            void biasRemoved( Dynamic::Bias* );
            void biasChanged( Dynamic::Bias* );

        private slots:
            void biasRemoved();

        protected: //protected data members make Mike cry :'(
            KVBox* m_mainLayout;
            Dynamic::Bias* m_bias;

        private:
            KToolBar* m_removeToolbar;
            QToolButton* m_removeButton;
    };

    class BiasGlobalWidget : public BiasWidget
    {
        Q_OBJECT

        public:
            explicit BiasGlobalWidget( Dynamic::GlobalBias* bias, QWidget* parent = 0 );

        private slots:
            void weightChanged( int );
            void fieldChanged( int );
            void compareChanged( int );
            void valueChanged( int );
            void valueChanged( const QString& );
            void valueDateChanged();
            void valueChanged( const QTime& );

            void populateComboBox( QString collectionId, QStringList );
            void comboBoxPopulated();

        private:
            void syncControlsToBias();
            void syncBiasToControls();

            void popuplateFieldSection();
            void setValueSelection( QWidget* );

            void makeCompareSelection( QWidget* parent );

            void makeGenericComboSelection( bool editable, Collections::QueryMaker* populateQuery );

            void makeArtistSelection();
            void makeComposerSelection();
            void makeAlbumSelection();
            void makeTitleSelection();
            void makeGenreSelection();
            void makeGenericNumberSelection( int min, int max, int def );
            void makePlaycountSelection();
            void makeRatingSelection();
            void makeLengthSelection();
            void makeDateTimeSelection();
            void makeFilenameSelection();

            QFrame*      m_controlFrame;
            QGridLayout* m_controlLayout;

            Amarok::Slider* m_weightSelection;
            QLabel*         m_weightLabel;
            QLabel*         m_withLabel;
            KComboBox*      m_fieldSelection;
            QWidget*        m_valueSelection;
            KComboBox*      m_compareSelection;

            Dynamic::GlobalBias* m_gbias;
            XmlQueryReader::Filter m_filter;

            QMap< Collections::QueryMaker*, KComboBox* > m_runningQueries;
    };

    class BiasNormalWidget : public BiasWidget
    {
        Q_OBJECT

        public:
            explicit BiasNormalWidget( Dynamic::NormalBias*, QWidget* parent = 0 );

        private slots:
            void fieldChanged( int );
            void valueChanged( int );
            void valueDateChanged();
            void valueChanged( const QTime& );
            void scaleChanged( int );

        private:
            void popuplateFieldSection();
            void syncControlsToBias();
            void makeGenericNumberSelection( int min, int max );
            void makeRatingSelection();
            void makeScoreSelection();
            void makeLengthSelection();
            void makeDateTimeSelection();
            void setValueSelection( QWidget* w );

            QFrame*      m_controlFrame;
            QGridLayout* m_controlLayout;

            KComboBox* m_fieldSelection;
            QLabel*  m_withLabel;
            QWidget* m_valueSelection;
            Amarok::Slider* m_scaleSelection;
            QLabel* m_scaleLabel;

            Dynamic::NormalBias* m_nbias;
    };



}

Q_DECLARE_METATYPE( PlaylistBrowserNS::BiasBoxWidget* )

#endif

