Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.gui" );
Importer.loadQtBinding( "qt.uitools" );

function globalScopeEval( inputCode )
{
  try
  {
      result = eval( inputCode ) + " ";
  }
  catch( error )
  {
      result = error + " ";
  }
  return result;
}

function ScriptConsoleMainWindow()
{
  QMainWindow.call( this, null );

  var mainWidget = new QWidget( this );
  this.historyList = new QListWidget( mainWidget );
  this.historyList.sizeHint = new QSize(900, 600);
  this.historyList.size = new QSize(900, 600);
  this.historyList.verticalScrollMode = QAbstractItemView.ScrollPerPixel;
  this.inputLine = new QTextEdit( mainWidget );
  this.executeButton = new QPushButton( mainWidget );
  this.commandArray = [];
  this.commandPos = -1;
  this.executeButton.clicked.connect( this, this.executeLine );
  this.executeButton.text = "Execute Code";
  this.windowTitle = "Amarok Script Console";
  //the following line doesn't work for some reason:
  //executeButton.shortcut = new QKeySequence( "CTRL+Return" );

  var executeShortcut = new QShortcut( this );
  executeShortcut.key =  new QKeySequence( "CTRL+Return" );
  executeShortcut.activated.connect( this, this.executeLine );

  var backHistoryShortcut = new QShortcut( this );
  backHistoryShortcut.key = new QKeySequence( QKeySequence.StandardKey( QKeySequence.MoveToPreviousPage ) );
  backHistoryShortcut.activated.connect( this, this.backHistory );

  var forwardHistoryShortcut = new QShortcut( this );
  forwardHistoryShortcut.key = new QKeySequence( QKeySequence.StandardKey( QKeySequence.MoveToNextPage ) );
  forwardHistoryShortcut.activated.connect( this, this.forwardHistory );

  var explanationItem = new QListWidgetItem("The Amarok Script Console allows you to easily execute JavaScript with access to all functions\nand methods you would have in an Amarok script.\nInformation on scripting for Amarok is available at:\nhttp://community.kde.org/Amarok/Development#Scripting\nExecute code: CTRL-Enter\nBack in code history: Page Up\nForward in code history: Page Down");
  //explanationItem.setForeground( new QBrush( new QColor(Qt.darkGray) ) );
  explanationItem.setFlags( !Qt.ItemIsSelectable );
  this.historyList.addItem( explanationItem );

  var layout = new QGridLayout( mainWidget );
  layout.addWidget( this.historyList, 0, 0, 1, 2 );
  layout.addWidget( this.inputLine, 1, 0, 3, 2 );
  layout.addWidget( this.executeButton, 4, 1 );
  mainWidget.setLayout( layout );
  this.setCentralWidget( mainWidget );
  this.resize(750, 400);
  this.show();
}

ScriptConsoleMainWindow.prototype = new QMainWindow();

ScriptConsoleMainWindow.prototype.executeLine = function()
{
    command = new QListWidgetItem( this.inputLine.plainText );
    this.commandArray.unshift( this.inputLine.plainText );
    boldFont = new QFont();
    boldFont.setBold( true );
    command.setFont( boldFont );
    result = globalScopeEval.call( null, this.inputLine.plainText );
    resultsRow = new QListWidgetItem( result );
    this.historyList.addItem( command  );
    this.historyList.addItem( resultsRow );
    this.historyList.setCurrentItem( resultsRow );
    this.inputLine.plainText = "";
    this.commandPos = -1;
}

ScriptConsoleMainWindow.prototype.backHistory = function()
{
  if( this.commandArray.length > ( this.commandPos + 1 ) )
  {
      this.commandPos++;
  }
  if( this.commandArray.length > 0 )
  {
    this.inputLine.plainText = this.commandArray[ this.commandPos ];
  }
  Amarok.debug( "back, " + this.commandArray[ this.commandPos ] );
}

ScriptConsoleMainWindow.prototype.forwardHistory = function()
{
  if( this.commandArray.length > 0 )
  {
    if( this.commandPos > 0 )
    {
      this.commandPos--;
    }
    this.inputLine.plainText = this.commandArray[ this.commandPos ];
  }
  Amarok.debug( "forward, " + this.commandPos + ": " + this.commandArray[ this.commandPos ] );
}

scriptConsoleMainWindow = new ScriptConsoleMainWindow();
