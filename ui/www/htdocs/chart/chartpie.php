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
require("pChart.2/pPie.class.php");

function getDataSet($ImageName, $dirs, $hash)
{
	$data = unserialize(file_get_contents($dirs['totals'].'/'.$ImageName.'.ser'));

	// Dataset definition 
	$DataSet = new pData;

	$points = array();
	$labels = array();
	$href = array();
	foreach($data as $k => $v)
	{
		$points[] = $v['count'];
		$labels[] = $k;
		//$href[] = 'href=detail.php?map='.$ImageName.'&hash='.md5($k);
		$href[] = 'onclick=chartDetailLoad("'.$ImageName.'", "'.md5($k).'");';
	}

	$DataSet->addPoints($points, 'Detail');
	if(isset($href) && count($href))
		$DataSet->addHref($href, 'Detail');
	$DataSet->addPoints($labels, 'Labels');
	$DataSet->setAbscissa('Labels');

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
		$v = 800;
		$size = array('w'=>$v, 'h'=> (($v/16) * 9 ));
		$Img = new pImage($size['w'],$size['h'], $DataSet);
		$Img->initialiseImageMap($ImageName, IMAGE_MAP_STORAGE_FILE, $ImageName, $dirs['imgmaps']);

		//$Img->drawGradientArea(0,0, $size['w'], $size['h'],DIRECTION_VERTICAL,array("StartR"=>180,"StartG"=>180,"StartB"=>180,"EndR"=>180,"EndG"=>180,"EndB"=>180,"Alpha"=>255));
		$Img->drawGradientArea(0,0, $size['w'], $size['h'],DIRECTION_VERTICAL);

		$fonts = array('Bedizen.ttf', 'GeosansLight.ttf', 'MankSans.ttf', 'calibri.ttf', 'verdana.ttf' );
		$Img->setFontProperties(array("FontName"=>$dirs['fonts'].'/'.$fonts[4],"FontSize"=>10, 'R'=>255, 'G'=>255, 'B'=>255));

		$Pie = new pPie($Img, $DataSet);
		$Pie->draw2DPie($size['w']/2, $size['h']/2,
			array(
				'Radius' => 100,
				//'DrawLabels'=>TRUE,
				//'ValuePosition' => PIE_VALUE_INSIDE,
				'LabelStacked'=>TRUE,
				'Border'=>TRUE,
				'LabelR' => 255,
				'LabelG' => 255,
				'LabelB' => 255,
				'WriteValues' => PIE_VALUE_PERCENTAGE,
				"DataGapAngle"=>10,"DataGapRadius"=>6,
				'ValuePadding' => 25,
				'RecordImageMap' => TRUE,
				)
			);
		$Pie->drawPieLegend(15,40,array("Alpha"=>20));

		$ImgCache->writeToCache($ImgHash,$Img);
		//$Img->render(getcwd().'/images/'.$ImageName);
		$Img->stroke();
	}
}

function imgmap($ImageName, $dirs)
{
	$Img = new pImage();
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
$map = $_REQUEST['map'];
$hash = $_REQUEST['hash'];
switch($action)
{
	case 'imgmap':
		imgmap($map, $dirs);
		break;
	case 'img':
		imgrender($map, getDataSet($map, $dirs, $hash), $dirs);
		break;
	default:
		html($map);
		break;
}

?>
