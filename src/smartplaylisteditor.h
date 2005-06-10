// (c) Pierpaolo Di Panfilo 2004
// (c) Alexandre Pereira de Oliveira 2005
// See COPYING file for licensing information

#ifndef SMARTPLAYLISTEDITOR_H
#define SMARTPLAYLISTEDITOR_H

#include <kdialogbase.h> //baseclass
#include <qdom.h>
#include <qhbox.h>       //baseclass
#include <qptrlist.h>    //definition required
#include <klineedit.h>   //inline function

class KComboBox;
class KIntSpinBox;
class KLineEdit;
class QCheckBox;
class QDateEdit;
class QLabel;
class QToolButton;
class QVGroupBox;

class CriteriaEditor;

class SmartPlaylistEditor : public KDialogBase
{
Q_OBJECT
    friend class CriteriaEditor;

    public:
        SmartPlaylistEditor( QString name, QWidget *parent, const char *name=0 );
        SmartPlaylistEditor( QWidget *parent, QDomElement xml, const char *name=0 );

        QDomElement result();

        QString name() const { return m_nameLineEdit->text(); }
        QString query() const { return m_query; }
        QString expandableQuery() const { return m_expandQuery; }

    public slots:
        void addCriteria();
        void addCriteria( QDomElement &xml );
        void removeCriteria( CriteriaEditor *criteria );

    private slots:
        void updateOrderTypes( int index );
        void buildQuery();

    private:
        void init(QString defaultName);
        void updateMatchWidgets();

        KLineEdit *m_nameLineEdit;

        QCheckBox *m_matchCheck;
        KComboBox *m_matchCombo;
        QLabel *m_matchLabel;

        QVGroupBox *m_criteriaGroupBox;
        //limit widgets
        QCheckBox *m_limitCheck;
        KIntSpinBox *m_limitSpin;
        //order by widgets
        QCheckBox *m_orderCheck;
        KComboBox *m_orderCombo;
        KComboBox *m_orderTypeCombo;
        //expand by
        QCheckBox *m_expandCheck;
        KComboBox *m_expandCombo;

        QString m_query;
        QString m_expandQuery;

        QPtrList<CriteriaEditor> m_criteriaEditorList;
};



class CriteriaEditor : public QHBox
{
Q_OBJECT
    public:
        CriteriaEditor( SmartPlaylistEditor *editor, QWidget *parent, QDomElement criteria = QDomElement() );
        ~CriteriaEditor();
        QString getSearchCriteria();
        void setSearchCriteria( const QString &str );
        QDomElement getDomSearchCriteria( QDomDocument &doc );
        void enableRemove( bool );

    private slots:
        void slotRemoveCriteria();
        void slotFieldSelected( int );
        void loadEditWidgets();

    private:
        enum ValueType { String, AutoCompletionString, Number, Year, Date };

        void loadCriteriaList( int valueType, QString condition = QString::null );
        int getValueType( int fieldIndex );

        SmartPlaylistEditor *m_playlistEditor;
        int m_currentValueType;
        QString m_lastCriteria;

        KComboBox *m_fieldCombo;
        KComboBox *m_criteriaCombo;
        QToolButton *m_addButton;
        QToolButton *m_removeButton;

        //editing widgets
        QHBox *m_editBox;
        KLineEdit *m_lineEdit;
        KComboBox *m_comboBox;
        KIntSpinBox *m_intSpinBox1;
        KIntSpinBox *m_intSpinBox2;
        QDateEdit *m_dateEdit1;
        QDateEdit *m_dateEdit2;
        KComboBox *m_dateCombo;
        QLabel *m_rangeLabel;
};

#endif
