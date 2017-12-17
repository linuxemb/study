#!/usr/bin/perl -w

open(SOURCE, "sourcefile") || die "$!";
open(DEST,">destination") || die "$!";
@contents=<SOURCE>;
print DEST @contents;

close(DEST);
close(SOURCE);
