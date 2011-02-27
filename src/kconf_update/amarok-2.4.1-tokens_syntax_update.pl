#!/usr/bin/perl -w
#
# This script updates all format strings in amarokc config file
# according to new syntax, replaces %token with %token% if It's needed.

use strict;

sub isUpdated($) {
    return ($_[0] =~ m/%[a-zA-Z0-9]+%/);
}

sub syntaxUpdate($) {
    my $value = $_[0];
    $value =~ s/(%[a-zA-Z0-9]+)/$1%/g;
    return $value;
}

#replaceKey("section", "key", "new value");
sub replaceKey($$$) {
    my ($section, $key, $value) = @_;
    print("# DELETE $section$key\n");
    print("$section\n$key=$value\n");
}

my $section = "";

while (<>) {
    chomp();
    if (/^\[/) {
        $section = $_;
    }
    elsif (($section eq "[OrganizeCollectionDialog]" and (/^Custom Scheme/ or /^Format Presets/)) or
           ($section eq "[FilenameLayoutDialog]" and /^Custom Scheme/ )) {
        my ($key,$value) = split(/=/);
        next unless length($value);
        replaceKey($section, $key, syntaxUpdate($value)) unless isUpdated($value);
    }
}
