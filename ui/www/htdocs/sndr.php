<?php

/**********************************************************************
 *
 * Developed by;
 *	Neal Horman - neal@wanlink.com - http://www.wanlink.com
 *	Copyright (c) 2010-2015 by Neal Horman, All Rights Reserved
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

function SndrLinesWrite($fname,$tbl)
{
	$fout = fopen($fname,'w');
	if($fout)
	{
		foreach($tbl as $row)
			fprintf($fout,"%s\n",$row);
		fflush($fout);
		fclose($fout);
	}
}

function SndrLinesReadRaw($fname)
{
	$ar = array();

	$fin = fopen($fname,'r');
	if($fin)
	{
		while(!feof($fin))
			$ar[] = trim(fgets($fin));
		$q = count($ar);
		if($ar[$q-1] == "")
			unset($ar[$q-1]);
		fclose($fin);
	}

	return $ar;
}

function SndrLinesReadTrimmed($fname)
{
	$ar = array();

	$fin = fopen($fname,'r');
	if($fin)
	{
		while(!feof($fin))
		{
			$line = trim(preg_replace(array('/#.*$/','/[ \t]{1,}/'),array('',''),fgets($fin)));
			if(strlen($line))
				$ar[] = $line;
		}
		fclose($fin);
	}

	return $ar;
}

function SndrLinesParse($lines)
{
	$ar = array();
	if(count($lines))
	{
		foreach($lines as $line)
		{
			$linear = explode('|',$line);
			if(count($linear) == 3)
				$linear[3] = "";
			$ar[] = $linear;
		}
	}

	return $ar;
}

function SndrLinesShowRaw($content)
{
	echo "<pre>\n";
	if(count($content))
	{
		foreach($content as $fields)
		{
			$line = "";
			foreach($fields as $field)
			{
				if(strlen($line))
					$line = $line." | ".$field;
				else
					$line = $field;
			}
			echo htmlspecialchars($line)."\n";
		}
	}
	echo "</pre>\n";
}

function SndrLinesShowTable($content)
{
	if(count($content))
	{
		echo "<table width=\"100\" border=1 cellpading=0 cellspacing=0>\n";
		echo "<colgroup span=\"4\" width=\"25\">\n";
		echo "<thead><tr class=\"h\"><td>domain</td><td>sender</td><td>action</td><td>action&nbsp;arguments</td></tr></thead>\n";
		echo "<tbody>\n";
/*
		echo "<form>\n";
		echo "<tr><td><input type=\"text\" name=\"input[domain]\"></input></td>";
		echo "<td><input type=\"text\" name=\"input[user]\"></input></td>";
		echo "<td><input type=\"text\" name=\"input[action]\"></input></td>";
		echo "<td><input type=\"text\" name=\"input[actionarg]\"></input></td></tr>\n";
		echo "<tr><td align=\"center\" colspan=\"4\"><input type=\"submit\" value=\"Submit\" name=\"submit\"></intput></td></tr>\n";
		echo "</form>\n";
*/
		foreach($content as $fields)
		{
			$line = "";
			$fieldnum=0;
			foreach($fields as $fieldkey => $fieldstr)
			{
				if($fieldkey == 2)
				{
					switch(strtolower($fieldstr))
					{
						case 'accept':
							$lineattribs=' class="ok"';
							break;
						case 'none':
							$lineattribs=' class="neutral"';
							break;
						case 'tarpit':
							$lineattribs=' class="medium"';
							break;
						case 'reject':
						default:
							$lineattribs=' class="bad"';
							break;
					}
				}
				if(strlen($fieldstr))
					$fieldstr = htmlspecialchars($fieldstr);
				else
					$fieldstr = '&nbsp;';
				$line = $line.'<td'.$cellattribs.'>'.$fieldstr.'</td>';
			}
			echo '<tr'.$lineattribs.'>'.$line."</tr>\n";
		}
		echo "</tbody></table>\n";
	}
}

?>
