#!/usr/bin/perl
use strict;
use warnings;
use File::Find;
use Switch;
        
my $ext = shift;

find( \&find_sources, shift || '.' );

sub find_sources 
{
    if (-d) 
    {
        switch ($_)
        {
            case '.svn'   { $File::Find::prune = 1; }
            case '_build' { $File::Find::prune = 1; }
            case 'tests'  { $File::Find::prune = 1; }
        }
    }
    elsif (-f and /\.$ext$/)
    {
        $File::Find::name =~ s|^\./||g;
        print "$File::Find::name\n";
    }
}
#!/usr/bin/perl
use strict;
use warnings;
use File::Find;
use Switch;
        
my $ext = shift;

find( \&find_sources, shift || '.' );

sub find_sources 
{
    if (-d) 
    {
        switch ($_)
        {
            case '.svn'   { $File::Find::prune = 1; }
            case '_build' { $File::Find::prune = 1; }
            case 'tests'  { $File::Find::prune = 1; }
        }
    }
    elsif (-f and /\.$ext$/)
    {
        $File::Find::name =~ s|^\./||g;
        print "$File::Find::name\n";
    }
}
