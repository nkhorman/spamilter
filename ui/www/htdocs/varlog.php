<?php

/**********************************************************************
 *
 * Developed by;
 *	Neal Horman - neal@wanlink.com - http://www.wanlink.com
 *	Copyright by Neal Horman. All Rights Reserved
 *
 *      License;
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions
 *      are met:
 *      1. Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *      2. Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *      3. Neither the name Neal Horman nor the names of any contributors
 *         may be used to endorse or promote products derived from this software
 *         without specific prior written permission.
 *
 *      Disclamer;
 *      THIS SOFTWARE IS PROVIDED BY NEAL HORMAN AND ANY CONTRIBUTORS ``AS IS'' AND
 *      ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *      IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *      ARE DISCLAIMED.  IN NO EVENT SHALL NEAL HORMAN OR ANY CONTRIBUTORS BE LIABLE
 *      FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *      DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *      OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *      HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *      LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *      OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *      SUCH DAMAGE.
 *
 **********************************************************************/

function natrcmp($a,$b)
{
	return strnatcmp($a,$b) * -1;
}

function FileListBuild($dirpath,$filename)
{
	$dirents = dir($dirpath);
	$pat = array(';\/;');
	$rep = array('\\\/');
	$dirpathpat = preg_replace($pat,$rep,$dirpath.'/');
	$ar = array();
	while(($dirent = $dirents->read()) != false)
	{
		$file = $dirpath."/".$dirent;
		if(is_file($file))
		{
			// strip the dirpath
			$pat = array(';'.$dirpathpat.';');
			$rep = array('');
			$fstr = preg_replace($pat,$rep,$file);
			// save the file name
			if(isset($fstr) && strlen($fstr) && strstr($fstr,$filename) && strcmp($fstr,$filename) != 0)
				$ar[] = $fstr;
		}
	}

	if(count($ar))
		usort($ar,"strnatcmp");

	return $ar;
}


function LogLinesRead($dirpath,$files)
{
	$content = array();
	foreach($files as $file)
	{
		$fname = $dirpath.'/'.$file;
		$fin = gzopen($fname,'r');
		if($fin)
		{
			$lastline = "";
			while(!gzeof($fin))
			{
				$line = trim(gzgets($fin,4096));
				if(strlen($line) > 0)
				{
					if(strstr($line,"last message repeated "))
					{
						// parse out the time and the number of times to repeat
						$pat=array('/^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t].*last message repeated ([0-9]{1,}) times/');
						$rep=array('\1|\2');
						list($repeattime,$repeattimes) = explode('|',preg_replace($pat,$rep,$line));

						// split the time from the body
						$pat=array('/^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t]{0,1}(.*)$/');
						$rep=array('\1|\2');
						list($lasttime,$lastbody) = explode('|',preg_replace($pat,$rep,$lastline));
					
						// repeat last message x times	
						for($i=0; $i<$repeattimes; $i++)
							$content [] = $repeattime." ".$lastbody;
					}
					else
					{
						$content[] = $line;
						$lastline = $line;
					}
				}
			}
			gzclose($fin);
		}
	}

	return $content;
}

function LogLinesParse($lines)
{
	$ar = array();
	if(count($lines))
	{
		$keys = array('time','status','sessionid','from','to','result');
		foreach($lines as $line)
		{
			if(!strstr($line,"logfile turned over"))
			{
				$pat = array(
					';\|;',
					';^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t](.*]:)[ \t]([^ \t]*)[ \t]([^ \t]*)[ \t](<[^>]{1,}>)[ \t](<[^>]{1,}>)[ \t]{0,1}(.*)$;',
					';^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t](.*]:)[ \t]([^ \t]*)[ \t]([^ \t]*)[ \t]([^ \t]{1,})[ \t]([^ \t]{1,})[ \t]{0,1}(.*)$;',
					';^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t](.*]:)[ \t]{0,1}(.*)$;',
				);
				$rep = array(
					'-',
					'\1|\3|\4|\5|\6|\7',
					'\1|\3|\4|\5|\6|\7',
					'\1|||||\3',
				);
				$line = preg_replace($pat,$rep,$line);
				$fields = explode('|',$line);
				foreach($fields as $k => $v)
					$f[$keys[$k]] = $v;
				$f['filter'] = '';
				$ar[$fields[0]] = $f;
			}
		}

		if(count($ar))
			uksort($ar,"natrcmp"); // reverse natural sort order
	}

	return $ar;
}


function LogDetailLinesRead($dirpath,$files,$key)
{
	$content = array();
	foreach($files as $file)
	{
		$fin = gzopen($dirpath.'/'.$file,'r');
		if($fin)
		{
			$lastline = "";
			while(!gzeof($fin))
			{
				$line = trim(gzgets($fin,16384));
				if(strlen($line) > 0 && strpos($line,'['.$key.']') !== false)
				{
					if(strstr($line,"last message repeated "))
					{
						// parse out the time and the number of times to repeat
						$pat=array('/^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t].*last message repeated ([0-9]{1,}) times/');
						$rep=array('\1|\2');
						list($repeattime,$repeattimes) = explode('|',preg_replace($pat,$rep,$line));

						// split the time from the body
						$pat=array('/^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t]{0,1}(.*)$/');
						$rep=array('\1|\2');
						list($lasttime,$lastbody) = explode('|',preg_replace($pat,$rep,$lastline));
					
						// repeat last message x times	
						for($i=0; $i<$repeattimes; $i++)
							$contenet [] = $repeattime." ".$lastbody;
					}
					else
					{
						$content[] = $line;
						$lastline = $line;
					}
				}
			}
			gzclose($fin);
		}
	}

	return $content;
}

function LogDetailLinesParse($lines)
{
	$ar = array();
	if(count($lines))
	{
		foreach($lines as $line)
		{
			if(!strstr($line,"logfile turned over"))
			{
				$pat = array(
					';\|;',
					';^([a-zA-Z]{1,3}[ \t]{1,2}[0-9]{1,2}[ \t][0-9]{1,2}:[0-9]{1,2}:[0-9]{1,2})[ \t](.*]:)[ \t]{0,1}\[([0-9a-zA-Z]{32})\][ \t](.*)$;',
				);
				$rep = array(
					'-',
					'\1|\3|\4',
				);
				$line = preg_replace($pat,$rep,$line);
				$fields = explode('|',$line);
				$ar[] = $fields;
			}
		}
	}

	return $ar;
}

function LogDetailLinesShowTable($content)
{
	if(count($content))
	{
		echo "<table width=\"100\" border=1 cellpading=0 cellspacing=0>\n";
		echo "<colgroup>\n";
		echo " <col span=\"2\" width=\"100\">\n";
		echo " <col span=\"1\" width=\"1000\">\n";
		echo "</colgroup>\n";
		echo "<thead><tr class=\"h\"><td>date</td><td>session id</td><td>detail</td></tr></thead>\n";
		echo "<tbody>\n";
		foreach($content as $fields)
		{
			$line = "";
			$fieldnum=0;
			$attribs = "";
			foreach($fields as $fieldstr)
			{
				if(strlen($fieldstr))
					$fieldstr = preg_replace('/ /','&nbsp;'," ".htmlspecialchars($fieldstr)." ");
				else
					$fieldstr = '&nbsp;';
				$line = $line.'<td>'.$fieldstr.'</td>';
			}
			echo '<tr>'.$line."</tr>\n";
		}
		echo "</tbody></table>\n";
	}
}

/*
function LogLinesShowRaw($content)
{
	echo "<pre>\n";
	if(count($content))
	{
		foreach($content as $fields)
		{
			$line = "";
			foreach($fields as $field)
				$line = $line." | ".$field;
			echo htmlspecialchars($line)."\n";
		}
	}
	echo "</pre>\n";
}
*/

function SndrTblXform($sndrtbl)
{
	foreach($sndrtbl as $k => $v)
		$ar[$v[1]."@".$v[0]] = $v[2];

	if(count($ar))
		ksort($ar);

	return $ar;
}

function LogLinesShowTable($content,$log,$sndrtbl)
{
	if(count($content))
	{
		echo "<table width=\"100\" border=1 cellpading=0 cellspacing=0>\n";
		echo "<colgroup span=\"1\" width=\"40\">\n";
		echo "<colgroup span=\"4\" width=\"20\">\n";
		echo "<thead><tr class=\"h\"><td>Date</td><td>Status</td><td>Filter Action</td><td>Session id</td><td>From</td><td>To</td><td>Result</td></tr></thead>\n";
		echo "<tbody>\n";

		$emails = SndrTblXform($sndrtbl);
		$keys = array('time','status','filter','sessionid','from','to','result');
		foreach($content as $fields)
		{
			$line = "";
			$attribs = "";
			$addrfrom = preg_replace(array('/^</','/>$/'),array('',''),$fields['from']);
			$filtertype='';
			$class='';
			foreach($keys as $k)
			{
				$fieldstr = $fields[$k];
				$cellattribs = "";
				if(strlen($fieldstr))
				{
					switch($k)
					{
						case 'time': // time
							$fieldstr = preg_replace('/ /','&nbsp;',$fieldstr);
							break;

						case 'status': // status
							$status = strtolower($fieldstr);
							switch($status)
							{
								case 'accepted':
									$class = "ok";
									$filtertype = 'reject';
									break;
								case 'rejected':
									$class = "bad";
									$filtertype = 'accept';
									break;
								case 'tempfailed':
									$class = "medium";
									$filtertype = 'accept';
									break;
							}
							$e = preg_split('/@/',$addrfrom);
							if(strlen($emails[$addrfrom]) || strlen($emails['@'.$e[1]]) || strlen($emails['@.'.$e[1]]))
							{
								$cellattribs = " class=\"colorblue\" ";
								$fieldstr = "<a href=\"?page=sndr&sndr=".htmlspecialchars($addrfrom)."\">".$fieldstr."</a>";
								$filtertype = 'remove';
							}

							if(isset($class))
							{
								$lineattribs = " class=\"".$class."\"";
								$cellattribs = $cellattribs." align=\"center\"";
							}
							break;

						case 'sessionid': // session id
							$x = htmlspecialchars($fieldstr);
							if(strlen($fieldstr) == 32)
								$fieldstr = '<a href="?log='.$log.'&page=detail&k='.$x.'">'.$x.'</a>';
							else
								$fieldstr = $x;
							break;

						case 'from': // from
							if(strlen($addrfrom))
								$fieldstr = htmlspecialchars($addrfrom);
							else
								$fieldstr = '&nbsp';
							break;

						case 'to': // to
							$addr = preg_replace(array('/^</','/>$/'),array('',''),$fieldstr);
							$fieldstr = htmlspecialchars($addr);
							break;
						//case 'result': // result
						default:
							$fieldstr = htmlspecialchars($fieldstr);
							break;
					}
				}
				else
				{
					if($k == 'filter')
					{
						$s = '<a href="?page=filter&action='.$filtertype.'&sndr='.htmlspecialchars($addrfrom).'">'.$filtertype.'</a>';
						$fieldstr = '&nbsp;'.$s.'&nbsp;';
					}
					else
						$fieldstr = '&nbsp;';
				}
				$line = $line.'<td'.$cellattribs.'>'.$fieldstr.'</td>';
			}
			echo '<tr'.$lineattribs.'>'.$line."</tr>\n";
		}
		echo "</tbody></table>\n";
	}
}
?>
