<?php

/**********************************************************************
 *
 * Developed by;
 *	Neal Horman - neal@wanlink.com - http://www.wanlink.com
 *	Copyright (c) 2011-2015 by Neal Horman. All Rights Reserved
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

class totalsLog
{
	protected $logpath = '/var/log';
	protected $dstpath = '/usr/local/apache2/apps/spamilter/htdocs/chart';
	protected $reasons = array('Accepted', 'Rejected', 'TempFailed');

	function buildOne($src)
	{
		$log = LogLinesParse(LogLinesRead($this->logpath, array($src)));
		foreach($this->reasons as $v)
			$totalsByReason[$v] = 0;

		$totalsReject = array();
		$totalsTempFailed = array();
		foreach($log as $k => $v)
		{
			@$totalsByReason[$v['status']]++;
			switch($v['status'])
			{
				case 'Rejected':
					$result = $v['result'];
					$s = preg_split("/'/", $result);
					$k = rtrim($s[0]);
					$s = @preg_split("/".$k."/", $result);
					@$totalsReject[$k]['count']++;
					if($s[1] == "")
					{
						switch($k)
						{
							case 'Sender address verification':
								@$totalsReject[$k][$v['from']]++;
								break;
							default:
								@$totalsReject[$k][$v['to']]++;
								break;
						}
					}
					else
						@$totalsReject[$k][$s[1]]++;
					break;

				case 'TempFailed':
					$result = $v['result'];
					@$totalsTempFailed[$result]['count']++;
					switch($k)
					{
						case 'Sender address verification':
							@$totalsTempFailed[$result][$v['to']]++;
							break;
						default:
							@$totalsTempFailed[$result][$v['from']]++;
							break;
					}
					break;
			}
		}

		return array('reason'=>$totalsByReason, 'reject'=>$totalsReject, 'tempfailed'=>$totalsTempFailed);
	}

	function status($callback, $status)
	{
		if(isset($callback))
		{
			if(
				(is_array($callback) && method_exists($callback[0], $callback[1]))
				|| (is_string($callback) && function_exists($callback))
				)
			{
				call_user_func($callback, $status);
			}
		}
	}

	function buildAll($days, $statusCallback)
	{
		$totalsByDayReason = array();
		for($day= 0; $day < $days; $day++)
		{
			$file = 'spam.log.'.$day.'.gz';
			$this->status($statusCallback, 'Building Totals '.($day+1).' of '.$days);
			$totals = $this->buildOne($file);

			$totalsByDayReason[$day] = $totals['reason'];
			file_put_contents($this->dstpath.'/totals/reject.'.$day.'.ser', serialize($totals['reject']));
			file_put_contents($this->dstpath.'/totals/tempfailed.'.$day.'.ser', serialize($totals['tempfailed']));
			$this->status($statusCallback, 'Totals '.($day+1).' of '.$days.' done');
		}

		file_put_contents($this->dstpath.'/totals/30day.ser', serialize($totalsByDayReason));
	}

	public function build($statusCallback)
	{
		ob_start();
		system('ls /var/log/spam.log.*.gz');
		$out = explode("\n",ob_get_contents());
		ob_end_clean();
		while($out[count($out)-1] == "")
			unset($out[count($out)-1]);

		$this->buildAll(count($out), $statusCallback);
	}
}

class totalsStatus
{
	protected $cacheFname = 'totalscache.ser';
	protected $sentinelFname = '/var/log/spam.log.0.gz';
	protected $statusFname = 'totalsstatus.ser';

	function currentGet()
	{
		ob_start();
		system('if [ -e '.$this->sentinelFname.' ]; then `which md5` -q '.$this->sentinelFname.' | `which tr` -d \'\\n\'; else echo""; fi;');
		$out = ob_get_contents();
		ob_end_clean();

		return $out;
	}

	function cacheGet()
	{
		return unserialize(@file_get_contents($this->cacheFname));
	}

	function cachePut($ob)
	{
		@file_put_contents($this->cacheFname, serialize($ob));
	}

	function statusGet()
	{
		header('Content-Type: application/json');
		echo @file_get_contents($this->statusFname);
	}

	function statusPut($status, $callback = NULL)
	{
		if(isset($callback))
		{
			if(
				(is_array($callback) && method_exists($callback[0], $callback[1]))
				|| (is_string($callback) && function_exists($callback))
				)
			{
				$status = call_user_func($callback);
			}
		}

		@file_put_contents($this->statusFname, json_encode(array('status'=>$status)));
		//echo json_encode(array('status'=>$status))."\n";
	}

	public function update($callback)
	{
		$md5 = $this->currentGet();
		$ob = $this->cacheGet();
		if(!isset($ob) || !isset($md5) || count($ob) == 0 || !isset($ob['md5']) || $ob['md5'] != $md5)
		{
			$this->statusPut('OLD');
			if(isset($callback))
			{
				if(
					(is_array($callback) && method_exists($callback[0], $callback[1]))
					|| (is_string($callback) && function_exists($callback))
					)
				{
					call_user_func($callback, array($this, 'statusPut'));
				}
			}
			$this->cachePut(array('md5'=>$md5));
			$this->statusPut('OK');
		}
	}
}

	$totalStatus = new totalsStatus();
	switch($_REQUEST['action'])
	{
		case 'status':
			$totalStatus->statusGet();
			break;
		default:
			$totalStatus->update(array(new totalsLog(),'build'));
			break;
	}

?>
