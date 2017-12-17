#!/usr/bin/perl  -w
$_ = "pttern";
$pat = <STDIN>;
chomp $pat;
if(/$pat/) {
print "\"$_\" contain the pattern $pat \n"; # escape ""

print "$_ contain the pattern $pat \n";
}

#$_="our house is hughe";
s/p/eeroom/;
print "$_";

#  put apple and red in fruit and color
$_="apple is redaa";
($fruit, $color)=/(.*)\sis\s(.*)/; #### \s for white space--- everything-> white space-> is-> white space-> everything
print"\n$fruit\n";
print"$color\n";


# anything between surrended by white space 
/\s\w+\s/;
/\s\w{1}\s/;
print "$_";
