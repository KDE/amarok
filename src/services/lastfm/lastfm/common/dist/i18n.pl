#!/usr/bin/perl
#run from ..

use File::Basename;


opendir( DIR, "i18n" );
@files = grep( /\.ts$/, readdir( DIR ) );
closedir( DIR );

mkdir ( "bin/data/i18n" );

foreach $file (@files) {
   my $name = basename( $file, ".ts" );
   system( "lrelease i18n/$file -qm bin/data/i18n/$name.qm" );
}


print "====> qm files will be found in bin/data/i18n/\n";
#!/usr/bin/perl
#run from ..

use File::Basename;


opendir( DIR, "i18n" );
@files = grep( /\.ts$/, readdir( DIR ) );
closedir( DIR );

mkdir ( "bin/data/i18n" );

foreach $file (@files) {
   my $name = basename( $file, ".ts" );
   system( "lrelease i18n/$file -qm bin/data/i18n/$name.qm" );
}


print "====> qm files will be found in bin/data/i18n/\n";
