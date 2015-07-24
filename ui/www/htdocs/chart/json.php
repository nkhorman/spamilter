<?php

	$fname = preg_replace(array('/\.\./','/\/\//'), array('','/'), $_REQUEST['map']);
	if(isset($fname) && strlen($fname))
		echo json_encode(unserialize(@file_get_contents('totals/'.$fname.'.ser')));
?>
