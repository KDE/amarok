Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.gui" );
Importer.loadQtBinding( "qt.uitools" );

function executeLine() 
{
  try {
    command = new QListWidgetItem( inputLine.plainText + ":");
    boldFont = new QFont();
    boldFont.setBold( true );
    command.setFont( boldFont );

    results = new QListWidgetItem(  eval( inputLine.plainText ) + " " );
    historyList.addItem( command  );
    historyList.addItem( results );
  }
  catch( error ) {
    print( error );
  }
}

var mainWindow = new QMainWindow();
var mainWidget = new QWidget( mainWindow );
var historyList = new QListWidget( mainWidget );
var inputLine = new QTextEdit( mainWidget );
var executeButton = new QPushButton( mainWidget );
executeButton.clicked.connect( this, this.executeLine );

var layout = new QGridLayout( mainWidget );
layout.addWidget( this.historyList, 0, 0, 1, 2 );
layout.addWidget( this.inputLine, 1, 0, 3, 2 );
layout.addWidget( this.executeButton, 4, 1 );
mainWidget.setLayout( layout );
mainWindow.setCentralWidget( mainWidget );
mainWindow.show();