
class Amarok.Playlist{

Q_INVOKABLE int activeIndex()
Q_INVOKABLE int totalTrackCount()
Q_INVOKABLE QString saveCurrentPlaylist()
Q_INVOKABLE void addMedia( const QUrl &url )
Q_INVOKABLE void addTrack( const Meta::TrackPtr &track )
Q_INVOKABLE void addMediaList( const QList<QUrl> &urls )
Q_INVOKABLE void addTrackList( const Meta::TrackList &tracks )
Q_INVOKABLE void clearPlaylist()
Q_INVOKABLE void playByIndex( int index )
Q_INVOKABLE void playMedia( const QUrl &url )
Q_INVOKABLE void playTrack( const Meta::TrackPtr &track )
Q_INVOKABLE void playMediaList( const QList<QUrl> &urls )
Q_INVOKABLE void playTrackList( const Meta::TrackList &trackList )
Q_INVOKABLE void removeCurrentTrack()
Q_INVOKABLE void removeByIndex( int index )
Q_INVOKABLE void savePlaylist( const QString& path )
Q_INVOKABLE void setStopAfterCurrent( bool on )
Q_INVOKABLE bool stopAfterCurrent()
Q_INVOKABLE void togglePlaylist()
Q_INVOKABLE QStringList filenames()
Q_INVOKABLE QStringList selectedFilenames()
};

class Amarok.Bookmark{

Q_PROPERTY( int id READ id WRITE setId )
Q_PROPERTY( BookmarkGroupPtr parent READ parent WRITE setParent )
Q_PROPERTY( QString command READ command WRITE setCommand )
Q_PROPERTY( QString name READ name WRITE setName )
Q_PROPERTY( QString path READ path WRITE setPath )
Q_PROPERTY( QString description READ description WRITE setDescription )
Q_PROPERTY( bool isNull READ isNull )
Q_PROPERTY( QString customValue READ customValue WRITE setCustomValue )
Q_PROPERTY( QString url READ url )
Q_PROPERTY( int id READ id )
Q_PROPERTY( BookmarkGroupPtr parent READ parent WRITE setParent )
Q_PROPERTY( QString name READ name WRITE setName )
Q_PROPERTY( QString description READ description WRITE setDescription )
Q_PROPERTY( int childCount READ childCount )
Q_INVOKABLE QJSValue contextView()
Q_INVOKABLE QJSValue currentPlaylistView()
Q_INVOKABLE QJSValue browserView()
Q_INVOKABLE QJSValue createCurrentTrackBookmark()
Q_INVOKABLE QJSValue bookmarkCtorWrapper( QJSValue arg0, QJSValue arg1  = QJSValue(QJSValue::UndefinedValue ) )
Q_INVOKABLE QJSValue bookmarkGroupCtorWrapper( QJSValue arg0, QJSValue arg1 = QJSValue(QJSValue::UndefinedValue ) )
Q_INVOKABLE bool save()
Q_INVOKABLE void initFromString( const QString & urlString )
Q_INVOKABLE bool run()
Q_INVOKABLE void removeFromDb()
Q_INVOKABLE QString prettyCommand()
Q_INVOKABLE StringMap args()
Q_INVOKABLE void setArg( const QString &name, const QString &value )
Q_INVOKABLE void save()
Q_INVOKABLE BookmarkGroupList childGroups()
Q_INVOKABLE BookmarkList childBookmarks()
Q_INVOKABLE void clear()
Q_INVOKABLE void deleteChildBookmark( const AmarokUrlPtr &bookmark )
Q_INVOKABLE void deleteChildBookmarkgroup( const BookmarkGroupPtr &bookmarkGroup )};

class Amarok.Window.Statusbar{

Q_INVOKABLE void longMessage( const QString &text )
Q_INVOKABLE void shortMessage( const QString &text )};

class ScriptableServiceScript{

Q_INVOKABLE int insertItem( StreamItem* item )
Q_INVOKABLE void setCurrentInfo( const QString &infoHtml )
Q_INVOKABLE int donePopulating()
Q_INVOKABLE void setIcon( const QPixmap &icon )
Q_INVOKABLE void setEmblem( const QPixmap &icon )
Q_INVOKABLE void setScalableEmblem( const QString &emblemPath )
};

class Amarok.StreamItem{

Q_PROPERTY( QString itemName WRITE setItemName READ itemName )
Q_PROPERTY( QString infoHtml WRITE setInfoHtml READ infoHtml )
Q_PROPERTY( QString playableUrl WRITE setPlayableUrl READ playableUrl )
Q_PROPERTY( QString callbackData WRITE setCallbackData READ callbackData )
Q_PROPERTY( int level WRITE setLevel READ level )
Q_PROPERTY( QString album WRITE setAlbum READ album )
Q_PROPERTY( QString artist WRITE setArtist READ artist )
Q_PROPERTY( QString genre WRITE setGenre READ genre )
Q_PROPERTY( QString composer WRITE setComposer READ composer )
Q_PROPERTY( int year WRITE setYear READ year )
Q_PROPERTY( QString coverUrl WRITE setCoverUrl READ coverUrl )};

class Amarok.Window.KNotify{

Q_PROPERTY( bool kNotifyEnabled READ kNotifyEnabled WRITE setKNotifyEnabled )
Q_INVOKABLE void showCurrentTrack()
Q_INVOKABLE void show(const QString &title, const QString &body, const QPixmap &pixmap = QPixmap() )};

class Amarok{

Q_INVOKABLE void quitAmarok()
Q_INVOKABLE void debug( const QString& text )
Q_INVOKABLE int alert( const QString& text, const QString& type = QStringLiteral("information") )
Q_INVOKABLE void end()
Q_INVOKABLE bool runScript( const QString& name )
Q_INVOKABLE bool stopScript( const QString& name )
Q_INVOKABLE QStringList listRunningScripts()
};

class Amarok.PlaylistManager{

Q_PROPERTY( QList<int> availableCategories READ availableCategories )
Q_INVOKABLE bool save( Meta::TrackList tracks, const QString &name = QString()
Q_INVOKABLE bool import( const QUrl &fromLocation )
Q_INVOKABLE bool rename( Playlists::PlaylistPtr playlist, const QString &newName )
Q_INVOKABLE bool deletePlaylists( Playlists::PlaylistList playlistList )
Q_INVOKABLE bool isWritable( const Playlists::PlaylistPtr &playlist )
};

class Amarok.Window{

Q_PROPERTY( bool isTrayIconShown READ isTrayIconShown )
Q_PROPERTY( QString activeBrowserName READ activeBrowserName )
Q_PROPERTY( QMainWindow* mainWindow READ mainWindow )
Q_PROPERTY( QString styleSheet READ styleSheet WRITE setStyleSheet )
Q_PROPERTY( QFont font READ font WRITE setFont )
Q_PROPERTY( QPalette palette READ palette WRITE setPalette )
Q_INVOKABLE void addToolsMenu( QMenu *menu )
Q_INVOKABLE void addSettingsMenu( QMenu *menu )
Q_INVOKABLE bool addToolsMenu( const QString &id, const QString &menuTitle, const QString &icon = QStringLiteral("amarok") )
Q_INVOKABLE void addToolsSeparator()
Q_INVOKABLE bool addSettingsMenu( const QString &id, const QString &actionName, const QString &icon = QStringLiteral("amarok") )
Q_INVOKABLE bool addCustomAction( const QString &menuName, const QString &id, const QString &actionName, const QString &icon = QStringLiteral("amarok") )
Q_INVOKABLE void addSettingsSeparator()
Q_INVOKABLE void showTrayIcon( bool show )
Q_INVOKABLE void showToolTip()
};

class Amarok.Info{

Q_INVOKABLE QString scriptPath()
Q_INVOKABLE QString scriptConfigPath( const QString& name )
Q_INVOKABLE QString iconPath( const QString& name, int size )
Q_INVOKABLE QString version()};

class Amarok.Engine{

Q_PROPERTY( bool randomMode READ randomMode WRITE setRandomMode )
Q_PROPERTY( bool dynamicMode READ dynamicMode )
Q_PROPERTY( bool repeatPlaylist READ repeatPlaylist WRITE setRepeatPlaylist )
Q_PROPERTY( bool repeatTrack READ repeatTrack WRITE setRepeatTrack )
Q_PROPERTY( int volume READ volume WRITE setVolume )
Q_PROPERTY( int fadeoutLength READ fadeoutLength WRITE setFadeoutLength )
Q_INVOKABLE void Play()
Q_INVOKABLE void Stop( bool forceInstant = false )
Q_INVOKABLE void Pause()
Q_INVOKABLE void Next()
Q_INVOKABLE void Prev()
Q_INVOKABLE void PlayPause()
Q_INVOKABLE void Seek( int ms )
Q_INVOKABLE void SeekRelative( int ms )
Q_INVOKABLE void SeekForward( int ms = 10000 )
Q_INVOKABLE void SeekBackward( int ms = 10000 )
Q_INVOKABLE void Mute()
Q_INVOKABLE QJSValue currentTrack()
};

class Amarok.Engine.Equalizer{

Q_PROPERTY( bool enabled READ enabled )
Q_PROPERTY( bool isSupported READ isSupported )
Q_PROPERTY( QString selectedPreset READ selectedPreset WRITE setEqualizerPreset )
Q_PROPERTY( QStringList userPresets READ userPresets )
Q_PROPERTY( QStringList translatedGlobalPresetList READ translatedGlobalPresetList )
Q_PROPERTY( QStringList globalPresetList READ globalPresetList )
Q_PROPERTY( QStringList translatedDefaultPresetList READ translatedDefaultPresetList )
Q_PROPERTY( QStringList defaultPresetList READ defaultPresetList )
Q_PROPERTY( QList<int> gains READ gains WRITE setGains )
Q_PROPERTY( int maxGain READ maxGain )
Q_PROPERTY( QStringList bandFrequencies READ bandFrequencies )
Q_INVOKABLE bool deletePreset( const QString &presetName )
Q_INVOKABLE void savePreset( const QString &name, const QList<int> &presetGains )
};

class Amarok.Window.OSD{

Q_INVOKABLE void showCurrentTrack()
Q_INVOKABLE void show()
Q_INVOKABLE void setDuration( int ms )
Q_INVOKABLE void setTextColor( const QColor &color )
Q_INVOKABLE void setOffset( int y )
Q_INVOKABLE void setImage( const QPixmap &image )
Q_INVOKABLE void setScreen( int screen )
Q_INVOKABLE void setText( const QString &text )
Q_INVOKABLE void setRating( const short rating )};

class Amarok.Script{

Q_INVOKABLE QVariant readConfig( const QString &name, const QVariant &defaultValue )
Q_INVOKABLE void writeConfig( const QString &name, const QVariant &content )};

class Amarok.Lyrics{

Q_INVOKABLE void showLyrics( const QString& lyrics )
Q_INVOKABLE void showLyricsHtml( const QString& lyrics )
Q_INVOKABLE void showLyricsError( const QString& error )
Q_INVOKABLE void showLyricsNotFound( const QString& msg )
Q_INVOKABLE QString escape( const QString& str )
Q_INVOKABLE void setLyricsForTrack( const QString& trackUrl , const QString& lyrics )
Q_INVOKABLE QString toUtf8( const QByteArray& lyrics, const QString& encoding = QStringLiteral("UTF-8") )
Q_INVOKABLE QString QStringtoUtf8( const QString& lyrics, const QString& encoding = QStringLiteral("UTF-8") )
Q_INVOKABLE QByteArray fromUtf8( const QString& str, const QString& encoding )
};

class Amarok.ServicePluginManager{

Q_INVOKABLE QStringList loadedServices()
Q_INVOKABLE QStringList loadedServiceNames()
Q_INVOKABLE QString serviceDescription( const QString &service )
Q_INVOKABLE QString serviceMessages( const QString &service )
Q_INVOKABLE QString sendMessage( const QString &service, const QString &message )};

class Amarok.Window.CollectionView{

Q_PROPERTY( QString filter READ filter WRITE setFilter )
Q_PROPERTY( QJSValue selection READ selectionScriptValue )
Q_PROPERTY( bool showYears READ showYears WRITE setShowYears )
Q_PROPERTY( bool showTrackNumbers READ showTrackNumbers WRITE setShowTrackNumbers )
Q_PROPERTY( bool showCovers READ showCovers WRITE setShowCovers )
Q_PROPERTY( QList<int> levels READ levels WRITE setLevels )
Q_PROPERTY( bool mergedView READ mergedView WRITE setMergedView )
Q_PROPERTY( CollectionTreeItem* parent READ parent )
Q_PROPERTY( int childCount READ childCount )
Q_PROPERTY( QList<CollectionTreeItem*> children READ children )
Q_PROPERTY( int row READ row )
Q_PROPERTY( int level READ level )
Q_PROPERTY( Collections::Collection* parentCollection READ parentCollection )
Q_PROPERTY( bool isCollection READ isCollection )
Q_PROPERTY( bool isDataItem READ isDataItem )
Q_PROPERTY( bool isAlbumItem READ isAlbumItem )
Q_PROPERTY( bool isTrackItem READ isTrackItem )
Q_PROPERTY( bool isVariousArtistItem READ isVariousArtistItem )
Q_PROPERTY( bool isNoLabelItem READ isNoLabelItem )
Q_PROPERTY( bool childrenLoaded READ childrenLoaded )
Q_PROPERTY( bool singleCollection READ singleCollection )
Q_PROPERTY( QList<CollectionTreeItem*> selectedItems READ selectedItems )
Q_INVOKABLE void addFilter( Collections::QueryMaker *queryMaker )
Q_INVOKABLE void loadChildren()

};

class Importer{

Q_INVOKABLE QStringList availableBindings()
Q_INVOKABLE bool loadAmarokBinding( const QString &name )
Q_INVOKABLE void loadExtension( const QString &src )
Q_INVOKABLE bool loadQtBinding( const QString &binding )
Q_INVOKABLE bool include( const QString &relativeFile )};

class Amarok.CollectionManager{

Q_INVOKABLE int totalAlbums()
Q_INVOKABLE int totalArtists()
Q_INVOKABLE int totalComposers()
Q_INVOKABLE int totalGenres()
Q_INVOKABLE int totalTracks()
Q_INVOKABLE QStringList collectionLocation()
Q_INVOKABLE QStringList query( const QString& sql )
Q_INVOKABLE QString escape( const QString& sql )
Q_INVOKABLE void scanCollection()
Q_INVOKABLE void scanCollectionChanges()
};

class BiasFactory{

Q_PROPERTY( bool enabled READ enabled WRITE setEnabled )
Q_PROPERTY( QString name READ i18nName WRITE setI18nName )
Q_PROPERTY( QString identifier READ name WRITE setName )
Q_PROPERTY( QString description READ i18nDescription WRITE setI18nDescription )
Q_PROPERTY( QJSValue widget READ widgetFunction WRITE setWidgetFunction )
Q_PROPERTY( QJSValue fromXml READ fromXmlFunction WRITE setFromXmlFunction )
Q_PROPERTY( QJSValue toXml READ toXmlFunction WRITE setToXmlFunction )
Q_PROPERTY( QJSValue matchingTracks READ matchingTracksFunction WRITE setMatchingTracksFunction )
Q_PROPERTY( QJSValue trackMatches READ trackMatchesFunction WRITE setTrackMatchesFunction )
Q_PROPERTY( QJSValue toStringFunction READ toStringFunction WRITE setToStringFunction )
Q_PROPERTY( QJSValue init READ initFunction WRITE setInitFunction )
Q_PROPERTY( int count READ trackCount )
Q_PROPERTY( bool isFull READ isFull )
Q_PROPERTY( bool isOutstanding READ isOutstanding )
Q_PROPERTY( bool isEmpty READ isEmpty )
Q_INVOKABLE QJSValue groupBiasCtor()
Q_INVOKABLE QJSValue biasCtor()
Q_INVOKABLE QJSValue trackSetConstructor( QJSValue arg0, QJSValue arg1 = QJSValue(QJSValue::UndefinedValue) )
Q_INVOKABLE void reset( bool value )
Q_INVOKABLE bool containsUid( const QString& uid )
Q_INVOKABLE bool containsTrack( const Meta::TrackPtr &track )
Q_INVOKABLE void uniteTrack( const Meta::TrackPtr &track )
Q_INVOKABLE void uniteTrackSet( const Dynamic::TrackSet &trackSet )
Q_INVOKABLE void uniteUids( const QStringList &uids )
Q_INVOKABLE void intersectTrackSet( const Dynamic::TrackSet &trackSet )
Q_INVOKABLE void intersectUids( const QStringList &uids )
Q_INVOKABLE void subtractTrack( const Meta::TrackPtr &track )
Q_INVOKABLE void subtractTrackSet( const Dynamic::TrackSet &trackSet )
Q_INVOKABLE void subtractUids( const QStringList &uids )
};

class Collection{

Q_PROPERTY( bool isOrganizable READ isOrganizable )
Q_PROPERTY( bool isWritable READ isWritable )
Q_PROPERTY( QString collectionId READ collectionId )
Q_PROPERTY( QString prettyName READ toString )
Q_PROPERTY( float usedCapacity READ usedCapacity )
Q_PROPERTY( float totalCapacity READ totalCapacity )
Q_PROPERTY( bool isValid READ isValid )
Q_PROPERTY( QIcon icon READ icon )
Q_PROPERTY( bool isQueryable READ isQueryable )
Q_PROPERTY( bool isViewable READ isViewable )
Q_PROPERTY( bool supportsTranscode READ supportsTranscode )
Q_PROPERTY( QString prettyLocation READ prettyLocation )
Q_PROPERTY( QStringList actualLocation READ actualLocation )
Q_INVOKABLE void copyTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection )
Q_INVOKABLE void copyTracks( const Meta::TrackPtr &track, Collections::Collection *targetCollection )
Q_INVOKABLE void queryAndCopyTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection )
Q_INVOKABLE void moveTracks( const Meta::TrackList &tracks, Collections::Collection *targetCollection )
Q_INVOKABLE void moveTracks( const Meta::TrackPtr &track, Collections::Collection *targetCollection )
Q_INVOKABLE void queryAndMoveTracks( Collections::QueryMaker *queryMaker, Collections::Collection *targetCollection )
Q_INVOKABLE void removeTracks( const Meta::TrackList &trackList )
Q_INVOKABLE void removeTracks( const Meta::TrackPtr &track )
Q_INVOKABLE void queryAndRemoveTracks( Collections::QueryMaker *qm )
};

class Playlist{

Q_PROPERTY( bool isValid READ isValid )
Q_PROPERTY( QString name READ toString WRITE setName )
Q_PROPERTY( QUrl uidUrl READ uidUrl )
Q_PROPERTY( int trackCount READ trackCount )
Q_PROPERTY( Playlists::PlaylistProvider* provider READ provider )
Q_INVOKABLE void triggerFullLoad()
Q_INVOKABLE void triggerQuickLoad()
Q_INVOKABLE void addTrack( Meta::TrackPtr track, int position = -1 )
Q_INVOKABLE void removeTrack( int position )
Q_INVOKABLE QString toString()
};

class PlaylistProvider{

Q_PROPERTY( bool isWritable READ isWritable )
Q_PROPERTY( QString prettyName READ toString )
Q_PROPERTY( bool isValid READ isValid )
Q_PROPERTY( QIcon icon READ icon )
Q_PROPERTY( int category READ category )
Q_PROPERTY( int playlistCount READ playlistCount )
Q_INVOKABLE QString toString()
Q_INVOKABLE void renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName )
Q_INVOKABLE bool deletePlaylists( const Playlists::PlaylistList &playlistlist )
};

class QueryMaker{

Q_PROPERTY( bool isValid READ isValid )
Q_PROPERTY( QString filter READ filter )
Q_INVOKABLE void run()
Q_INVOKABLE void abort()
Q_INVOKABLE void addFilter( const QString &filter )
};
