<?php
 header("Content-type: text/xml");
$names = array (
        "John Smith", "John Jon", "lisa shi");
 echo "<?xml version=\"1.0\" ?>\n";
 echo "<names>\n";
 while (list($k,$v)=each($names)) {

 		echo "<name>$v</name>\n";
 	if(stristr($v, $_GET['query'])) {
 		echo "<name>$v</name>\n";
 	}
 }
 echo "</names>\n";
?>
