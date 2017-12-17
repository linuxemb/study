
#!/usr/bin/perl -w
open(FILE,"a.txt") || die "openfile $!";
# 1st way to print a file
#@contents=<MYFILE>;
#print $contents[0];
#print $contents[1];
#print $contents[2];



# 2nd way to print a file not workin ????
#while(<MYFLIE>)
#{
#print $_;
#}

# 3rd way working
while(defined($a=<FILE>))
{
print $a;
}

# Write to file
open(FH, ">>log.txt") || die "$!";
if (!print FH "my log time at ", scalar(localtime), "\n")  {

warn "Unable to write to file : $!";
}

close (FH);


# bin mode
open(BFH, "../Pictures/deer6.jpg") || die "$!";
binmode(BFH);
print BFH "GIF87a\056\001\045\015\000";
close( BFH);



 # Test file open
print " save data to which file? ";

$filename= <STDIN>;
chomp  $filename;
print "$filename";
if ( -e $filename ) {
warn " $filename contents will be overwritten !\n";
warn "$filename  was last updated  ", -M $filename, "days ago \n";
}
