#!/usr/bin/perl -w


# SORT 
#sub sort($x, $y) {
@sorted =sort { return(1) if ($a>$b);
}
return(0) if ($a==$b);
return (-1) if($a<$b);
} @numbers;



print $sorted;
# Equal to dimont 

@sourted = sort { $a<=>$b;} @numbers;

#  REcording your arrat
@lines =qw( I do not like grenn eggs and ham);
print join(' ',  @lines);
print join('= ', reverse @lines);
print join('- ', sort  @lines);

####################33
@music = ('while','whelo');
foreach $record (@music) {

	($record, $artist) = split(',', $record);
}

