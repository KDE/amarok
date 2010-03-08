/***************************************************************************
                                 likeback.h
                             -------------------
    begin                : unknown
    imported to LB svn   : 3 june, 2009
    copyright            : (C) 2006 by Sebastien Laout
                           (C) 2008-2009 by Valerio Pilo, Sjors Gielen
    email                : sjors@kmess.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIKEBACK_H
#define LIKEBACK_H

#include <QObject>

// Uncomment to enable debug output
// #define DEBUG_LIKEBACK

class KAboutData;
class KAction;
class KConfig;
class KActionCollection;

class LikeBackPrivate;

/**
 * @short System to Get Quick Feedback from Beta-Testers
 *
 * This system allows users to communicate theire liking of the application to its developers.
 * Thus, developers know what theire users prefer of theire applications, what should be enhanced, etc.
 *
 * Basically, how does it work?
 * Whenever the user notice something good he appreciate or something he do not like, do not understand, do not find polished...
 * he can send a few short words to the developers to tell them what he like or do not like. It is only two or three clicks away.
 * It is fast and efficient.
 *
 * This greatly lowers the communication barrier between the application developers and the application users.
 * It makes the developers understand and satisfy better the needs of the users.
 *
 * The LikeBack system has 5 components:
 * @li In the application: The comment dialog, where the user write a comment, select a type of comment, etc.
 * @li In the application: The KAction to plug in the Help menu. This action displays the comment dialog.
 * @li In the application: The button-bar, that floats bellow titlebar of every windows of the application, and let the user to quickly show the comment dialog.
 *                         The button-bar can be hidden.
 * @li On the server: A PHP script that collects every comments that users send. The LikeBack object should be configured to contact that server.
 * @li On the server: The developer interface. It lists every comments that were sent, let you sort them, add remarks to them, and mark them as fixed or another status.
 *
 * Here is an example of code to call to quickly setup LikeBack on the client:
 * @code
 *     // Instanciate the LikeBack system, and show the first-use information dialog if the button-bar is shown:
 *     LikeBack *likeBack = new LikeBack(LikeBack::AllButtons, LikeBack::isDevelopmentVersion(KGlobal::mainComponent().aboutData->version())); // Show button-bar only in beta-versions
 *     likeBack->setServer("myapp.kde.org", "/likeback/send.php");
 *     likeBack->setAcceptedLanguages(QStringList::split(";", "en;fr"), i18n("Please write in English or French."));
 *
 *     // Comment the following line once you are sure all your windows have a name:
 *     likeBack->setWindowNamesListing(LikeBack::WarnUnnamedWindows);
 *
 *     // This line should be called early in your KMainWindow constructor because it references actionCollection().
 *     // It should be called before createGUI() for the action to be plugged in the Help menu:
 *     likeBack->sendACommentAction(actionCollection());
 * @endcode
 *
 * @see Visit http://basket.kde.org/likeback.php for more information, screenshots, a tutorial, hints, return of experiences, and to download the server-side developer interface...
 * @author Sebastien Laout <slaout@linux62.org>
 */
class LikeBack : public QObject
{
  Q_OBJECT
  public:
	/**
	 * Ids of every LikeBack buttons the button-bar can have.
	 * The four first values are each individual buttons you can enable or not.
	 * The next ones are combinations: all buttons at once, and the default set of buttons (Like, Dislike).
	 * Those values are used in the constructor, to set the allowed type of comments, and when triggering the comment dialog, to set the default checked type.
	 * @see The LikeBack constructor and execCommentDialog().
	 */
	enum Button {
		Like           = 0x01,                           /// The user select that option to report a positive experience he got with the application.
		Dislike        = 0x02,                           /// The user select that option to report a frustrating experience he got with the application.
		Bug            = 0x04,                           /// The user select that option to report a bug in the application.
		Feature        = 0x10,                           /// The user select that option to ask for a new feature he desire.
		                                                 /// If not enabled, the user is explicitely informed she cannot ask for new features.
		AllButtons     = Like | Dislike | Bug | Feature, /// Usable in the constructor to enable every posible buttons.
		DefaultButtons = Like | Dislike                  /// Usable in the constructor to enable only the recommended default set of buttons.
	};

	/**
	 * Flags letting LikeBack print out name and path of each window you show during execution, for debugging purpose.
	 * @see The method setWindowNamesListing() explains how to use those values.
	 */
	enum WindowListing {
		NoListing = 0,          /// Do not print out any window name. For release time.
		WarnUnnamedWindows = 1, /// Each time the user option a window, print out a message if the window is unnamed. For development needs, to check windows.
		AllWindows = 2          /// Print out the window hierarchy of each opened windows during execution. For development needs, to check every windows have an understandable name.
	};

	/**
	 * You only need to call the constructor once, typically in main.cpp.
	 * Even if you do not show the button-bar by default, you should instanciate LikeBack,
	 * to include its action in the Help menu of your application, to let the users send comments or activate the bar.
	 * @param buttons          The types of comments you want to get. Determine which radio-buttons are shown in the comment dialog,
	 *                         and which ones are displayed in the button-bar. Default buttons do not show the Bug and Feature buttons because you are
	 *                         likely to already have a way to get bug and feature reports (most of the time, it is a bugs.kde.org account).
	 *                         If you do not have that, then use the value LikeBack::AllButtons to show every possible buttons.
	 * @param showBarByDefault Determines if the floating button-bar should also be shown, in addition to the action in the Help menu.
	 *                         Advise: to avoid getting too much noise, enable it only if it is a small application or a development release.
	 *                         Notes: This is only a default value, the user will be able to enable or disabled the bar afterward.
	 *                         The button-bar display is stored by version. On a new version, your default value will take effect again.
	 *                         This allow you to disable the button-bar once the version is stable enought to be released as final.
	 * @param config           Set the configuration file where to store the user email address and if the button-bar should be shown.
	 *                         By default (null), the KApplication configuration object is used.
	 * @param aboutData        Set the KAboutData instance used to get the application name and version. By default (null), the KApplication about data object is used.
	 *                         The application name is only used in the first-use information message.
	 *                         The version is used to store the button-bar visibility per version (can be shown in a development version but not in a final one...)
	 *                         and to send with the comment, so you can filter per version and know if a comment refers the latest version of the application or not.
	 */
	explicit LikeBack(Button buttons = DefaultButtons, bool showBarByDefault = false,  KConfig *config = 0, const KAboutData *aboutData = 0 );

	/**
	 * Destructor.
	 * Also hide the button-bar, if it was shown.
	 * Be careful, the KAction is deleted. Do not use it afterward, and take care to unplug it before destroying this LikeBack instance.
	 */
	~LikeBack();

	/**
	 * This method is interesting while setting up the system for the first time.
	 * LikeBack send the current window name (and hierarchy) with the comment. This allows you to put the comments in theire context.
	 * So, of course, you are encouraged to give a name to your windows. It is done in the constructor of the widgets.
	 * This method allows to output the name of the current window to the standard output.
	 * So you can use the application, open all the windows, and when you see a warning, you know which window you should assign a name.
	 * @see The WindowListing flags for an enumeration and explaining of every possibilities.
	 * @note If you do not name your windows, the name of the classes will be sent. So it is not that grave.
	 */
	void setWindowNamesListing(WindowListing windowListing);

	/**
	 * @returns The window listing flag.
	 * @see setWindowNamesListing()
	 */
	WindowListing windowNamesListing();

	/**
	 * By default, only English comments are accepted. The user is informed she must write in this language by a sentence placed in the comment dialog.
	 * If you have people talking other languages in your development team, it can be interesting to call this method to define the accepted locales (languages),
	 * and provide a message to inform users. The developer interface on the server let developers view comments in theire locale.
	 * Note that no verification is done to check if the user used the right language, it would be impossible.
	 * The list of locales is there to make it possible to NOT show the message for users of the accepted languages.
	 * For instance, if you accept only English and French, and that the application run in a French environment,
	 * it is likely the user is French and will write comments using French. Telling him he should write in French is unnecessary and redundant.
	 * Passing an empty list and an empty string to the method will make LikeBack display the default message telling the user only English is accepted.
	 * Example of call you can quickly copy, paste and adapt:
	 * @code
	 *     likeBack->setAcceptedLanguages(QStringList::split(";", "en;fr"), i18n("Please write in English or French."));
	 * @endcode
	 * @note During tests, if you do not see the sentence, it is because you are running the application with an "accepted language": do not be surprised ;-)
	 * @param locales The list of locales where the message does not need to be shown. See TODO TODO for a list of available locales for you to choose.
	 */
	void setAcceptedLanguages(const QStringList &locales);

	/**
	 * @returns The list of accepted locales for the user to write comments.
	 * @see setAcceptedLanguages()
	 */
	QStringList acceptedLocales();

	/**
	 * Set the path where LikeBack should send every comments.
	 * It is composed of the server host name, the path to the PHP script used to send comments, and optionnaly a port number if it is not 80.
	 * This call is mandatory for LikeBack to work.
	 * @param hostName   The server host name to contact when sending comments. For instance "myapp.kde.org".
	 * @param remotePath The path to the send script on the server. For instance, "/likeback/send.php".
	 * @param hostPort   Optionnal port used to contact the server using the HTTP protocol. By default, it is port 80.
	 */
	void setServer(const QString &hostName, const QString &remotePath, quint16 hostPort = 80);

	/**
	 * @returns The server host name to contact when sending comments.
	 * @see setServer()
	 */
	QString hostName();

	/**
	 * @returns The path to the send script on the server.
	 * @see setServer()
	 */
	QString remotePath();

	/**
	 * @returns The port used to contact the server using the HTTP protocol.
	 * @see setServer()
	 */
	quint16 hostPort();

  /**
   * Create the menu actions.
   * They will be added to the given actionCollection(), so if you use KXmlGuiWindow,
   * you can add them via XML in the Help menu (or any other!) like this:
   * @code
   * <Menu name="help">
   *   <Action name="likeBackSendComment" />
   *   <Action name="likeBackShowIcons" />
   * </Menu>
   * @endcode
   *
   * This will add the actions above "Report a Bug", if it's visible, and below "What's This?".
   * If you do not have a Bugzilla account, LikeBack is a good way for your small application to get bug reports: remove "Report a Bug".
   * For more information about how to configure LikeBack depending on your application size and settings, see the constructor documentation.
   *
   * If you use KMainWindow, add them like this:
   * @code
   * KActionCollection *collection = new KActionCollection();
   * collection->addAssociatedWidget( this );
   * likeback->createActions( collection );
   * menu()->addAction( collection->action( "likeBackSendComment" );
   * menu()->addAction( collection->action( "likeBackShowIcons" );
   * @endcode
   *
	 */
	void createActions( KActionCollection *parent = 0 );

	/**
	 * @returns The path of the currently active window. Each windows are separated with "~~".
	 * Normally, you should not need to call this method since it is used to send the window path.
	 * But if you call execCommentDialog(), you could need to use it.
	 */
	static QString activeWindowPath();

	/**
	 * @returns The combination of buttons that are shown in the comment dialog and the button-bar.
	 */
	Button buttons();

	/**
	 * @returns true if the button-bar is currently enabled. Ie, if it has been re-enabled as many times as it has been disabled.
	 * @see The method disableBar() for more information on how enabling/disabling works.
	 */
	bool enabledBar();

  public slots:

	/**
	 * Temporarily disable the button-bar: it is hiden from the screen if it was shown.
	 * Does not affect anything if the user has not choosen to show the button-bar.
	 * @note Calls to enableBar() and disableBar() are ref-counted.
	 * This means that the number of times disableBar() is called is memorized,
	 * and enableBar() will only have effect after it has been called as many times as disableBar() was called before.
	 * So, make sure to always call enableBar() the same number of times ou called disableBar().
	 * And please make sure to ALWAYS call disableBar() BEFORE enableBar().
	 * In the counter-case, another code could call disableBar() and EXCPECT the bar to be disabled. But it will not, because its call only canceled yours.
	 * @note Sometimes, you will absolutely need to call enableBar() before disableBar().
	 * For instance, MyWindow::show() calls enableBar() and MyWindow::hide() calls disableBar().
	 * This is the trick used to show the LikeBack button-bar of a Kontact plugin only when the main widget of that plugin is active.
	 * In this case, call disableBar() at the begin of your program, so the disable count will never be negative.
	 * @note If the bar is enabled, it does not mean the bar is shown. For that, the developer (using showBarByDefault in the construcor)
	 *       or the user (by checking the checkbox in the comment dialog) have to explicitely show the bar.
	 */
	void disableBar();

	/**
	 * Re-enable the button-bar one time.
	 * @see The method disableBar() for more information on how enabling/disabling works.
	 */
	void enableBar();

	/**
	 * Show the first-use information dialog telling the user the meaning of the LikeBack system and giving examples of every comment types.
	 */
	void showInformationMessage();

	/**
	 * Popup the comment dialog.
	 * With no parameter, it popups in the default configuration: the first type is checked, empty message, current window path, and empty context.
	 * You can use the following parameters to customize how it should appears:
	 * @param type           Which radiobutton should be checked when poping up. AllButton, the default value, means the first available type will be checked.
	 * @param initialComment The text to put in the comment text area. Allows you to popup the dialog in some special circumstances,
	 *                       like to let the user report an internal error by populating the comment area with technical details useful for you to debug.
	 * @param windowPath     The window path to send with the comment. If empty (the default), the current window path is took.
	 *                       Separate window names with "~~". For instance "MainWindow~~NewFile~~FileOpen".
	 *                       If you popup the dialog after an error occurred, you can put the error name in that field (if the window path has no sense in that context).
	 *                       When the dialog is popuped up from the sendACommentAction() KAction, this value is "HelpMenu", because there is no way to know if the user
	 *                       is commenting a thing he found/thinked about in a sub-dialog.
	 * @param context        Not used for the moment. Will allow more fine-grained application status report.
	 */
	void execCommentDialog(Button type = AllButtons, const QString &initialComment = "", const QString &windowPath = "", const QString &context = "");

  private:
	LikeBackPrivate *d;

	/**
	 * Get the user email address from KControl.
	 */
	void fetchUserEmail();

  private slots:
	/**
	 * Slot triggered by the "Help -> Send a Comment to Developers" KAction.
	 * It popups the comment dialog, and set the window path to "HelpMenuAction",
	 * because current window path has no meaning in that case.
	 */
	void execCommentDialogFromHelp();

  public slots:

  /**
   * Explicitely set if the floating button-bar should be shown or not.
   * Tehorically, this choice should only be left to the user,
   * and to the developers for the default value, already provided in the constructor.
   */
  void setUserWantsToShowBar(bool showBar);

  public:

	/**
	 * @returns true if the user has enabled the LikeBack bar for this version.
	 */
	bool userWantsToShowBar();

	/**
	 * @returns A pointer to the KAboutData used to determin the application name and version.
	 * @see The LikeBack constructor for more information.
	 */
	const KAboutData *aboutData();

	/**
	 * @returns A pointer to the KConfig group to store user configuration (email address, if the button-bar should be shown).
	 * @see The LikeBack constructor for more information.
	 */
	KConfig *config();

	/**
	 * During the first comment sending, the user is invited to enter his email address for the developers to be able to contact him back.
	 * He is only asked once, or he can set or change it by using the bottom-left button in the comment dialog.
	 * @returns true if the user has already configured his email address.
	 */
	bool emailAddressAlreadyProvided();

	/**
	 * @returns The email user address, or ask it to the user if he have not provided or ignored it.
	 * @returns An empty string if the user cancelled the request dialog.
	 */
	QString emailAddress();

	/**
	 * Define or re-define the user email address.
   * LikeBack will not ask it again to the user, unless you set @p userProvided to false.
   * Then, this call can be considered as setting the default email address, that the user should confirm later.
	 */
	void setEmailAddress( const QString &address, bool userProvided = false );

	/**
	 * @returns true if @p version is an Alpha, Beta, RC, SVN or CVS version.
	 * You can use this static method in the constructor to enable the button-bar by default only during beta-releases.
	 */
	static bool isDevelopmentVersion(const QString &version);

  bool isLikeActive() const;
  bool isDislikeActive() const;
  bool isBugActive() const;
  bool isFeatureActive() const;
};

#endif // LIKEBACK_H
