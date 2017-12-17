#!/usr/bin/perl -w

@words=qw( internet answers printer program );
#print words[0..3];
@guesses=();
$wrong=0;

$choice=$words[rand  @words ];
print $choice;
$hangman = "0-|--<";

@letters=split(/ /, $choice); # choice has been splited 

@hangman=split(/ /, $hangman); #split hangman in pieces

@blankword=(0) x scalar(@lettters); #which letter has guessed sucessfully
#print blankword[0];

# loop for seup,  continues looping until number of wronggues ==len of hangman
OUTER:
while ($wrong < @hangman ) {
	foreach $i (0..$#letters) {
		if ($blankword[$i]) {
			print $blankword[$i];
		}
		else {
			print "-";
		}
	}

	print "\n";

#number of wrong guesses, 
	if ($wrong) {
		print @hangman[0..$wrong-1];
	} # slice print hangman from 0, to wrong guesses

	print "\n you guess:";

	$guess=<STDIN>; 
	chomp $guess;
(@guesses) = $guess;

# wont penalize guesser
	foreach(@guesses) {
		next OUTER if ($_ eq $guess);
	}

	$guesses[@guesses] = $guess;
	$right=0;

# Meat here, array is seached letters if found set corresponding letter in blandword... 
	for ($i=0; $i<@letters; $i++) {
		if ($letters[$i] eq $guess)  {
			$blankword[$i]=$guess;
			$right=1; # flag to mark how many time need to run
		}
	}

	$wrong++ if (not $right);
	if (join(' ', @blankword) eq $choice ) {
		print "You got it right \n";
		exit;
	}
}

print "$hangman\nSorry, the word was $choice. \n";

