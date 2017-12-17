#!/usr/bin/perl -w
for ($a=0; $a< 10; $a=$a +2) {

	print "a is now $a\n";
}


$im_thinkingof =int(rand 110);
print "Pick a num\n";


print "I a thinking is =  $im_thinkingof\n";
$gusss= <STDIN>;
$in = <STDIN> ;
chomp $in;
print "in= $in\n";
chomp $guess;
print "guess = $guess\n";
if ($in == int( 4))  {
	print ":upi guess etoo high !\n";
}
elsif ($guess < $im_thingingof ) {
	print " you guess too low \n";
}
else {
	print "You got it right \n";

}
