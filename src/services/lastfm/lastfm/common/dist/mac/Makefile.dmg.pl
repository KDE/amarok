#!/usr/bin/perl
# Author: max@last.fm
# Usage: export QT from your QMake project
# Usage: export QMAKE_LIBDIR_QT from your QMake project
# Usage: Makefile.dmg.pl $$DESTDIR $$VERSION $$LIBS
# You must include this from within an app target qmake pro file for it to work!
# the above usage refers to the QMAKE variables

use Cwd 'abs_path';
use File::Basename;

$DESTDIR = shift;
$VERSION = shift;
$QT_FRAMEWORKS_DIR = $ENV{'QMAKE_LIBDIR_QT'};
$REVISION = `svn info | grep "Last Changed Rev" | cut -d' ' -f4`;

while( my $v = shift )
{
	if ($v =~ m/^-l(.*)/)
	{
		push( @DYLIBS, "lib$1.1.dylib" );
	}
}

$plist = "\$(CONTENTS)/Info.plist";
$root = abs_path( dirname( $0 ) . "/../../../" );

sub getQtModules()
{
	# these 4 lines de-dupe $QT
	my %saw;
	my @in = split( ' ', $ENV{'QT'} );
	@saw{@in} = ();
	my @out = keys %saw;

	foreach my $x (@out)
	{
	    $x = 'Qt'.ucfirst( $x ) unless $x eq "phonon";
		$bundle_frameworks .= "\$(CONTENTS)/Frameworks/$x.framework ";
	    push( @QT_MODULES, $x );
	}
}

getQtModules();

foreach my $x (@DYLIBS)
{
	$bundle_macos .= "\$(CONTENTS)/MacOS/$x ";
}

$DEPOSX = "\$(DIST_TOOLS_DIR)/deposx.sh";
$dmg = "\$(DESTDIR)\$(QMAKE_TARGET)-\$(VERSION)-\$(REVISION).dmg";

print <<END;
DIST_TOOLS_DIR = $root/common/dist/mac
BUNDLE = \$(DESTDIR)\$(QMAKE_TARGET).app
CONTENTS = \$(BUNDLE)/Contents
VERSION = $VERSION
REVISION = $REVISION
BUNDLE_FRAMEWORKS = $bundle_frameworks
BUNDLE_MACOS = $bundle_macos
INSTALLDIR = /Applications/\$(QMAKE_TARGET).app

.PHONY = bundle bundle-clean bundle-install dmg dmg-clean help everything

YOUR_MUM: all

\$(DESTDIR)mxcl-is-super: \$(TARGET) $plist
	perl -pi -e 's/@VERSION@/'\$(VERSION)'/g' $plist
	perl -pi -e 's/@SHORT_VERSION@/'`echo \$(VERSION) | cut -d'.' -f1,2,3`'/g' $plist
	$DEPOSX \$(TARGET) $QT_FRAMEWORKS_DIR
	touch \$\@

bundle-clean:
	rm -rf \$(BUNDLE_FRAMEWORKS)
	rm -f \$(BUNDLE_MACOS)
	rm -rf \$(CONTENTS)/MacOS/imageformats
	rm -f \$(CONTENTS)/COPYING
	rm -f \$(CONTENTS)/Resources/qt.conf

dmg: $dmg

dmg-clean: bundle-clean
	rm -f $dmg

$dmg: bundle
	rm -f '\$\@'
	hdiutil create -srcfolder '\$(DESTDIR)\$(QMAKE_TARGET).app' -format UDZO -imagekey zlib-level=9 -scrub '\$\@'

\$(CONTENTS)/Resources/qt.conf:
	echo '[Paths]' > /tmp/qt.conf
	echo 'plugins = MacOS' >> /tmp/qt.conf
	mv /tmp/qt.conf \$(CONTENTS)/Resources

\$(CONTENTS)/COPYING:
	cp $root/COPYING \$(CONTENTS)/COPYING

END


QtFrameworks();
plugins( "imageformats" );
plugins( "phonon_backend" );
dylibs();


sub plugins
{
	my $d = shift;
	
	my $from = abs_path( "$QT_FRAMEWORKS_DIR/../plugins/$d" );
	my $to = "\$(CONTENTS)/MacOS/$d";
	my $install = "\$(INSTALLDIR)/Contents/MacOS/$d";

print <<END;
$to:
	mkdir -p \$\@

$install:
	mkdir -p \$\@

END

	opendir( DIR, $from );
	foreach my $name (grep( /\.dylib$/, readdir( DIR ) ))
	{
		next if ($name =~ /_debug\.dylib$/);

		print <<END;
$to/$name: $from/$name |$to
	cp $from/$name \$\@
	$DEPOSX \$\@ $QT_FRAMEWORKS_DIR

$install/$name: $to/$name |$install
	cp $to/$name \$\@

END
		$bundle_deps .= " $to/$name";
		$install_deps .= " $install/$name";
	}
	closedir( DIR );
}


sub QtFrameworks
{
	my $to = "\$(CONTENTS)/Frameworks";
	my $install = "\$(INSTALLDIR)/Contents/Frameworks";
	
	print <<END;
$to:
	mkdir -p \$\@

$install:
	mkdir -p \$\@
END
	foreach my $module (@QT_MODULES)
	{
		print <<END;

$to/$module.framework: $QT_FRAMEWORKS_DIR/$module.framework |$to
	cp -Rf $QT_FRAMEWORKS_DIR/$module.framework \$\@
	rm -f \$\@/Versions/4/${module}_debug \$\@/${module}_debug*
	$DEPOSX \$\@/$module $QT_FRAMEWORKS_DIR
	install_name_tool -id \$\@/Versions/4/$module \$\@/Versions/4/$module

$install/$module.framework: $to/$module.framework
	cp -Rf $to/$module.framework $install

END
		$install_deps .= " $install/$module.framework";
	}
}

sub dylibs
{
	my $to = "\$(CONTENTS)/MacOS";
	my $install = "\$(INSTALLDIR)/Contents/MacOS";
	
	foreach my $dylib (@DYLIBS)
	{
		print <<END;

$to/$dylib: \$(DESTDIR)$dylib
	cp \$(DESTDIR)$dylib \$\@
	$DEPOSX \$\@ $QT_FRAMEWORKS_DIR

$install/$dylib: $to/$dylib |$install
	cp $to/$dylib \$\@
END
		$install_deps .= " $install/$dylib";
	}	
}

print <<END;

\$(INSTALLDIR)/Contents:
	mkdir -p \$\@

\$(INSTALLDIR)/Contents/MacOS:
	mkdir -p \$\@
	
\$(INSTALLDIR)/Contents/MacOS/\$(QMAKE_TARGET): \$(TARGET) |\$(INSTALLDIR)/Contents/MacOS
	cp \$(TARGET) \$(INSTALLDIR)/Contents/MacOS

\$(INSTALLDIR)/Contents/Info.plist: |\$(INSTALLDIR)/Contents
	cp \$(CONTENTS)/Info.plist \$(INSTALLDIR)/Contents

\$(INSTALLDIR)/Contents/Resources/qt.conf: \$(INSTALLDIR)/Contents/Resources
	cp \$(CONTENTS)/Resources/qt.conf \$(INSTALLDIR)/Contents/Resources

\$(INSTALLDIR)/Contents/Resources: |\$(INSTALLDIR)/Contents
	cp -r \$(CONTENTS)/Resources \$(INSTALLDIR)/Contents

bundle-install: bundle \$(INSTALLDIR)/Contents/MacOS/\$(QMAKE_TARGET) \$(INSTALLDIR)/Contents/Info.plist $install_deps \$(INSTALLDIR)/Contents/Resources/qt.conf

bundle: all \$(BUNDLE_FRAMEWORKS) \$(BUNDLE_MACOS) \$(CONTENTS)/COPYING \$(DESTDIR)mxcl-is-super \$(CONTENTS)/Resources/qt.conf $bundle_deps

END
#!/usr/bin/perl
# Author: max@last.fm
# Usage: export QT from your QMake project
# Usage: export QMAKE_LIBDIR_QT from your QMake project
# Usage: Makefile.dmg.pl $$DESTDIR $$VERSION $$LIBS
# You must include this from within an app target qmake pro file for it to work!
# the above usage refers to the QMAKE variables

use Cwd 'abs_path';
use File::Basename;

$DESTDIR = shift;
$VERSION = shift;
$QT_FRAMEWORKS_DIR = $ENV{'QMAKE_LIBDIR_QT'};
$REVISION = `svn info | grep "Last Changed Rev" | cut -d' ' -f4`;

while( my $v = shift )
{
	if ($v =~ m/^-l(.*)/)
	{
		push( @DYLIBS, "lib$1.1.dylib" );
	}
}

$plist = "\$(CONTENTS)/Info.plist";
$root = abs_path( dirname( $0 ) . "/../../../" );

sub getQtModules()
{
	# these 4 lines de-dupe $QT
	my %saw;
	my @in = split( ' ', $ENV{'QT'} );
	@saw{@in} = ();
	my @out = keys %saw;

	foreach my $x (@out)
	{
	    $x = 'Qt'.ucfirst( $x ) unless $x eq "phonon";
		$bundle_frameworks .= "\$(CONTENTS)/Frameworks/$x.framework ";
	    push( @QT_MODULES, $x );
	}
}

getQtModules();

foreach my $x (@DYLIBS)
{
	$bundle_macos .= "\$(CONTENTS)/MacOS/$x ";
}

$DEPOSX = "\$(DIST_TOOLS_DIR)/deposx.sh";
$dmg = "\$(DESTDIR)\$(QMAKE_TARGET)-\$(VERSION)-\$(REVISION).dmg";

print <<END;
DIST_TOOLS_DIR = $root/common/dist/mac
BUNDLE = \$(DESTDIR)\$(QMAKE_TARGET).app
CONTENTS = \$(BUNDLE)/Contents
VERSION = $VERSION
REVISION = $REVISION
BUNDLE_FRAMEWORKS = $bundle_frameworks
BUNDLE_MACOS = $bundle_macos
INSTALLDIR = /Applications/\$(QMAKE_TARGET).app

.PHONY = bundle bundle-clean bundle-install dmg dmg-clean help everything

YOUR_MUM: all

\$(DESTDIR)mxcl-is-super: \$(TARGET) $plist
	perl -pi -e 's/@VERSION@/'\$(VERSION)'/g' $plist
	perl -pi -e 's/@SHORT_VERSION@/'`echo \$(VERSION) | cut -d'.' -f1,2,3`'/g' $plist
	$DEPOSX \$(TARGET) $QT_FRAMEWORKS_DIR
	touch \$\@

bundle-clean:
	rm -rf \$(BUNDLE_FRAMEWORKS)
	rm -f \$(BUNDLE_MACOS)
	rm -rf \$(CONTENTS)/MacOS/imageformats
	rm -f \$(CONTENTS)/COPYING
	rm -f \$(CONTENTS)/Resources/qt.conf

dmg: $dmg

dmg-clean: bundle-clean
	rm -f $dmg

$dmg: bundle
	rm -f '\$\@'
	hdiutil create -srcfolder '\$(DESTDIR)\$(QMAKE_TARGET).app' -format UDZO -imagekey zlib-level=9 -scrub '\$\@'

\$(CONTENTS)/Resources/qt.conf:
	echo '[Paths]' > /tmp/qt.conf
	echo 'plugins = MacOS' >> /tmp/qt.conf
	mv /tmp/qt.conf \$(CONTENTS)/Resources

\$(CONTENTS)/COPYING:
	cp $root/COPYING \$(CONTENTS)/COPYING

END


QtFrameworks();
plugins( "imageformats" );
plugins( "phonon_backend" );
dylibs();


sub plugins
{
	my $d = shift;
	
	my $from = abs_path( "$QT_FRAMEWORKS_DIR/../plugins/$d" );
	my $to = "\$(CONTENTS)/MacOS/$d";
	my $install = "\$(INSTALLDIR)/Contents/MacOS/$d";

print <<END;
$to:
	mkdir -p \$\@

$install:
	mkdir -p \$\@

END

	opendir( DIR, $from );
	foreach my $name (grep( /\.dylib$/, readdir( DIR ) ))
	{
		next if ($name =~ /_debug\.dylib$/);

		print <<END;
$to/$name: $from/$name |$to
	cp $from/$name \$\@
	$DEPOSX \$\@ $QT_FRAMEWORKS_DIR

$install/$name: $to/$name |$install
	cp $to/$name \$\@

END
		$bundle_deps .= " $to/$name";
		$install_deps .= " $install/$name";
	}
	closedir( DIR );
}


sub QtFrameworks
{
	my $to = "\$(CONTENTS)/Frameworks";
	my $install = "\$(INSTALLDIR)/Contents/Frameworks";
	
	print <<END;
$to:
	mkdir -p \$\@

$install:
	mkdir -p \$\@
END
	foreach my $module (@QT_MODULES)
	{
		print <<END;

$to/$module.framework: $QT_FRAMEWORKS_DIR/$module.framework |$to
	cp -Rf $QT_FRAMEWORKS_DIR/$module.framework \$\@
	rm -f \$\@/Versions/4/${module}_debug \$\@/${module}_debug*
	$DEPOSX \$\@/$module $QT_FRAMEWORKS_DIR
	install_name_tool -id \$\@/Versions/4/$module \$\@/Versions/4/$module

$install/$module.framework: $to/$module.framework
	cp -Rf $to/$module.framework $install

END
		$install_deps .= " $install/$module.framework";
	}
}

sub dylibs
{
	my $to = "\$(CONTENTS)/MacOS";
	my $install = "\$(INSTALLDIR)/Contents/MacOS";
	
	foreach my $dylib (@DYLIBS)
	{
		print <<END;

$to/$dylib: \$(DESTDIR)$dylib
	cp \$(DESTDIR)$dylib \$\@
	$DEPOSX \$\@ $QT_FRAMEWORKS_DIR

$install/$dylib: $to/$dylib |$install
	cp $to/$dylib \$\@
END
		$install_deps .= " $install/$dylib";
	}	
}

print <<END;

\$(INSTALLDIR)/Contents:
	mkdir -p \$\@

\$(INSTALLDIR)/Contents/MacOS:
	mkdir -p \$\@
	
\$(INSTALLDIR)/Contents/MacOS/\$(QMAKE_TARGET): \$(TARGET) |\$(INSTALLDIR)/Contents/MacOS
	cp \$(TARGET) \$(INSTALLDIR)/Contents/MacOS

\$(INSTALLDIR)/Contents/Info.plist: |\$(INSTALLDIR)/Contents
	cp \$(CONTENTS)/Info.plist \$(INSTALLDIR)/Contents

\$(INSTALLDIR)/Contents/Resources/qt.conf: \$(INSTALLDIR)/Contents/Resources
	cp \$(CONTENTS)/Resources/qt.conf \$(INSTALLDIR)/Contents/Resources

\$(INSTALLDIR)/Contents/Resources: |\$(INSTALLDIR)/Contents
	cp -r \$(CONTENTS)/Resources \$(INSTALLDIR)/Contents

bundle-install: bundle \$(INSTALLDIR)/Contents/MacOS/\$(QMAKE_TARGET) \$(INSTALLDIR)/Contents/Info.plist $install_deps \$(INSTALLDIR)/Contents/Resources/qt.conf

bundle: all \$(BUNDLE_FRAMEWORKS) \$(BUNDLE_MACOS) \$(CONTENTS)/COPYING \$(DESTDIR)mxcl-is-super \$(CONTENTS)/Resources/qt.conf $bundle_deps

END
