<?php

$pwd = getcwd();
$dirs = array();
$dirs['pwd'] = $pwd;
$dirs['imgmaps'] = $pwd.'/imgmaps';
$dirs['fonts'] = $pwd.'/fonts';
$dirs['cache'] = $pwd.'/cache';
$dirs['totals'] = $pwd.'/totals';

require("pChart.2/pData.class.php");
require("pChart.2/pDraw.class.php");
require("pChart.2/pImage.class.php");
require("pChart.2/pCache.class.php");

function getDataSet($ImageName, $dirs)
{
	$totalsByDayReason = unserialize(file_get_contents($dirs['totals'].'/'.$ImageName.'.ser'));
	$reasons = array('Accepted', 'Rejected', 'TempFailed');

	// Dataset definition 
	$DataSet = new pData;

	// get the totals for each reason by day
	foreach($reasons as $k => $reason)
	{
		$points = array();
		$href = array();
		foreach($totalsByDayReason as $dayk => $dayv)
		{
			$points[$dayk] = $dayv[$reason];
			switch($reason)
			{
				case 'Rejected':
					//$href[$dayk] = 'href=chart/chartpie.php?map=reject.'.$dayk;
					$href[$dayk] = 'onclick=chartDrilldownLoad("reject.'.$dayk.'");';
					break;
				case 'TempFailed':
					//$href[$dayk] = 'href=chart/chartpie.php?map=tempfailed.'.$dayk;
					$href[$dayk] = 'onclick=chartDrilldownLoad("tempfailed.'.$dayk.'");';
					break;
			}
		}

		$DataSet->addPoints($points, $reason);
		if(isset($href) && count($href))
			$DataSet->addHref($href, $reason);
	}

	// label the days
	$points = array();
	foreach($totalsByDayReason as $dayk => $dayv)
		$points[$dayk] = ($dayk + 1);
	$DataSet->addPoints($points, "Labels");

	$DataSet->setAxisName(0,"Emails");
	$DataSet->setAbscissa("Labels");
	$DataSet->setAbscissaName("Days Past");
	$DataSet->setPalette('Accepted', array('R'=>178,'G'=>255,'B'=>78,'Alpha'=>255));
	$DataSet->setPalette('Rejected', array('R'=>255,'G'=>78,'B'=>78,'Alpha'=>255));
	$DataSet->setPalette('TempFailed', array('R'=>255,'G'=>255,'B'=>78,'Alpha'=>255));

	return $DataSet;
}

function imgrender($ImageName, $DataSet, $dirs)
{
	// Initialise the graph
	$ImgCache = new pCache(array('CacheFolder'=>$dirs['cache']));
	$ImgHash = $ImgCache->getHash($DataSet);

//	if ($ImgCache->isInCache($ImgHash) && file_exists($dirs['imgmaps'].'/'.$ImageName.'.map'))
//		$ImgCache->strokeFromCache($ImgHash);
//	else
	{
		//$size = array('w'=>1300, 'h'=>600);
		$v = 800;
		$size = array('w'=>$v, 'h'=> (($v/16) * 9 ));
		$Img = new pImage($size['w'],$size['h'], $DataSet);
		$Img->initialiseImageMap($ImageName, IMAGE_MAP_STORAGE_FILE, $ImageName, $dirs['imgmaps']);

		//$Img->drawGradientArea(0,0, $size['w'], $size['h'],DIRECTION_VERTICAL,array("StartR"=>180,"StartG"=>180,"StartB"=>180,"EndR"=>180,"EndG"=>180,"EndB"=>180,"Alpha"=>255));
		$Img->drawGradientArea(0,0, $size['w'], $size['h'],DIRECTION_VERTICAL);

		$fonts = array('Bedizen.ttf', 'GeosansLight.ttf', 'MankSans.ttf', 'calibri.ttf', 'verdana.ttf' );
		$Img->setFontProperties(array("FontName"=>$dirs['fonts'].'/'.$fonts[4],"FontSize"=>10, 'R'=>255, 'G'=>255, 'B'=>255));

		// Draw the scale and the chart
		$Img->setGraphArea(60,20, $size['w']-20, $size['h']-40);
		$Img->drawScale(array("DrawSubTicks"=>TRUE,"Mode"=>SCALE_MODE_ADDALL_START0));
		$Img->setShadow(FALSE);
		$Img->drawStackedBarChart(array("Surrounding"=>-15, "InnerSurrounding"=>15, 'RecordImageMap'=>TRUE));

		// Write a label
		//$Img->writeLabel($reasons,0,array("DrawVerticalLine"=>TRUE, 'NoTitle'=>TRUE, 'FontSize'=>7));
		//$Img->writeLabel($reasons,14,array("DrawVerticalLine"=>TRUE, 'NoTitle'=>TRUE, 'FontSize'=>7));
		//$Img->writeLabel($reasons,30,array("DrawVerticalLine"=>TRUE, 'NoTitle'=>TRUE, 'FontSize'=>7));

		// Write the chart legend
		$Img->drawLegend(60, $size['h']-20, array("Style"=>LEGEND_NOBORDER, "Mode"=>LEGEND_HORIZONTAL));

		$ImgCache->writeToCache($ImgHash,$Img);
		//$Img->render(getcwd().'/images/'.$ImageName);
		$Img->stroke();
	}
}

function imgmap($ImageName, $dirs)
{
	$Img = new pImage(1,1);
	$Img->dumpImageMap($ImageName, IMAGE_MAP_STORAGE_FILE, $ImageName, $dirs['imgmaps']);
}

function html($map)
{
	$self = $_SERVER["PHP_SELF"];
	echo "<html> <head></head> <body>\n";
	echo "<script src=\"imagemap.js\" type=\"text/javascript\"></script>\n";
	echo "<img src=\"".$self."?map=".$map."&action=img\" id=\"testPicture\" alt=\"\" class=\"pChartPicture\"/>\n";
	echo "<script>\n";
	echo "addImage(\"testPicture\",\"pictureMap\",\"".$self."?map=".$map."&action=imgmap\");\n";
	echo "</script>\n";
	echo "</body>\n";
}

$action = $_REQUEST['action'];
$map = "30day";
switch($action)
{
	case 'imgmap':
		imgmap($map, $dirs);
		break;
	case 'img':
		imgrender($map, getDataSet($map, $dirs), $dirs);
		break;
	case 'data':
		print_r(getDataSet($map, $dirs)->getData());
		break;
	default:
		html($map);
		break;
}

?>
