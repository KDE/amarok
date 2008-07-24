/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

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

        signals:
            void widgetChanged( QWidget* );

        protected:
            void resizeEvent( QResizeEvent* );

        private:
            void paintEvent( QPaintEvent* );

    };

    class BiasAddWidget : public BiasBoxWidget
    {
        Q_OBJECT

        public:
            BiasAddWidget( QWidget* parent = 0 );
            
        private slots:
            void addBias();

        signals:
            void addBias( Dynamic::Bias* );
            void clicked();

        protected:
            virtual void mouseReleaseEvent( QMouseEvent* );

        private:
            KToolBar*    m_addToolbar;
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

        protected:
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
            void valueChanged( const QString& );
            void valueChanged( const QTime& );

        private:
            void syncControlsToBias();
            void syncBiasToControls();

            void popuplateFieldSection();
            void setValueSection( QWidget* );

            void makeCompareSelection( QWidget* parent );

            void makeGenericComboSelection( bool editable, QueryMaker* populateQuery );

            void makeArtistSelection();
            void makeComposerSelection();
            void makeAlbumSelection();
            void makeTitleSelection();
            void makeGenreSelection();
            void makeYearSelection();
            void makePlaycountSelection();
            void makeLengthSelection();

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
    };


}

Q_DECLARE_METATYPE( PlaylistBrowserNS::BiasBoxWidget* )

#endif

