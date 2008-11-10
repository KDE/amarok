/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
 *
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

#include "theme.h"

#include <QApplication>
#include <QFile>
#ifdef Q_WS_X11
#include <QX11Info>
#endif

#include <KColorScheme>
#include <KComponentData>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KGlobalSettings>
#include <KPixmapCache>
#include <KSelectionWatcher>
#include <KSharedConfig>
#include <KStandardDirs>
#include <KWindowSystem>

#include "private/packages_p.h"

namespace Plasma
{

#define DEFAULT_WALLPAPER_THEME "Blue_Curl"
#define DEFAULT_WALLPAPER_SUFFIX ".jpg"
static const int DEFAULT_WALLPAPER_WIDTH = 1920;
static const int DEFAULT_WALLPAPER_HEIGHT = 1200;

class ThemePrivate
{
public:
    ThemePrivate(Theme *theme)
        : q(theme),
          colorScheme(QPalette::Active, KColorScheme::Window, KSharedConfigPtr(0)),
          buttonColorScheme(QPalette::Active, KColorScheme::Button, KSharedConfigPtr(0)),
          defaultWallpaperTheme(DEFAULT_WALLPAPER_THEME),
          defaultWallpaperSuffix(DEFAULT_WALLPAPER_SUFFIX),
          defaultWallpaperWidth(DEFAULT_WALLPAPER_WIDTH),
          defaultWallpaperHeight(DEFAULT_WALLPAPER_HEIGHT),
          pixmapCache(0),
          locolor(false),
          compositingActive(KWindowSystem::compositingActive()),
          isDefault(false),
          useGlobal(true),
          hasWallpapers(false)
    {
        KConfigGroup cg(KGlobal::config(), "CachePolicies");
        bool useCache = cg.readEntry("CacheTheme", true);
        if (useCache) {
            pixmapCache = new KPixmapCache(KGlobal::mainComponent().componentName());
            pixmapCache->setCacheLimit(cg.readEntry("ThemeCacheKb", 80 * 1024));
        }
        generalFont = QApplication::font();
    }

    ~ThemePrivate()
    {
       delete pixmapCache;
    }

    KConfigGroup &config()
    {
        if (!cfg.isValid()) {
            QString groupName = "Theme";

            if (!useGlobal) {
                QString app = KGlobal::mainComponent().componentName();

                if (!app.isEmpty() && app != "plasma") {
                    kDebug() << "using theme for app" << app;
                    groupName.append("-").append(app);
                }
            }

            cfg = KConfigGroup(KSharedConfig::openConfig("plasmarc"), groupName);
        }

        return cfg;
    }

    QString findInTheme(const QString &image, const QString &theme) const;
    void compositingChanged();

    static const char *defaultTheme;
    static PackageStructure::Ptr packageStructure;

    Theme *q;
    QString themeName;
    KSharedConfigPtr colors;
    KColorScheme colorScheme;
    KColorScheme buttonColorScheme;
    KConfigGroup cfg;
    QFont generalFont;
    QString defaultWallpaperTheme;
    QString defaultWallpaperSuffix;
    int defaultWallpaperWidth;
    int defaultWallpaperHeight;
    KPixmapCache *pixmapCache;
    KSharedConfigPtr svgElementsCache;
    QHash<QString, QList<QString> > invalidElements;

#ifdef Q_WS_X11
    KSelectionWatcher *compositeWatch;
#endif
    bool locolor : 1;
    bool compositingActive : 1;
    bool isDefault : 1;
    bool useGlobal : 1;
    bool hasWallpapers : 1;
};

PackageStructure::Ptr ThemePrivate::packageStructure(0);
const char *ThemePrivate::defaultTheme = "default";

QString ThemePrivate::findInTheme(const QString &image, const QString &theme) const
{
    //TODO: this should be using Package
    QString search;

    if (locolor) {
        search = "desktoptheme/" + theme + "/locolor/" + image;
        search =  KStandardDirs::locate("data", search);
    } else if (!compositingActive) {
        search = "desktoptheme/" + theme + "/opaque/" + image;
        search =  KStandardDirs::locate("data", search);
    }

    //not found or compositing enabled
    if (search.isEmpty()) {
        search = "desktoptheme/" + theme + '/' + image;
        search =  KStandardDirs::locate("data", search);
    }

    return search;
}

void ThemePrivate::compositingChanged()
{
#ifdef Q_WS_X11
    bool nowCompositingActive = compositeWatch->owner() != None;

    if (compositingActive != nowCompositingActive) {
        compositingActive = nowCompositingActive;
        emit q->themeChanged();
    }
#endif
}

class ThemeSingleton
{
public:
    ThemeSingleton()
    {
        self.d->isDefault = true;
    }

   Theme self;
};

K_GLOBAL_STATIC(ThemeSingleton, privateThemeSelf)

Theme *Theme::defaultTheme()
{
    return &privateThemeSelf->self;
}

Theme::Theme(QObject *parent)
    : QObject(parent),
      d(new ThemePrivate(this))
{
    settingsChanged();

#ifdef Q_WS_X11
    Display *dpy = QX11Info::display();
    int screen = DefaultScreen(dpy);
    d->locolor = DefaultDepth(dpy, screen) < 16;

    if (!d->locolor) {
        char net_wm_cm_name[100];
        sprintf(net_wm_cm_name, "_NET_WM_CM_S%d", screen);
        d->compositeWatch = new KSelectionWatcher(net_wm_cm_name, -1, this);
        connect(d->compositeWatch, SIGNAL(newOwner(Window)), this, SLOT(compositingChanged()));
        connect(d->compositeWatch, SIGNAL(lostOwner()), this, SLOT(compositingChanged()));
    }
#endif
}

Theme::~Theme()
{
    delete d;
}

PackageStructure::Ptr Theme::packageStructure()
{
    if (!ThemePrivate::packageStructure) {
        ThemePrivate::packageStructure = new ThemePackage();
    }

    return ThemePrivate::packageStructure;
}

void Theme::settingsChanged()
{
    setThemeName(d->config().readEntry("name", ThemePrivate::defaultTheme));
}

void Theme::setThemeName(const QString &themeName)
{
    QString theme = themeName;
    if (theme.isEmpty() || theme == d->themeName) {
        // let's try and get the default theme at least
        if (d->themeName.isEmpty()) {
            theme = ThemePrivate::defaultTheme;
        } else {
            return;
        }
    }

    //TODO: should we care about names with relative paths in them?
    QString themePath = KStandardDirs::locate("data", "desktoptheme/" + theme + '/');
    if (themePath.isEmpty() && d->themeName.isEmpty()) {
        themePath = KStandardDirs::locate("data", "desktoptheme/default/");

        if (themePath.isEmpty()) {
            return;
        }

        theme = ThemePrivate::defaultTheme;
    }

    if (d->themeName == theme) {
        return;
    }

    if (!d->themeName.isEmpty() && d->pixmapCache) {
        d->pixmapCache->discard();
    }

    d->themeName = theme;

    // load the color scheme config
    QString colorsFile = KStandardDirs::locate("data", "desktoptheme/" + theme + "/colors");
    //kDebug() << "we're going for..." << colorsFile << "*******************";

    // load the wallpaper settings, if any
    KConfig metadata(KStandardDirs::locate("data", "desktoptheme/" + theme + "/metadata.desktop"));
    KConfigGroup cg;
    if (metadata.hasGroup("Wallpaper")) {
        // we have a theme color config, so let's also check to see if
        // there is a wallpaper defined in there.
        cg = KConfigGroup(&metadata, "Wallpaper");
    } else {
        // since we didn't find an entry in the theme, let's look in the main
        // theme config
        cg = d->config();
    }

    d->defaultWallpaperTheme = cg.readEntry("defaultWallpaperTheme", DEFAULT_WALLPAPER_THEME);
    d->defaultWallpaperSuffix = cg.readEntry("defaultFileSuffix", DEFAULT_WALLPAPER_SUFFIX);
    d->defaultWallpaperWidth = cg.readEntry("defaultWidth", DEFAULT_WALLPAPER_WIDTH);
    d->defaultWallpaperHeight = cg.readEntry("defaultHeight", DEFAULT_WALLPAPER_HEIGHT);

    disconnect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
               this, SIGNAL(themeChanged()));
    if (colorsFile.isEmpty()) {
        d->colors = 0;
        connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                this, SIGNAL(themeChanged()));
    } else {
        d->colors = KSharedConfig::openConfig(colorsFile);
    }

    d->colorScheme = KColorScheme(QPalette::Active, KColorScheme::Window, d->colors);
    d->buttonColorScheme = KColorScheme(QPalette::Active, KColorScheme::Button, d->colors);
    d->hasWallpapers =
        !KStandardDirs::locate("data", "desktoptheme/" + theme + "/wallpapers").isEmpty();

    if (d->isDefault) {
        // we're the default theme, let's save our state
        KConfigGroup &cg = d->config();
        if (ThemePrivate::defaultTheme == d->themeName) {
            cg.deleteEntry("name");
        } else {
            cg.writeEntry("name", d->themeName);
        }
    }


    QString svgElementsFile = KStandardDirs::locateLocal("cache", "plasma-svgelements-"+themeName);
    d->invalidElements.clear();
    d->svgElementsCache = KSharedConfig::openConfig(svgElementsFile);

    emit themeChanged();
}

QString Theme::themeName() const
{
    return d->themeName;
}

QString Theme::imagePath(const QString &name) const
{
    // look for a compressed svg file in the theme
    if (name.contains("../")) {
        // we don't support relative paths
        return QString();
    }

    QString path = d->findInTheme(name + ".svgz", d->themeName);

    if (path.isEmpty()) {
        // try for an uncompressed svg file
        path = d->findInTheme(name + ".svg", d->themeName);

        if (path.isEmpty() && d->themeName != ThemePrivate::defaultTheme) {
            // try a compressed svg file in the default theme
            path = d->findInTheme(name + ".svgz", ThemePrivate::defaultTheme);

            if (path.isEmpty()) {
                // try an uncompressed svg file in the default theme
                path = d->findInTheme(name + ".svg", ThemePrivate::defaultTheme);
            }
        }

    }

    if (path.isEmpty()) {
        kDebug() << "Theme says: bad image path " << name;
    }

    return path;
}

QString Theme::wallpaperPath(const QSize &size) const
{
    QString fullPath;
    QString image = d->defaultWallpaperTheme;

    image.append("/contents/images/%1x%2").append(d->defaultWallpaperSuffix);
    QString defaultImage = image.arg(d->defaultWallpaperWidth).arg(d->defaultWallpaperHeight);

    if (size.isValid()) {
        // try to customize the paper to the size requested
        //TODO: this should do better than just fallback to the default size.
        //      a "best fit" matching would be far better, so we don't end
        //      up returning a 1920x1200 wallpaper for a 640x480 request ;)
        image = image.arg(size.width()).arg(size.height());
    } else {
        image = defaultImage;
    }

    //TODO: the theme's wallpaper overrides regularly installed wallpapers.
    //      should it be possible for user installed (e.g. locateLocal) wallpapers
    //      to override the theme?
    if (d->hasWallpapers) {
        // check in the theme first
        fullPath = d->findInTheme("wallpaper/" + image, d->themeName);

        if (fullPath.isEmpty()) {
            fullPath = d->findInTheme("wallpaper/" + defaultImage, d->themeName);
        }
    }

    if (fullPath.isEmpty()) {
        // we failed to find it in the theme, so look in the standard directories
        //kDebug() << "looking for" << image;
        fullPath = KStandardDirs::locate("wallpaper", image);
    }

    if (fullPath.isEmpty()) {
        // we still failed to find it in the theme, so look for the default in
        // the standard directories
        //kDebug() << "looking for" << defaultImage;
        fullPath = KStandardDirs::locate("wallpaper", defaultImage);

        if (fullPath.isEmpty()) {
            kDebug() << "exhausted every effort to find a wallpaper.";
        }
    }

    return fullPath;
}

bool Theme::currentThemeHasImage(const QString &name) const
{
    if (name.contains("../")) {
        // we don't support relative paths
        return false;
    }

    return !(d->findInTheme(name + ".svgz", d->themeName).isEmpty()) ||
           !(d->findInTheme(name + ".svg", d->themeName).isEmpty());
}

KSharedConfigPtr Theme::colorScheme() const
{
    return d->colors;
}

QColor Theme::color(ColorRole role) const
{
    switch (role) {
        case TextColor:
            return d->colorScheme.foreground(KColorScheme::NormalText).color();
            break;

        case HighlightColor:
            return d->colorScheme.background(KColorScheme::ActiveBackground).color();
            break;

        case BackgroundColor:
            return d->colorScheme.background().color();
            break;

        case ButtonTextColor:
            return d->buttonColorScheme.foreground(KColorScheme::NormalText).color();
            break;

        case ButtonBackgroundColor:
            return d->buttonColorScheme.background(KColorScheme::ActiveBackground).color();
            break;
    }

    return QColor();
}

void Theme::setFont(const QFont &font, FontRole role)
{
    Q_UNUSED(role)
    d->generalFont = font;
}

QFont Theme::font(FontRole role) const
{
    Q_UNUSED(role)
    switch (role) {
    case DesktopFont:
    {
        KConfigGroup cg(KGlobal::config(), "General");
        return cg.readEntry("desktopFont", QFont("Sans Serif", 10));
    }
    break;
    case DefaultFont:
    default:
        return d->generalFont;
        break;
    }
}

QFontMetrics Theme::fontMetrics() const
{
    //TODO: allow this to be overridden with a plasma specific font?
    return QFontMetrics(d->generalFont);
}

bool Theme::windowTranslucencyEnabled() const
{
    return d->compositingActive;
}

void Theme::setUseGlobalSettings(bool useGlobal)
{
    if (d->useGlobal == useGlobal) {
        return;
    }

    d->useGlobal = useGlobal;
    d->cfg = KConfigGroup();
    d->themeName.clear();
    settingsChanged();
}

bool Theme::useGlobalSettings() const
{
    return d->useGlobal;
}

bool Theme::findInCache(const QString &key, QPixmap &pix)
{
    return d->pixmapCache && d->pixmapCache->find(key, pix);
}

void Theme::insertIntoCache(const QString& key, const QPixmap& pix)
{
    if (d->pixmapCache) {
        d->pixmapCache->insert(key, pix);
    }
}

bool Theme::findInRectsCache(const QString &image, const QString &element, QRectF &rect) const
{
    if (!d->pixmapCache) {
        return false;
    }

    KConfigGroup imageGroup(d->svgElementsCache, image);
    rect = imageGroup.readEntry(element+"Size", QRectF());

    if (!d->invalidElements.contains(image)) {
        d->invalidElements[image] = imageGroup.readEntry("invalidElements", QStringList());
    }

    return d->invalidElements[image].contains(element) || rect.isValid();
}

void Theme::insertIntoRectsCache(const QString& image, const QString &element, const QRectF &rect)
{
    if (!d->pixmapCache) {
        return;
    }

    KConfigGroup imageGroup(d->svgElementsCache, image);
    if (rect.isValid()) {
        imageGroup.writeEntry(element+"Size", rect);
    } else if (!d->invalidElements[image].contains(element)) {
        d->invalidElements[image].append(element);
        if (d->invalidElements[image].count() > 1000) {
            d->invalidElements[image].pop_front();
        }
        imageGroup.writeEntry("invalidElements", d->invalidElements[image]);
    }
}

void Theme::invalidateRectsCache(const QString& image)
{
    KConfigGroup imageGroup(d->svgElementsCache, image);
    imageGroup.deleteGroup();
}

void Theme::setCacheLimit(int kbytes)
{
    if (d->pixmapCache) {
        d->pixmapCache->setCacheLimit(kbytes);
    }
}

}

#include <theme.moc>
