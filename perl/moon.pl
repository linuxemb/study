
#!/usr/bin/perl  -w

print "your weight";
$_=<STDIN>;
chomp;

s/^\s+//;  #remove leading space


# take area code.
$_="800-133-1212";
if (/(\d{3})-(\d{3})-(\d{4})/) {
#if m/(\d{3})-(\d{3})-(\d{4})/) {
s/(\d{3})-(\d{3})-(\d{4})/r2-r2-1r/;

# /(\d{3})-(\d{3})-(\d{4})/; 
print "The area code is $1";
print "The phone  code is $2";
print "The areaiik code is $3";

}

#####grep expressing ,list
#grep block list
# found all hound 
@dogs=qw(greyhound bloodhound terrier mutt chihuahua);
@hounds=grep /hound/,@dogs;
print "\n@hounds\n";

###substitude hound --> hunds
@hounds=grep s/hound/hounds/, @dogs;
print "\n@dogs\n";

##  grep combine with other funs
@longdogs=grep length($_)>8, @dogs;

print "\n@longdogs\n";


############map funs == grep except return valure from the express of block form map not valurof $_
@input = qw{i and you};
@words = map {split ' ', $_} @input;
print "\n@words\n";


##### exchange x and y
$_=qw(x=y);
#s/(.+)=(.+)/$2=$1/;
s/(.*)=(.*)/$2=$1/;
print "\n$_\n";


######### bind operator
$foo = "star wars: The Phantom Menace";

$foo=~/star\s((wars): The Phantom Menace)/i;
print "$1\n"; # ??? why the whole ???
print "$2\n"; # wars
print "$3\n"; #empty`

