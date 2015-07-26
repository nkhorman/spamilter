<?php

/**********************************************************************
 *
 * Developed by;
 *	Neal Horman - neal@wanlink.com - http://www.wanlink.com
 *	Copyright (c) 2011-2015 by Neal Horman, All Rights Reserved
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

require('varlog.php');
require('sndr.php');

?>
<html>
<style type="text/css">
body {background-color: #000000; color: #ffffff;}
body, td, th, h1, h2 {font-family: sans-serif;}
pre {margin: 0px; font-family: monospace;}
a:link {color: #cccccc; text-decoration: none; background-color: #333333;}
a:visited {color: #cccccc; text-decoration: none; background-color: #333333;}
a:hover {text-decoration: underline;}
table {border-collapse: collapse;}
.center {text-align: center;}
.center table { margin-left: auto; margin-right: auto; text-align: left;}
.center th { text-align: center !important; }
td, th { border: 1px solid #000000; font-size: 75%; vertical-align: baseline;}
h1 {font-size: 150%;}
h2 {font-size: 125%;}
.p {text-align: left;}
.h {background-color: #9999cc; font-weight: bold; color: #000000;}
.vr {background-color: #cccccc; text-align: right; color: #000000;}
.ok {background-color: #ccFFcc; color: #000000;}
.medium {background-color: #FFFFcc; color: #000000;}
.neutral {background-color: #cccccc; color: #000000;}
.bad {background-color: #FFcccc; color: #000000;}
.colorblue {background-color: #2222cc; color: #cccccc;}
img {float: right; border: 0px;}
hr {width: 600px; background-color: #cccccc; border: 0px; height: 1px; color: #000000;}
</style>
<body>
<script src="jquery-1.11.2.js" type="text/javascript"></script>
<script src="chart/imagemap.js" type="text/javascript"></script>
<script>

function chartLoad(url, srcmap, id, hash)
{
	x = url + "?map=" + srcmap;
	if(hash != '' && hash !== undefined)
		x = x + "&hash=" + hash;

	$("#map_" + id).html("<img src=\"" + x + "&action=img\" id=\"" + id +"\" alt=\"\" class=\"pChartPicture\"/>");
	addImage(id, id + "Map", x + "&action=imgmap");
}

function chartDetailLoad(srcmap, hash)
{
	chartLoad("chart/chartpie.php", srcmap, "detail", hash);
}

function chartDrilldownLoad(srcmap)
{
	$('#map_detail').empty();
	chartLoad("chart/chartpie.php", srcmap, "drilldown");
}

function chart30dayLoad(srcmap)
{
	chartLoad("chart/chart30.php", srcmap, "30day");
}

function detailLoad(map, key)
{
	$.getJSON('chart/json.php?map=' + map, function(data)
	{
		$('#').empty();
	});
}

var domTotals = setInterval(function()
{
	clearInterval(domTotals);
	$.getJSON('totals.php', function(data) {});
}, 10);

var domStatus = setInterval(function()
{
	$.getJSON('totals.php?action=status', function(data)
	{
		var status = data.status;
		if(status == "OK")
		{
			clearInterval(domStatus);
			$('#status').empty();
			chart30dayLoad("30day");
		}
		else
			$('#status').text(status);
	});
}, 250);

</script>
<?php

function sndrFilter($val)
{ 
	global $sndrFilterVal;

	$vdom = ltrim(rtrim($val[0]));
	$vmbox = ltrim(rtrim($val[1]));
	$edom = $sndrFilterVal[1];

	return ((strcasecmp($vdom,$edom) == 0 || strcasecmp($vdom,'.'.$edom) == 0) && ($vmbox == "" || strcasecmp($vmbox,$sndrFilterVal[0]) == 0));
}
	$dirpath = '/var/log';
	$log = $_REQUEST['log'];
	$lognum = $log-1;
	$sndrtblfname = "/var/db/spamilter/db.sndr";
	$sndrtbl = SndrLinesParse(SndrLinesReadTrimmed($sndrtblfname));

	$sndrFilterVal = array();
	$page = $_REQUEST['page'];

	switch($page)
	{
		case 'log':
			if($log == 0)
				$files = array('spam.log');
			else
				$files = array('spam.log.'.$lognum.'.gz');

			$content = LogLinesParse(LogLinesRead($dirpath,$files));
			LogLinesShowTable($content,$log,$sndrtbl);
			break;

		case 'filter':
			$sndr = htmlspecialchars_decode($_REQUEST['sndr']);
			$action = $_REQUEST['action'];
			$sndrtbl = SndrLinesReadRaw($sndrtblfname);
			$e = preg_split('/@/',$sndr);
			switch($action)
			{
				case 'accept':
				case 'reject':
					$dt = date("D M j G:i:s T Y");
					$user = $_SERVER['REMOTE_USER'].'/'.$_SERVER['PHP_AUTH_USER'];
					$ip = $_SERVER['SERVER_ADDR'];
					$sndrtbl[] = implode('|',array($e[1],$e[0],ucfirst($action),'# '.$user.' - from '.$ip.' on '.$dt));
					SndrLinesWrite($sndrtblfname,$sndrtbl);
					break;
				case 'remove':
					$sndrFilterVal = $e;
					$removear = array_filter(SndrLinesParse($sndrtbl),"sndrFilter");
					unset($sndrtbl[key($removear)]);
					SndrLinesWrite($sndrtblfname,$sndrtbl);
					break;
			}
			$sndrtbl = SndrLinesParse(SndrLinesReadTrimmed($sndrtblfname));
			//break;
		case 'sndr':
			$sndr = htmlspecialchars_decode($_REQUEST['sndr']);
			if(strlen($sndr))
			{
				$e = preg_split('/@/',$sndr);
				foreach($sndrtbl as $k => $v)
				{
					$email = $v[1]."@".$v[0];
					if($email == $sndr || $v[0] == $e[1] || $v[0] == '.'.$e[1])
					{
						$sndrtbl = array($k => $v);
						break;
					}
				}
			}
			SndrLinesShowTable($sndrtbl);
			break;
		case 'detail':
			if($log == 0)
				$files = array('spam.info');
			else
				$files = array('spam.info.'.$lognum.'.gz');

			$content = LogDetailLinesParse(LogDetailLinesRead($dirpath,$files,$_REQUEST['k']));
			LogDetailLinesShowTable($content);
			break;
		default:
?>
<div id="status"></div>
<div id="map_30day"></div>
<div id="map_drilldown"></div>
<div id="map_detail"></div>
<div id="links">
<a href="?page=sndr">The Black/White list</a><br>
<a href="?page=log">Activity log - Today</a><br>
<?php

			$files = FileListBuild($dirpath,"spam.log");
			$lognum = 0;
			foreach($files as $file)
			{
				$lognum++;
				if($lognum == 1)
					$when = 'Yesterday';
				else
					$when = $lognum.' days ago';
				$l = $lognum;
				echo "<a href=\"?page=log&log=".$l."\">Activity log - ".$when."</a><br>\n";
			}
			break;
	}
?>
</div>
<div id="detail"></div>
</body></html>
