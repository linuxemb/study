print "savedata to what file?" ;
$filename =<STDIN>;
chomp $filenam;

warn "$file contents will be overwritten\n";warn "$file last updated ", -M $filename, "days ago"\n";
if (-s $filename) {
warn "$file contents will be overwritten\n";#warn "$file last updated ", -M $filename, "days ago"\n";
}
