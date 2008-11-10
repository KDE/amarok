/*
 *   Copyright 2006-2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
 *   Copyright 2008 by Ménard Alexis <darktears31@gmail.com>

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_APPLET_H
#define PLASMA_APPLET_H

#include <QtGui/QGraphicsItem>
#include <QtGui/QWidget>
#include <QtGui/QGraphicsWidget>

#include <KDE/KConfigGroup>
#include <KDE/KGenericFactory>
#include <KDE/KPluginInfo>
#include <KDE/KShortcut>

#include <plasma/configloader.h>
#include <plasma/packagestructure.h>
#include <plasma/plasma.h>
#include <plasma/animator.h>
#include <plasma/version.h>

class KConfigDialog;
class QGraphicsView;
class KActionCollection;

namespace Plasma
{

class AppletPrivate;
class Containment;
class Context;
class DataEngine;
class Extender;
class ExtenderItem;
class Package;

/**
 * @class Applet plasma/applet.h <Plasma/Applet>
 *
 * @short The base Applet class
 *
 * Applet provides several important roles for add-ons widgets in Plasma.
 *
 * First, it is the base class for the plugin system and therefore is the
 * interface to applets for host applications. It also handles the life time
 * management of data engines (e.g. all data engines accessed via
 * Applet::dataEngine(const QString&) are properly deref'd on Applet
 * destruction), background painting (allowing for consistent and complex
 * look and feel in just one line of code for applets), loading and starting
 * of scripting support for each applet, providing access to the associated
 * plasmoid package (if any) and access to configuration data.
 *
 * See techbase.kde.org for tutorial on writing Applets using this class.
 */
class PLASMA_EXPORT Applet : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(bool hasConfigurationInterface READ hasConfigurationInterface)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString category READ category)
    Q_PROPERTY(ImmutabilityType immutability READ immutability WRITE setImmutability)
    Q_PROPERTY(bool hasFailedToLaunch READ hasFailedToLaunch WRITE setFailedToLaunch)
    Q_PROPERTY(bool configurationRequired READ configurationRequired WRITE setConfigurationRequired)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(bool shouldConserveResources READ shouldConserveResources)

    public:
        typedef QList<Applet*> List;
        typedef QHash<QString, Applet*> Dict;

        /**
         * Description on how draw a background for the applet
         */
        enum BackgroundHint {
            NoBackground = 0,         /**< Not drawing a background under the
                                          applet, the applet has its own implementation */
            StandardBackground = 1,   /**< The standard background from the theme is drawn */
            TranslucentBackground = 2, /**< An alternate version of the background is drawn,
                                          usually more translucent */
            DefaultBackground = StandardBackground /**< Default settings:
                                          both standard background */
        };
        Q_DECLARE_FLAGS(BackgroundHints, BackgroundHint)

        ~Applet();

        /**
         * @return a package structure representing a Theme
         */
        static PackageStructure::Ptr packageStructure();

        /**
         * @return the id of this applet
         */
        uint id() const;

        /**
        * Returns the KConfigGroup to access the applets configuration.
        *
        * This config object will write to an instance
        * specific config file named \<appletname\>\<instanceid\>rc
        * in the Plasma appdata directory.
        **/
        KConfigGroup config() const;

        /**
         * Returns a config group with the name provided. This ensures
         * that the group name is properly namespaced to avoid collision
         * with other applets that may be sharing this config file
         *
         * @param group the name of the group to access
         **/
        KConfigGroup config(const QString &group) const;

        /**
         * Saves state information about this applet that will
         * be accessed when next instantiated in the restore(KConfigGroup&) method.
         *
         * This method does not need to be reimplmented by Applet
         * subclasses, but can be useful for Applet specializations
         * (such as Containment) to do so.
         *
         * Applet subclasses may instead want to reimplement saveState().
         **/
        virtual void save(KConfigGroup &group) const;

        /**
         * Restores state information about this applet saved previously
         * in save(KConfigGroup&).
         *
         * This method does not need to be reimplmented by Applet
         * subclasses, but can be useful for Applet specializations
         * (such as Containment) to do so.
         **/
        virtual void restore(KConfigGroup &group);

        /**
         * Returns a KConfigGroup object to be shared by all applets of this
         * type.
         *
         * This config object will write to an applet-specific config object
         * named plasma_\<appletname\>rc in the local config directory.
         */
        KConfigGroup globalConfig() const;

        /**
         * Returns the config skeleton object from this applet's package,
         * if any.
         *
         * @return config skeleton object, or 0 if none
         **/
        ConfigLoader *configScheme() const;

        /**
         * Loads the given DataEngine
         *
         * Tries to load the data engine given by @p name.  Each engine is
         * only loaded once, and that instance is re-used on all subsequent
         * requests.
         *
         * If the data engine was not found, an invalid data engine is returned
         * (see DataEngine::isValid()).
         *
         * Note that you should <em>not</em> delete the returned engine.
         *
         * @param name Name of the data engine to load
         * @return pointer to the data engine if it was loaded,
         *         or an invalid data engine if the requested engine
         *         could not be loaded
         */
        Q_INVOKABLE DataEngine *dataEngine(const QString &name) const;

        /**
         * Accessor for the associated Package object if any.
         * Generally, only Plasmoids come in a Package.
         *
         * @return the Package object, or 0 if none
         **/
        const Package *package() const;

        /**
         * Returns the view this widget is visible on, or 0 if none can be found.
         * @warning do NOT assume this will always return a view!
         * a null view probably means that either plasma isn't finished loading, or your applet is
         * on an activity that's not being shown anywhere.
         */
        QGraphicsView *view() const;

        /**
         * Maps a QRect from a view's coordinates to local coordinates.
         * @param view the view from which rect should be mapped
         * @param rect the rect to be mapped
         */
        QRectF mapFromView(const QGraphicsView *view, const QRect &rect) const;

        /**
         * Maps a QRectF from local coordinates to a view's coordinates.
         * @param view the view to which rect should be mapped
         * @param rect the rect to be mapped
         */
        QRect mapToView(const QGraphicsView *view, const QRectF &rect) const;

        /**
         * Reccomended position for a popup window like a menu or a tooltip
         * given its size
         * @param s size of the popup
         * @returns reccomended position
         */
        QPoint popupPosition(const QSize &s) const;

        /**
         * Called when any of the geometry constraints have been updated.
         * This method calls constraintsEvent, which may be reimplemented,
         * once the Applet has been prepared for updating the constraints.
         *
         * @param constraints the type of constraints that were updated
         */
        void updateConstraints(Plasma::Constraints constraints = Plasma::AllConstraints);

        /**
         * Returns the current form factor the applet is being displayed in.
         *
         * @see Plasma::FormFactor
         */
        virtual FormFactor formFactor() const;

        /**
         * Returns the location of the scene which is displaying applet.
         *
         * @see Plasma::Location
         */
        virtual Location location() const;

        /**
         * Returns the workspace context which the applet is operating in
         */
        Context *context() const;

        /**
         * @return the preferred aspect ratio mode for placement and resizing
         */
        Plasma::AspectRatioMode aspectRatioMode() const;

        /**
         * Sets the preferred aspect ratio mode for placement and resizing
         */
        void setAspectRatioMode(Plasma::AspectRatioMode);

        /**
         * Returns a list of all known applets.
         *
         * @param category Only applets matchin this category will be returned.
         *                 Useful in conjunction with knownCategories.
         *                 If "Misc" is passed in, then applets without a
         *                 Categories= entry are also returned.
         *                 If an empty string is passed in, all applets are
         *                 returned.
         * @param parentApp the application to filter applets on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin info.
         *                  The default value of QString() will result in a
         *                  list containing only applets not specifically
         *                  registered to an application.
         * @return list of applets
         **/
        static KPluginInfo::List listAppletInfo(const QString &category = QString(),
                                              const QString &parentApp = QString());

        /**
         * Returns a list of all known applets associated with a certain mimetype.
         *
         * @return list of applets
         **/
        static KPluginInfo::List listAppletInfoForMimetype(const QString &mimetype);

        /**
         * Returns a list of all the categories used by
         * installed applets.
         *
         * @param parentApp the application to filter applets on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin info.
         *                  The default value of QString() will result in a
         *                  list containing only applets not specifically
         *                  registered to an application.
         * @return list of categories
         * @param visibleOnly true if it should only return applets that are marked as visible
         */
        static QStringList listCategories(const QString &parentApp = QString(),
                                          bool visibleOnly = true);

        /**
         * Attempts to load an applet
         *
         * Returns a pointer to the applet if successful.
         * The caller takes responsibility for the applet, including
         * deleting it when no longer needed.
         *
         * @param name the plugin name, as returned by KPluginInfo::pluginName()
         * @param appletId unique ID to assign the applet, or zero to have one
         *        assigned automatically.
         * @param args to send the applet extra arguments
         * @return a pointer to the loaded applet, or 0 on load failure
         **/
        static Applet *load(const QString &name, uint appletId = 0,
                            const QVariantList &args = QVariantList());

        /**
         * Attempts to load an applet
         *
         * Returns a pointer to the applet if successful.
         * The caller takes responsibility for the applet, including
         * deleting it when no longer needed.
         *
         * @param info KPluginInfo object for the desired applet
         * @param appletId unique ID to assign the applet, or zero to have one
         *        assigned automatically.
         * @param args to send the applet extra arguments
         * @return a pointer to the loaded applet, or 0 on load failure
         **/
        static Applet *load(const KPluginInfo &info, uint appletId = 0,
                            const QVariantList &args = QVariantList());

        /**
         * Get the category of the given applet
         *
         * @param applet a KPluginInfo object for the applet
         */
        static QString category(const KPluginInfo &applet);

        /**
         * Get the category of the given applet
         *
         * @param appletName the name of the applet
         */
        static QString category(const QString &appletName);

        /**
         * This method is called when the interface should be painted.
         *
         * @param painter the QPainter to use to do the paintiner
         * @param option the style options object
         * @param contentsRect the rect to paint within; automatically adjusted for
         *                     the background, if any
         **/
        virtual void paintInterface(QPainter *painter,
                                    const QStyleOptionGraphicsItem *option,
                                    const QRect &contentsRect);

        /**
         * Returns the user-visible name for the applet, as specified in the
         * .desktop file.
         *
         * @return the user-visible name for the applet.
         **/
        QString name() const;

        /**
         * @return the font currently set for this widget
         **/
        QFont font() const;

        /**
         * Returns the plugin name for the applet
         */
        QString pluginName() const;

        /**
         * Whether the applet should conserve resources. If true, try to avoid doing stuff which
         * is computationally heavy. Try to conserve power and resources.
         *
         * @return true if it should conserve resources, false if it does not.
         */
        bool shouldConserveResources() const;

        /**
         * Returns the icon related to this applet
         **/
        QString icon() const;

        /**
         * Returns the category the applet is in, as specified in the
         * .desktop file.
         */
        QString category() const;

        /**
         * @return The type of immutability of this applet
         */
        ImmutabilityType immutability() const;

        void paintWindowFrame(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget);

        /**
         * If for some reason, the applet fails to get up on its feet (the
         * library couldn't be loaded, necessary hardware support wasn't found,
         * etc..) this method returns true
         **/
        bool hasFailedToLaunch() const;

        /**
         * @return true if the applet currently needs to be configured,
         *         otherwise, false
         */
        bool configurationRequired() const;

        /**
         * @return true if this plasmoid provides a GUI configuration
         **/
        bool hasConfigurationInterface() const;

        /**
         * Returns a list of context-related QAction instances.
         *
         * This is used e.g. within the \a DesktopView to display a
         * contextmenu.
         *
         * @return A list of actions. The default implementation returns an
         *         empty list.
         **/
        virtual QList<QAction*> contextualActions();

        /**
         * Returns the QAction with the given name from our collection
         */
        QAction *action(QString name) const;

        /**
         * Adds the action to our collection under the given name
         */
        void addAction(QString name, QAction *action);

        /**
         * Sets the BackgroundHints for this applet @see BackgroundHint
         *
         * @param hints the BackgroundHint combination for this applet
         */
        void setBackgroundHints(const BackgroundHints hints);

        /**
         * @return BackgroundHints flags combination telling if the standard background is shown
         *         and if it has a drop shadow
         */
        BackgroundHints backgroundHints() const;

        /**
         * @return true if this Applet is currently being used as a Containment, false otherwise
         */
        bool isContainment() const;

        /**
         * This method returns screen coordinates for the widget; this method can be somewhat
         * expensive and should ONLY be called when screen coordinates are required. For
         * example when positioning top level widgets on top of the view to create the
         * appearance of unit. This should NOT be used for popups (@see popupPosition) or
         * for normal widget use (use Plasma:: widgets or QGraphicsProxyWidget instead).
         *
         * @return a rect of the applet in screen coordinates.
         */
        QRect screenRect() const;

        /**
         * Reimplemented from QGraphicsItem
         **/
        int type() const;
        enum {
            Type = Plasma::AppletType
        };

        /**
         * @return the Containment, if any, this applet belongs to
         **/
        Containment *containment() const;

        /**
         * Sets the global shorcut to associate with this widget.
         */
        void setGlobalShortcut(const KShortcut &shortcut);

        /**
         * @return the global shortcut associated with this wiget, or
         * an empty shortcut if no global shortcut is associated.
         */
        KShortcut globalShortcut() const;

        /**
         * associate actions with this widget, including ones added after this call.
         * needed to make keyboard shortcuts work.
         */
        virtual void addAssociatedWidget(QWidget *widget);

        /**
         * un-associate actions from this widget, including ones added after this call.
         * needed to make keyboard shortcuts work.
         */
        virtual void removeAssociatedWidget(QWidget *widget);

        /**
         * Gets called when and extender item has to be initialized after a plasma restart. If you
         * create ExtenderItems in your applet, you should implement this function to again create
         * the widget that should be shown in this extender item. This function might look something
         * like this:
         *
         * @code
         * SuperCoolWidget *widget = new SuperCoolWidget();
         * dataEngine("engine")->connectSource(item->config("dataSourceName"), widget);
         * item->setWidget(widget);
         * @endcode
         *
         * You can also add one or more custom qactions to this extender item in this function.
         *
         * Note that by default, not all ExtenderItems are persistent. Only items that are detached,
         * will have their configuration stored when plasma exits.
         */
        virtual void initExtenderItem(ExtenderItem *item);

        /**
         * @param parent the QGraphicsItem this applet is parented to
         * @param serviceId the name of the .desktop file containing the
         *      information about the widget
         * @param appletId a unique id used to differentiate between multiple
         *      instances of the same Applet type
         */
        explicit Applet(QGraphicsItem *parent = 0,
                        const QString &serviceId = QString(),
                        uint appletId = 0);

    Q_SIGNALS:
        /**
         * This signal indicates that an application launch, window
         * creation or window focus event was triggered. This is used, for instance,
         * to ensure that the Dashboard view in Plasma hides when such an event is
         * triggered by an item it is displaying.
         */
        void releaseVisualFocus();

        /**
         * Emitted whenever the applet makes a geometry change, so that views
         * can coordinate themselves with these changes if they desire.
         */
        void geometryChanged();

        /**
         * Emitted by Applet subclasses when they change a sizeHint and wants to announce the change
         */
        void sizeHintChanged(Qt::SizeHint which);

        /**
         * Emitted when an applet has changed values in its configuration
         * and wishes for them to be saved at the next save point. As this implies
         * disk activity, this signal should be used with care.
         *
         * @note This does not need to be emitted from saveState by individual
         * applets.
         */
        void configNeedsSaving();

        /**
         * Emitted when activation is requested due to, for example, a global
         * keyboard shortcut. By default the wiget is given focus.
         */
        void activate();

    public Q_SLOTS:
        /**
         * Sets the immutability type for this applet (not immutable,
         * user immutable or system immutable)
         * @arg immutable the new immutability type of this applet
         */
        void setImmutability(const ImmutabilityType immutable);

        /**
         * Destroys the applet; it will be removed nicely and deleted.
         * Its configuration will also be deleted.
         */
        virtual void destroy();

        /**
         * Lets the user interact with the plasmoid options.
         * Called when the user selects the configure entry
         * from the context menu.
         *
         * Unless there is good reason for overriding this method,
         * Applet subclasses should actually override createConfigurationInterface
         * instead. A good example of when this isn't plausible is
         * when using a dialog prepared by another library, such
         * as KPropertiesDialog from libkfile.
         */
        virtual void showConfigurationInterface();

        /**
         * Causes this applet to raise above all other applets.
         */
        void raise();

        /**
         * Causes this applet to lower below all the other applets.
         */
        void lower();

        /**
         * Sends all pending contraints updates to the applet. Will usually
         * be called automatically, but can also be called manually if needed.
         */
        void flushPendingConstraintsEvents();

        /**
         * This method is called once the applet is loaded and added to a Corona.
         * If the applet requires a QGraphicsScene or has an particularly intensive
         * set of initialization routines to go through, consider implementing it
         * in this method instead of the constructor.
         **/
        virtual void init();

        /**
         * Called when applet configuration values has changed.
         */
        virtual void configChanged();

    protected:
        /**
         * This constructor is to be used with the plugin loading systems
         * found in KPluginInfo and KService. The argument list is expected
         * to have two elements: the KService service ID for the desktop entry
         * and an applet ID which must be a base 10 number.
         *
         * @param parent a QObject parent; you probably want to pass in 0
         * @param args a list of strings containing two entries: the service id
         *      and the applet id
         */
        Applet(QObject *parent, const QVariantList &args);

        /**
         * Call this method when the applet fails to launch properly. An
         * optional reason can be provided.
         *
         * Not that all children items will be deleted when this method is
         * called. If you have pointers to these items, you will need to
         * reset them after calling this method.
         *
         * @param failed true when the applet failed, false when it succeeded
         * @param reason an optional reason to show the user why the applet
         *               failed to launch
         **/
        void setFailedToLaunch(bool failed, const QString &reason = QString());

        /**
         * When called, the Applet should write any information needed as part
         * of the Applet's running state to the configuration object in config()
         * and/or globalConfig().
         *
         * Applets that always sync their settings/state with the config
         * objects when these settings/states change do not need to reimplement
         * this method.
         **/
        virtual void saveState(KConfigGroup &config) const;

        /**
         * Sets whether or not this applet provides a user interface for
         * configuring the applet.
         *
         * It defaults to false, and if true is passed in you should
         * also reimplement createConfigurationInterface()
         *
         * @param hasInterface whether or not there is a user interface available
         **/
        void setHasConfigurationInterface(bool hasInterface);

        /**
         * When the applet needs to be configured before being usable, this
         * method can be called to show a standard interface prompting the user
         * to configure the applet
         *
         * Not that all children items will be deleted when this method is
         * called. If you have pointers to these items, you will need to
         * reset them after calling this method.
         *
         * @param needsConfiguring true if the applet needs to be configured,
         *                         or false if it doesn't
         */
        void setConfigurationRequired(bool needsConfiguring, const QString &reason = QString());

        /**
         * Reimplement this method so provide a configuration interface,
         * parented to the supplied widget. Ownership of the widgets is passed
         * to the parent widget.
         *
         * @param parent the dialog which is the parent of the configuration
         *               widgets
         */
        virtual void createConfigurationInterface(KConfigDialog *parent);

        /**
         * Called when any of the geometry constraints have been updated.
         *
         * This is always called prior to painting and should be used as an
         * opportunity to layout the widget, calculate sizings, etc.
         *
         * Do not call update() from this method; an update() will be triggered
         * at the appropriate time for the applet.
         *
         * @param constraints the type of constraints that were updated
         * @property constraint
         */
        virtual void constraintsEvent(Plasma::Constraints constraints);

        /**
         * Register the widgets that manage mouse clicks but you still want
         * to be able to drag the applet around when holding the mouse pointer
         * on that widget.
         *
         * Calling this results in an eventFilter being places on the widget.
         *
         * @param item the item to watch for mouse move
         */
        void registerAsDragHandle(QGraphicsItem *item);

        /**
         * Unregister a widget registered with registerAsDragHandle.
         *
         * @param item the item to unregister
         */
        void unregisterAsDragHandle(QGraphicsItem *item);

        /**
         * @param item the item to look for if it is registered or not
         * @return true if it is registered, false otherwise
         */
        bool isRegisteredAsDragHandle(QGraphicsItem *item);


        /**
         * @return the extender of this applet.
         */
        Extender *extender() const;

        /**
         * @internal event filter; used for focus watching
         **/
        bool eventFilter(QObject *o, QEvent *e);

        /**
         * @internal scene event filter; used to manage applet dragging
         */
        bool sceneEventFilter (QGraphicsItem *watched, QEvent *event);

        /**
         * @internal manage the mouse movement to drag the applet around
         */
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

        /**
         * @internal manage the mouse movement to drag the applet around
         */
        void mousePressEvent(QGraphicsSceneMouseEvent *event);

        /**
         * Reimplemented from QGraphicsItem
         */
        void focusInEvent(QFocusEvent *event);

        /**
         * Reimplemented from QGraphicsItem
         */
        void resizeEvent(QGraphicsSceneResizeEvent *event);

        /**
         * Reimplemented from QGraphicsItem
         */
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        /**
         * Reimplemented from QGraphicsItem
         */
        QPainterPath shape() const;

        /**
         * Reimplemented from QGraphicsLayoutItem
         */
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const;

        /**
         * Reimplemented from QGraphicsLayoutItem
         */
        void hoverEnterEvent(QGraphicsSceneHoverEvent *event);

        /**
         * Reimplemented from QGraphicsLayoutItem
         */
        void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

        /**
         * Reimplemented from QObject
         */
        void timerEvent (QTimerEvent *event);

    private:
        Q_PRIVATE_SLOT(d, void setFocus())
        Q_PRIVATE_SLOT(d, void checkImmutability())
        Q_PRIVATE_SLOT(d, void themeChanged())
        Q_PRIVATE_SLOT(d, void appletAnimationComplete(QGraphicsItem *item,
                                                       Plasma::Animator::Animation anim))
        Q_PRIVATE_SLOT(d, void selectItemToDestroy())
        Q_PRIVATE_SLOT(d, void updateRect(const QRectF& rect))

        /**
         * Reimplemented from QGraphicsItem
         **/
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

        AppletPrivate *const d;

        //Corona needs to access setFailedToLaunch and init
        friend class Corona;
        friend class CoronaPrivate;
        friend class Containment;
        friend class ContainmentPrivate;
        friend class AppletScript;
        friend class AppletHandle;
        friend class AppletPrivate;
        friend class PopupApplet;
        friend class PopupAppletPrivate;

        friend class Extender;
        friend class ExtenderItem;
};

} // Plasma namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Plasma::Applet::BackgroundHints)

/**
 * Register an applet when it is contained in a loadable module
 */
#define K_EXPORT_PLASMA_APPLET(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_applet_" #libname)) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#endif // multiple inclusion guard
