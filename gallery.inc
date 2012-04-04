<?php

// The settings defined below should not be modified from within this file, but should instead be
//  defined in a custom file which then includes this file. The below defines will then be skipped
//  as they have already been defined

/**
 * Include thumbnail image data directly in the HTML using <img src="data:image/jpeg;base64,......"/> tags.
 * If disabled, standard <img src="?thumb=..."/> tags will be used instead.
 * 
 * Data URIs aren't supported by older browsers, but using them significantly reduces the number of HTTP requests required to load a page
 */
@define('USE_DATA_URIS', true);

/**
 * Rotate images, as required by their orientation flag, using CSS transforms
 * 
 * Only one of CSS_ROTATION or PHP_ROTATION should be used
 */
@define('CSS_ROTATION', false);

/**
 * Rotate images, as required by their orientation flag, using PHP's gd function
 * 
 * Only one of CSS_ROTATION or PHP_ROTATION should be used
 */
@define('PHP_ROTATION', true);

/**
 * How long cached geocode data will be considered valid, after which it will be reloaded from Google
 * 
 * Defaults to 2678400, or 31 days
 */
@define('GEOCODE_CACHE_TIMEOUT', 2678400);

/**
 * Whether to gzip compress the geocode data cache files.
 * 
 * These cache files are surprisingly large (around 15 kB each) and are highly compressible (generally reduced to less than 10% of their original size) so leaving this enabled is recommended unless CPU resources are costly while there is a surplus of disk space
 */
@define('GEOCODE_GZIP_CACHE', true);

class Formatting {
    public static function formatFilesize($bytes, $decimalPlaces=2, $includeTrailingZeroes=false, $useBinaryPrefixesForFilesizes=true) {
        if ($useBinaryPrefixesForFilesizes) {
            $suffixes = array(    'B',
                                'KiB',
                                'MiB',
                                'GiB',
                                'TiB',
                                'PiB',
                                'EiB',
                                'ZiB',
                                'YiB'
                                );
        } else {
            $suffixes = array(   'B',
                                'KB',
                                'MB',
                                'GB',
                                'TB',
                                'PB',
                                'EB',
                                'ZB',
                                'YB'
                                );
        }
        $suffix = 0;
        while($bytes >= 1024) {
            $bytes /= 1024;
            $suffix++;
        }
        $r = number_format(round($bytes, $decimalPlaces), $decimalPlaces, '.', ',');
        if (!$includeTrailingZeroes) {
            if (strpos($r, '.') !== false) {
                //Trim off trailing zeroes, then the trailing decimal point, if necessary (check
                // for the existence of a decimal point first, or we can end up stripping trailing
                // zeroes BEFORE the decimal point, thus completely altering the displayed value!)
                $r = rtrim(rtrim($r, '0'), '.');
            }
        }
        return $r . ' ' . $suffixes[$suffix];
    }
}

class NoExifDataException extends Exception {}

class Coordinates {
    private $latRef;
    private $latDeg;
    private $latMin;
    private $latSec;
    private $lonRef;
    private $lonDeg;
    private $lonMin;
    private $lonSec;
    
    private $latDecimal;
    private $lonDecimal;
    
    public function __construct($latRef, $latDeg, $latMin, $latSec, $lonRef, $lonDeg, $lonMin, $lonSec) {
        $this->latRef = $latRef;
        $this->latDeg = $latDeg;
        $this->latMin = $latMin;
        $this->latSec = $latSec;
        $this->lonRef = $lonRef;
        $this->lonDeg = $lonDeg;
        $this->lonMin = $lonMin;
        $this->lonSec = $lonSec;
        
        $this->calculateDecimalPair();
    }
    
    private static function parseFraction($string) {
        return eval("return $string;");
    }
    
    public static function fromExifArrays($latRef, array $lat, $lonRef, array $lon) {
        return new Coordinates(
            $latRef, 
            self::parseFraction($lat[0]),
            self::parseFraction($lat[1]),
            self::parseFraction($lat[2]),
            $lonRef,
            self::parseFraction($lon[0]),
            self::parseFraction($lon[1]),
            self::parseFraction($lon[2])
        );
    }
          /*
          ["GPSVersion"]=> string(3) ""
          ["GPSLatitudeRef"]=> string(1) "N"
          ["GPSLatitude"]=> array(3) {
                                [0]=> string(4) "51/1"
                                [1]=> string(4) "17/1"
                                [2]=> string(7) "164/100"
          ["GPSLongitudeRef"]=> string(1) "W"
          ["GPSLongitude"]=> array(3) {
                                [0]=> string(3) "1/1"
                                [1]=> string(3) "4/1"
                                [2]=> string(8) "5982/100"
          ["GPSAltitudeRef"]=> string(1) ""
          ["GPSAltitude"]=> string(3) "0/1"
          ["GPSTimeStamp"]=> array(3) {
                                [0]=> string(4) "14/1"
                                [1]=> string(4) "56/1"
                                [2]=> string(4) "12/1"
          ["GPSMapDatum"]=> string(6) "WGS-84"
          ["GPSProcessingMode"]=> string(15) "ASCIINETWORK"
          ["GPSDateStamp"]=> string(10) "2011:07:19"
          */
    
    public function toPrettyCoordinates() {
        //  N51°17'1.64" W1°4'59.82"
        return "{$this->latRef}{$this->latDeg}°{$this->latMin}'{$this->latSec}\" {$this->lonRef}{$this->lonDeg}°{$this->lonMin}'{$this->lonSec}\"";
    }
    
    public function __toString() {
        return "{$this->queryLocationName()} ({$this->toPrettyCoordinates()} / {$this->toDecimalString()})";
    }
    
    private function calculateDecimalPair() {
        $lat = $lon = 0;
        
        $lat += $this->latDeg + ($this->latMin / 60) + ($this->latSec / 60 / 60);
        if ($this->latRef == 'S')
            $lat *= -1;
        
        $lon += $this->lonDeg + ($this->lonMin / 60) + ($this->lonSec / 60 / 60);
        if ($this->lonRef == 'W')
            $lon *= -1;
        
        $this->latDecimal = number_format(round($lat, 5), 5);
        $this->lonDecimal = number_format(round($lon, 5), 5);
    }
    
    public function toDecimalPair() {
        return array($this->latDecimal, $this->lonDecimal);
    }
    
    public function toDecimalString() {
        list($lat, $lon) = $this->toDecimalPair();
        return "$lat,$lon";
    }
    
    private $locationNameCache;
    public function queryLocationName() {
        if (!isset($this->locationNameCache)) {
            $cacheDecimalValues = round($this->latDecimal, 4) . ',' . round($this->lonDecimal, 4);
            if (GEOCODE_GZIP_CACHE) {
                $cacheFilename = sys_get_temp_dir() . "/geocode_$cacheDecimalValues.cache.gz";
                $cacheFilenameWrapper = "compress.zlib://$cacheFilename";
            } else {
                $cacheFilenameWrapper = $cacheFilename = sys_get_temp_dir() . "/geocode_$cacheDecimalValues.cache";
            }
            
            if (!file_exists($cacheFilename) || filemtime($cacheFilename) < (time() - GEOCODE_CACHE_TIMEOUT)) {
                $jsonString = file_get_contents("http://maps.googleapis.com/maps/api/geocode/json?latlng={$this->toDecimalString()}&sensor=false");
                file_put_contents($cacheFilenameWrapper, $jsonString);
            } else {
                $jsonString  = file_get_contents($cacheFilenameWrapper);
            }
            
            $json = json_decode($jsonString);
            if ($json->status != 'OK') {
                $this->locationNameCache = "[LOCATION REQUEST FAILED]";
                unlink($cacheFilename);
            } else {
                $this->locationNameCache = $json->results[0]->formatted_address;
            }
        }
        return $this->locationNameCache;
    }
}

class ExifData {
    const ORIENTATION_STANDARD = 1;
    const ORIENTATION_FLIP_HORIZONTAL = 2;
    const ORIENTATION_ROTATE_180 = 3;
    const ORIENTATION_FLIP_VERTICAL = 4;
    const ORIENTATION_FLIP_VERTICAL_ROTATE_270 = 5;
    const ORIENTATION_ROTATE_270 = 6;
    const ORIENTATION_FLIP_HORIZONTAL_ROTATE_270 = 7;
    const ORIENTATION_ROTATE_90 = 8;
    
    private $model;
    private $make;
    private $orientation;
    private $datetime;
    private $exposure;
    private $fNumber;
    private $isoSpeed;
    private $shutterSpeed;
    private $aperture;
    private $flash;
    private $focalLength;
    private $coordinates;
    
    public function __construct($filename) {
        $data = @exif_read_data($filename, 'EXIF');
        if (!$data)
            throw new NoExifDataException("Unable to find EXIF data for '$filename'");
        
        $this->model = @$data['Model'];
        $this->make = @$data['Make'];
        $this->orientation = @$data['Orientation'];
        $this->datetime = !empty($data['DateTimeOriginal']) ? strtotime($data['DateTimeOriginal']) : strtotime($data['DateTime']);
        $this->exposure = @$data['ExposureTime'];
        $this->fNumber = @$data['FNumber'];
        $this->isoSpeed = @$data['ISOSpeedRatings'];
        $this->shutterSpeed = @$data['ShutterSpeedValue'];
        $this->aperture = @$data['ApertureValue'];
        $this->flash = @$data['Flash'];
        $this->focalLength = @$data['FocalLength'];
        
        if (isset($data['GPSLatitudeRef'], $data['GPSLatitude'], $data['GPSLongitudeRef'], $data['GPSLongitude'])) {
            $this->coordinates = Coordinates::fromExifArrays($data['GPSLatitudeRef'], $data['GPSLatitude'], $data['GPSLongitudeRef'], $data['GPSLongitude']);
        }
    }
    
    private static function _getFloat($value) {
        $pos = strpos($value, '/');
        if ($pos === false) return (float) $value;
        $a = (float) substr($value, 0, $pos);
        $b = (float) substr($value, $pos+1);
        $out = ($b == 0) ? ($a) : ($a / $b);
        return $out;
    }
    
    private static function getFloat($value) {
        return number_format(round(self::_getFloat($value), 2), 2);
    }
    
    public function getModel() {
        return $this->model;
    }
    
    public function getMake() {
        return $this->make;
    }
    
    public function getOrientation() {
        return $this->orientation;
    }
    
    public function getDateTime() {
        return $this->datetime;
    }
    
    public function getExposure() {
        return self::getFloat($this->exposure);
    }
    
    public function getFNumber() {
        $apex  = self::getFloat($this->aperture);
        $fstop = pow(2, $apex/2);
        if ($fstop == 0) return false;
        return 'f/' . round($fstop,1);
    }
    
    public function getIsoSpeed() {
        return $this->isoSpeed;
    }
    
    public function getShutterSpeed() {
        $apex    = self::getFloat($this->shutterSpeed);
        $shutter = pow(2, -$apex);
        if ($shutter == 0) return false;
        if ($shutter >= 1) return round($shutter) . ' secs';
        return '1/' . round(1 / $shutter) . ' secs';
    }
    
    public function getAperture() {
        return self::getFloat($this->aperture);
    }
    
    public function getFlash() {
        return $this->flash;
    }
    
    public function getFocalLength() {
        return self::getFloat($this->focalLength);
    }
    
    public function getCoordinates() {
        if (!$this->coordinates) {
            throw new Exception("Coordinates not set");
        }
        return $this->coordinates;
    }
    
    public function getCoordinateString() {
        if ($this->coordinates) {
            return "{$this->coordinates}";
        } else {
            return '';
        }
    }
}

interface IMediaInfo {
    public function __construct($filename);
    public function getFilename();
    public function getBasename($stripExtension=false);
    public function getFilesize();
    public function getFilesizeHumanReadable($decimalPlaces=2, $includeTrailingZeroes=true);
    public function getWidth();
    public function getHeight();
    public function getType();
    public function getMimeType();
    public function getResizedDimensions($maxWidth, $maxHeight);
    public function getResizedImgTagSizesString($maxWidth, $maxHeight);
    public function getThumbnail($maxWidth, $maxHeight, $return=false);
    public function getOrientation();
    public function getRotationDegrees($clockwise=false);
    public function getExifData();
    public function getExtraMetaData();
}

class ImageInfo implements IMediaInfo {
    private $filename;
    private $width;
    private $height;
    private $type;
    private $mimeType;
    
    public function __construct($filename, array $info=array()) {
        // 0 = width; 1 = height; 2 = IMAGETYPE_* const; 3 = <img/> tag size string; mime = mime type string; channels = channel count (3 for RGB, 4 for CMYK), bits = colour bit count
        $this->filename = $filename;
        
        if (!$info) {
            $info = getimagesize($filename);
            if (!$info)
                throw new Exception("unable to fetch image data for '$filename'");
        }
        $this->width = $info[0];
        $this->height = $info[1];
        $this->type = $info[2];
        $this->mimeType = $info['mime'];
    }
    
    public function getFilename() {
        return $this->filename;
    }
    
    public function getBasename($stripExtension=false) {
        $info = pathinfo($this->filename);
        if ($stripExtension) {
            return $info['filename'];
        } else {
            return $info['basename'];
        }
    }
    
    public function getFilesize() {
        return filesize($this->filename);
    }
    
    public function getFilesizeHumanReadable($decimalPlaces=2, $includeTrailingZeroes=true) {
        return Formatting::formatFilesize($this->getFilesize(), $decimalPlaces, $includeTrailingZeroes);
    }
    
    public function getWidth() {
        return $this->width;
    }
    
    public function getHeight() {
        return $this->height;
    }
    
    public function getImgTagSizesString() {
        return sprintf('width="%d" height="%d"', $this->width, $this->height);
    }
    
    public function getType() {
        return $this->type;
    }
    
    public function getMimeType() {
        return $this->mimeType;
    }
    
    public function getResizedDimensions($maxWidth, $maxHeight, $currentWidth=null, $currentHeight=null) {
        $width = $this->width;
        $height = $this->height;
        
        if (isset($currentWidth))
            $width = $currentWidth;
        if (isset($currentHeight))
            $height = $currentHeight;
        
        $ar = $width / $height;
        
        if ($ar == 1) {
            //square
            $width = $height = min($maxWidth, $maxHeight);
        } elseif ($ar > ($maxWidth / $maxHeight)) {
            //Resize width
            $width = $maxWidth;
            $height = $maxWidth / $ar;
        } else {
            //Resize height
            $height = $maxHeight;
            $width = $maxHeight * $ar;
        }
        
        return array($width, $height);
    }
    
    public function getResizedImgTagSizesString($maxWidth, $maxHeight) {
        $width = $this->width;
        $height = $this->height;
        
        if (PHP_ROTATION) {
            switch ($this->getOrientation()) {
                case ExifData::ORIENTATION_FLIP_VERTICAL_ROTATE_270:
                case ExifData::ORIENTATION_ROTATE_270:
                case ExifData::ORIENTATION_FLIP_HORIZONTAL_ROTATE_270:
                case ExifData::ORIENTATION_ROTATE_90:
                    $temp = $height;
                    $height = $width;
                    $width = $temp;
                    break;
            }
        }
        
        list($w, $h) = $this->getResizedDimensions($maxWidth, $maxHeight, $width, $height);
        return sprintf('width="%d" height="%d"', $w, $h);
    }
    
    public function getThumbnail($maxWidth, $maxHeight, $return=false) {
        $cacheFilename = sys_get_temp_dir() . '/thumb_' . str_replace('/', '--', dirname(realpath($this->filename))) . "_{$this->getBasename(false)}.{$maxWidth}x{$maxHeight}.thumb";
        
        if (!file_exists($cacheFilename) || filemtime($cacheFilename) < filemtime($this->filename)) {
            switch ($this->type) {
                case IMAGETYPE_JPEG:
                    $in = imagecreatefromjpeg($this->filename);
                    break;
                case IMAGETYPE_GIF:
                    $in = imagecreatefromgif($this->filename);
                    break;
                case IMAGETYPE_PNG:
                    $in = imagecreatefrompng($this->filename);
                    break;
                default:
                    throw new Exception("Unable to handle image '{$this->filename}'");
            }
            
            if (PHP_ROTATION) {
                $blk = imagecolorallocate($in, 0, 0, 0);
                switch ($this->getOrientation()) {
                    case ExifData::ORIENTATION_STANDARD:
                        break;
                    case ExifData::ORIENTATION_FLIP_HORIZONTAL:
                        // @todo
                        break;
                    case ExifData::ORIENTATION_ROTATE_180:
                        $in = imagerotate($in, 180, $blk);
                        break;
                    case ExifData::ORIENTATION_FLIP_VERTICAL:
                        // @todo
                        break;
                    case ExifData::ORIENTATION_FLIP_VERTICAL_ROTATE_270:
                        // @todo
                        $in = imagerotate($in, -90, $blk);
                        break;
                    case ExifData::ORIENTATION_ROTATE_270:
                        $in = imagerotate($in, -90, $blk);
                        break;
                    case ExifData::ORIENTATION_FLIP_HORIZONTAL_ROTATE_270:
                        // @todo
                        $in = imagerotate($in, -90, $blk);
                        break;
                    case ExifData::ORIENTATION_ROTATE_90:
                        $in = imagerotate($in, 90, $blk);
                        break;
                }
            }
            
            list($w, $h) = $this->getResizedDimensions($maxWidth, $maxHeight, imagesx($in), imagesy($in));
            
            $out = imagecreatetruecolor($w, $h);
            
            imagecopyresampled($out, $in, 0, 0, 0, 0, $w, $h, imagesx($in), imagesy($in));
            
            imagejpeg($out, $cacheFilename, 80);
            
            imagedestroy($in);
            imagedestroy($out);
        }
        
        if ($return) {
            return file_get_contents($cacheFilename);
        } else {
            header("Content-Type: image/jpeg");
            readfile($cacheFilename);
        }
    }
    
    public function getOrientation() {
        try {
            $exif = $this->getExifData();
            return $exif->getOrientation();
        } catch (NoExifDataException $ex) {}
        return 0;
    }
    
    public function getRotationDegrees($clockwise=false) {
        $deg = 0;
        switch ($this->getOrientation()) {
            case ExifData::ORIENTATION_FLIP_HORIZONTAL_ROTATE_270:
            case ExifData::ORIENTATION_FLIP_VERTICAL_ROTATE_270:
            case ExifData::ORIENTATION_ROTATE_270:
                $deg = 270;
                break;
            case ExifData::ORIENTATION_ROTATE_180:
                $deg = 180;
                break;
            case ExifData::ORIENTATION_ROTATE_90:
                $deg = 90;
                break;
        }
        if ($clockwise) {
            return 360-$deg;
        }
        return $deg;
    }
    
    public function getExifData() {
        return new ExifData($this->filename);
    }
    
    public function getExtraMetaData() {
        return array();
    }
}

class VideoInfo implements IMediaInfo {
    private $filename;
    private $width;
    private $height;
    private $type;
    private $mimeType;
    private $ffmepg;
    
    public function __construct($filename) {
        $this->filename = $filename;
        
        $this->ffmpeg = new ffmpeg_movie($filename);
        $this->width = $this->ffmpeg->getFrameWidth();
        $this->height = $this->ffmpeg->getFrameHeight();
        $this->type = 0;
        $this->mimeType = 'video/avi';
    }
    
    public function getFilename() {
        return $this->filename;
    }
    
    public function getBasename($stripExtension=false) {
        $info = pathinfo($this->filename);
        if ($stripExtension) {
            return $info['filename'];
        } else {
            return $info['basename'];
        }
    }
    
    public function getFilesize() {
        return filesize($this->filename);
    }
    
    public function getFilesizeHumanReadable($decimalPlaces=2, $includeTrailingZeroes=true) {
        return Formatting::formatFilesize($this->getFilesize(), $decimalPlaces, $includeTrailingZeroes);
    }
    
    public function getWidth() {
        return $this->width;
    }
    
    public function getHeight() {
        return $this->height;
    }
    
    public function getType() {
        return $this->type;
    }
    
    public function getMimeType() {
        return $this->mimeType;
    }
    
    public function getResizedDimensions($maxWidth, $maxHeight) {
        $ar = $this->width / $this->height;
        
        $width = $this->width;
        $height = $this->height;
        
        if ($ar == 1) {
            //square
            $width = $height = min($maxWidth, $maxHeight);
        } elseif ($ar > ($maxWidth / $maxHeight)) {
            //Resize width
            $width = $maxWidth;
            $height = $maxWidth / $ar;
        } else {
            //Resize height
            $height = $maxHeight;
            $width = $maxHeight * $ar;
        }
        
        return array($width, $height);
    }
    
    public function getResizedImgTagSizesString($maxWidth, $maxHeight) {
        list($w, $h) = $this->getResizedDimensions($maxWidth, $maxHeight);
        return sprintf('width="%d" height="%d"', $w, $h);
    }
    
    public function getThumbnail($maxWidth, $maxHeight, $return=false) {
        $cacheFilename = sys_get_temp_dir() . '/thumb_' . str_replace('/', '--', dirname(realpath($this->filename))) . "_{$this->getBasename(false)}.{$maxWidth}x{$maxHeight}.thumb";
        
        if (!file_exists($cacheFilename) || filemtime($cacheFilename) < filemtime($this->filename)) {
            list($w, $h) = $this->getResizedDimensions($maxWidth, $maxHeight);
            
            $frame = $this->ffmpeg->getFrame(1);
            $in = $frame->toGDImage();
            
            $out = imagecreatetruecolor($w, $h);
            
            imagecopyresampled($out, $in, 0, 0, 0, 0, $w, $h, $this->width, $this->height);
            
            imagejpeg($out, $cacheFilename, 80);
            
            imagedestroy($in);
            imagedestroy($out);
        }
        
        if ($return) {
            return file_get_contents($cacheFilename);
        } else {
            header("Content-Type: image/jpeg");
            readfile($cacheFilename);
        }
    }
    public function getOrientation() {
        return 0;
    }
    
    public function getRotationDegrees($clockwise=false) {
        return 0;
    }
    
    public function getExifData() {
        throw new NoExifDataException();
    }
    
    public function getExtraMetaData() {
        $out = array(
            'Frame Rate' => $this->ffmpeg->getFrameRate() . 'fps',
        );
        
        $d = round($this->ffmpeg->getDuration());
        $h = $m = 0;
        $s = $d;
        if ($d >= 60) {
            $m = floor($d / 60);
            $s = $d % 60;
            
            if ($m >= 60) {
                $h = floor($m / 60);
                $m = $m % 60;
            }
        }
        $h = str_pad($h, 2, '0', STR_PAD_LEFT);
        $m = str_pad($m, 2, '0', STR_PAD_LEFT);
        $s = str_pad($s, 2, '0', STR_PAD_LEFT);
        $out['Duration'] = ltrim("$h:$m:$s", '0:') . ($d < 60 ? ' secs' : '');
        
        if ($this->ffmpeg->hasVideo()) {
            $out['Video Codec'] = $this->ffmpeg->getVideoCodec();
        }
        if ($this->ffmpeg->hasAudio()) {
            $out['Audio Codec'] = $this->ffmpeg->getAudioCodec();
        }
        
        return $out;
    }
}

class Gallery {
    public static function createAllForDirectory($dir=null) {
        if ($dir === null)
            $dir = getcwd();
        
        $out = array();
        foreach (glob("$dir/*") as $ff) {
            if ($ff == __FILE__) continue;
            
            $fInfo = pathinfo($ff);
            if (isset($fInfo['extension']) && in_array($fInfo['extension'], array('avi','mkv'))) {
                //Video
                $out[] = new VideoInfo($ff);
            } else {
                //Image?
                if (!$img = getimagesize($ff)) continue;
                
                $out[] = new ImageInfo($ff, $img);
            }
        }
        return $out;
    }
}

function html($s) {
    return htmlspecialchars($s, ENT_NOQUOTES);
}
function htmlq($s) {
    return htmlspecialchars($s, ENT_QUOTES);
}

if (!function_exists('imagecreatefromjpeg')) {
    die("The 'gd' extension is not available.");
}

$startTime = microtime(true);
@ob_start('ob_gzhandler', 1024);

$files = glob('*');

if (isset($_GET['thumb'])) {
    if (in_array($_GET['thumb'], $files)) {
        $img = new ImageInfo($_GET['thumb']);
        if (isset($_GET['l'])) {
            $img->getThumbnail(1024, 1024);
        } else {
            $img->getThumbnail(200, 200);
        }
        die();
    } else {
        die("Invalid image");
    }
}

$images = Gallery::createAllForDirectory();

$singleFile = $prevImage = $nextImage = false;
if (isset($_GET['file'])) {
    while (list($k, /** @var IMediaInfo */ $v) = each($images)) {
        if ($v->getBasename(false) == $_GET['file']) {
            $singleFile = $v;
            
            if (array_key_exists($k-1, $images)) {
                $prevImage = $images[$k-1];
            }
            if (array_key_exists($k+1, $images)) {
                $nextImage = $images[$k+1];
            }
            break;
        }
    }
}

if (isset($_GET['archive']) && in_array(strtolower($_GET['archive']), array('zip', 'tar'))) {
    // ZIP or TAR archive
    if (!$images)
        die();
    
    $dir = dirname($images[0]->getFilename());
    
    $filenames = array();
    foreach ($images as /** @var IMediaInfo */ $i) {
        if (dirname($i->getFilename()) != $dir) {
            $filenames[] = escapeshellarg($i->getFilename());
        } else {
            $filenames[] = escapeshellarg($i->getBasename());
        }
    }
    $filenames = join(' ', $filenames);
    
    $archiveFilename = basename(getcwd()) . '.' . $_GET['archive'];
    
    header('Content-Disposition: attachment; filename="' . $archiveFilename . '"');
    
    $curDir = getcwd();
    chdir($dir);
    if (strcasecmp($_GET['archive'], 'zip') == 0) {
        header('Content-Type: application/zip');
        passthru("zip - $filenames");
    } elseif (strcasecmp($_GET['archive'], 'tar') == 0) {
        header('Content-Type: application/x-tar');
        passthru("tar -c $filenames");
    }
    chdir($curdir);
    
    die();
}

?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <meta name="viewport" content="width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=no" />
    <title>Gallery of <?=html(basename(dirname($_SERVER['PHP_SELF'])))?>!</title>
    <script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/1.5.1/jquery.min.js"></script>
    <script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.8.10/jquery-ui.min.js"></script>
    <script type="text/javascript">
    //<![CDATA[
    /**
     * Cookie plugin
     *
     * Copyright (c) 2006 Klaus Hartl (stilbuero.de)
     * Dual licensed under the MIT and GPL licenses:
     * http://www.opensource.org/licenses/mit-license.php
     * http://www.gnu.org/licenses/gpl.html
     *
     */

    /**
     * Create a cookie with the given name and value and other optional parameters.
     *
     * @example $.cookie('the_cookie', 'the_value');
     * @desc Set the value of a cookie.
     * @example $.cookie('the_cookie', 'the_value', { expires: 7, path: '/', domain: 'jquery.com', secure: true });
     * @desc Create a cookie with all available options.
     * @example $.cookie('the_cookie', 'the_value');
     * @desc Create a session cookie.
     * @example $.cookie('the_cookie', null);
     * @desc Delete a cookie by passing null as value. Keep in mind that you have to use the same path and domain
     *       used when the cookie was set.
     *
     * @param String name The name of the cookie.
     * @param String value The value of the cookie.
     * @param Object options An object literal containing key/value pairs to provide optional cookie attributes.
     * @option Number|Date expires Either an integer specifying the expiration date from now on in days or a Date object.
     *                             If a negative value is specified (e.g. a date in the past), the cookie will be deleted.
     *                             If set to null or omitted, the cookie will be a session cookie and will not be retained
     *                             when the the browser exits.
     * @option String path The value of the path atribute of the cookie (default: path of page that created the cookie).
     * @option String domain The value of the domain attribute of the cookie (default: domain of page that created the cookie).
     * @option Boolean secure If true, the secure attribute of the cookie will be set and the cookie transmission will
     *                        require a secure protocol (like HTTPS).
     * @type undefined
     *
     * @name $.cookie
     * @cat Plugins/Cookie
     * @author Klaus Hartl/klaus.hartl@stilbuero.de
     */

    /**
     * Get the value of a cookie with the given name.
     *
     * @example $.cookie('the_cookie');
     * @desc Get the value of a cookie.
     *
     * @param String name The name of the cookie.
     * @return The value of the cookie.
     * @type String
     *
     * @name $.cookie
     * @cat Plugins/Cookie
     * @author Klaus Hartl/klaus.hartl@stilbuero.de
     */
    jQuery.cookie = function(name, value, options) {
        if (typeof value != 'undefined') { // name and value given, set cookie
            options = options || {};
            if (value === null) {
                value = '';
                options.expires = -1;
            }
            var expires = '';
            if (options.expires && (typeof options.expires == 'number' || options.expires.toUTCString)) {
                var date;
                if (typeof options.expires == 'number') {
                    date = new Date();
                    date.setTime(date.getTime() + (options.expires * 24 * 60 * 60 * 1000));
                } else {
                    date = options.expires;
                }
                expires = '; expires=' + date.toUTCString(); // use expires attribute, max-age is not supported by IE
            }
            // CAUTION: Needed to parenthesize options.path and options.domain
            // in the following expressions, otherwise they evaluate to undefined
            // in the packed version for some reason...
            var path = options.path ? '; path=' + (options.path) : '';
            var domain = options.domain ? '; domain=' + (options.domain) : '';
            var secure = options.secure ? '; secure' : '';
            document.cookie = [name, '=', encodeURIComponent(value), expires, path, domain, secure].join('');
        } else { // only name given, get cookie
            var cookieValue = null;
            if (document.cookie && document.cookie != '') {
                var cookies = document.cookie.split(';');
                for (var i = 0; i < cookies.length; i++) {
                    var cookie = jQuery.trim(cookies[i]);
                    // Does this cookie string begin with the name we want?
                    if (cookie.substring(0, name.length + 1) == (name + '=')) {
                        cookieValue = decodeURIComponent(cookie.substring(name.length + 1));
                        break;
                    }
                }
            }
            return cookieValue;
        }
    };
    //]]>
    </script>
    <style type="text/css">
    /*<![CDATA[*/
    body {
        font-family: sans-serif;
        padding: 0px;
        margin: 0px;
    }
    div#gallery {
        margin: 0px auto;
        text-align: center;
        padding: 10px;
    }
    div.img {
        display: inline-block;
        vertical-align: top;
        margin: 20px 20px 0px 20px;
        text-align: center;
        cursor: pointer;
    }
    div.img img {
        padding: 3px;
        border-width: 2px;
        border-style: solid;
        border-color: #aaa;
        border-radius: 5px;
        -webkit-border-radius: 5px;
        -moz-border-radius: 5px;
        margin-top: 5px;
        -webkit-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        -moz-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
    }
    div.img:hover img {
        border-color: #000;
    }
    div.img a:link, div.img a:active, div.img a:visited {
        color: #aaa;
        text-decoration: none;
    }
    div.img:hover a {
        color: #000;
    }
    div.img ul {
        text-align: left;
        white-space: nowrap;
        background-color: #ddd;
        margin: 8px;
        border-radius: 3px;
        -webkit-border-radius: 3px;
        -moz-border-radius: 3px;
        padding: 5px 5px 5px 10px;
    }
    div.img ul li {
        list-style-type: none;
    }
    
    div.movie img {
        border-left: 8px dotted #000;
        border-right: 8px dotted #000;
        border-top: none;
        border-bottom: none;
    }
    
    div.exif-btn {
        text-align: center;
    }
    
    div.header {
        background-color: #aaa;
        background-image: -webkit-gradient(
            linear,
            left bottom,
            left top,
            color-stop(0.25, #aaa),
            color-stop(0.75, #ddd)
        );
        background-image: -moz-linear-gradient(
            center bottom,
            #aaa 25%,
            #ddd 75%
        );
        padding: 20px 10px 5px 10px;
        margin: -20px 20px 20px 20px;
        border-bottom-left-radius: 20px;
        border-bottom-right-radius: 20px;
        border-radius-bottomleft: 20px;
        border-radius-bottomright: 20px;
        -webkit-border-bottom-left-radius: 20px;
        -webkit-border-bottom-right-radius: 20px;
        -moz-border-radius-bottomleft: 20px;
        -moz-border-radius-bottomright: 20px;
        -webkit-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        -moz-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
    }
    div.header h1 {
        text-align: center;
        text-shadow: #aaa 3px 3px;
    }
    div.header div.options {
        text-align: right;
    }
    div.header a {
        color: #333;
    }
    
    div.footer {
        background-color: #aaa;
        background-image: -webkit-gradient(
            linear,
            left bottom,
            left top,
            color-stop(0.25, #aaa),
            color-stop(0.75, #ddd)
        );
        background-image: -moz-linear-gradient(
            center bottom,
            #aaa 25%,
            #ddd 75%
        );
        padding: 5px 10px;
        margin: 20px 0px 0px 0px;
        text-align: center;
        font-size: 0.8em;
    }
    
    abbr {
        border-bottom: 1px dashed;
        cursor: help;
    }
    abbr * {
        cursor: help;
    }
    
    div.singleimage {
        text-align: center;
    }
    div.singleimage img {
        padding: 3px;
        border-width: 2px;
        border-style: solid;
        border-color: #aaa;
        border-radius: 5px;
        -webkit-border-radius: 5px;
        -moz-border-radius: 5px;
        margin-top: 5px;
        -webkit-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        -moz-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
    }
    div.singleimage a:hover img {
        border-color: #000;
    }
    div.singleimage a:link, div.singleimage a:active, div.singleimage a:visited {
        color: #aaa;
        text-decoration: none;
    }
    div.singleimage:hover a {
        color: #000;
    }
    
    p.nav {
        background-color: #aaa;
        background-image: -webkit-gradient(
            linear,
            left bottom,
            left top,
            color-stop(0.25, #aaa),
            color-stop(0.75, #ddd)
        );
        background-image: -moz-linear-gradient(
            center bottom,
            #aaa 25%,
            #ddd 75%
        );
        -webkit-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        -moz-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        border-radius: 8px;
        -moz-border-radius: 8px;
        -webkit-border-radius: 8px;
        padding: 4px 10px;
        margin: 5px 20px 25px 20px;
        text-align: center;
    }
    p.nav a.prev:link, p.nav a.next:link, p.nav a.up:link,
    p.nav a.prev:visited, p.nav a.next:visited, p.nav a.up:visited,
    p.nav a.prev:active, p.nav a.next:active, p.nav a.up:active
     {
        color: black;
        font-weight: bold;
        text-decoration: none;
    }
    p.nav a.prev:hover, p.nav a.next:hover, p.nav a.up:hover {
        text-decoration: underline;
    }
    p.nav a.prev {
        float: left;
    }
    p.nav a.next {
        float: right;
    }
    p.nav a.up {
        
    }
    
    .archivelinks {
        background-color: #aaa;
        background-image: -webkit-gradient(
            linear,
            left bottom,
            left top,
            color-stop(0.25, #aaa),
            color-stop(0.75, #ddd)
        );
        background-image: -moz-linear-gradient(
            center bottom,
            #aaa 25%,
            #ddd 75%
        );
        -webkit-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        -moz-box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        box-shadow: 5px 5px 7px rgba(0, 0, 0, 0.2);
        border-radius: 8px;
        -moz-border-radius: 8px;
        -webkit-border-radius: 8px;
        padding: 10px 10px;
        margin: 5px 50px 25px 50px;
        font-size: large;
        text-align: center;
    }
    .archivelinks a:link, .archivelinks a:visited, .archivelinks a:active {
        color: #555;
    }
    .archivelinks a:hover {
        color: #000;
    }
    
    @media all and (min-width: 600px) {
        /* Desktop browsers */
        div.options .desktop-option {
            display: inherit;
        }
        div.options .mobile-option {
            display: none;
        }
        
        body.ani div.img:hover img {
            -webkit-box-shadow: 6px 6px 4px rgba(0, 0, 0, 0.3);
            -moz-box-shadow: 6px 6px 4px rgba(0, 0, 0, 0.3);
            box-shadow: 6px 6px 4px rgba(0, 0, 0, 0.3);
            position: relative;
            left: -2px;
            top: -2px;
        }
        div.img div.exif-data {
            display: none;
            position: absolute;
            z-index: 100;
            background-color: rgba(128, 128, 128, 0.8);
            border-radius: 5px;
            -webkit-border-radius: 5px;
            -moz-border-radius: 5px;
            -webkit-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
            -moz-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
            box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        }
    }
    
    @media all and (max-width: 600px) {
        /* Mobile browsers */
        div.options .desktop-option {
            display: none;
        }
        div.options .mobile-option {
            display: inherit;
        }
        
        div.img div.exif-data {
            display: block;
            background-color: rgba(128, 128, 128, 0.8);
            padding: 3px;
            border-radius: 5px;
            -webkit-border-radius: 5px;
            -moz-border-radius: 5px;
            -webkit-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
            -moz-box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
            box-shadow: 3px 3px 4px rgba(0, 0, 0, 0.4);
        }
        div.img ul {
            white-space: normal;
            padding: 5px 5px 5px 8px;
        }
        div.img ul li {
            text-indent: -30px;
            margin-left: 30px;
        }
    }
    /*]]>*/
    </style>
</head>
<body class="ani">

<?php if ($singleFile) { ?>

    <div class="header">
        <h1><?=html($singleFile->getBasename(true))?></h1>
    </div>
    
    <p class="nav">
        <? if ($prevImage) { ?>
            <a href="?file=<?=htmlq($prevImage->getBasename(false))?>" class="prev" title="<?=htmlq($prevImage->getBasename(true))?>">&lt; Previous</a>
        <? } ?>
        <a href="./" class="up" title="Return to Gallery">Return</a>
        <? if ($nextImage) { ?>
            <a href="?file=<?=htmlq($nextImage->getBasename(false))?>" class="next" title="<?=htmlq($nextImage->getBasename(true))?>">Next &gt;</a>
        <? } ?>
    </p>
    
    <div class="singleimage">
        <? $deg = (CSS_ROTATION ? $singleFile->getRotationDegrees(true) : 0); ?>
        <a href="<?=htmlq($singleFile->getBasename(false))?>"><img src="?thumb=<?=htmlq($singleFile->getBasename(false))?>&amp;l" alt="<?=htmlq($singleFile->getBasename(true))?>" style="-webkit-transform:rotate(<?=$deg?>deg); -moz-transform:rotate(<?=$deg?>deg); -o-transform:rotate(<?=$deg?>deg);" /></a>
    </div>
    
    <div class="singleexif">
        <table border="1" style="margin: 30px auto;" cellpadding="4" cellspacing="1">
            <thead>
                <tr>
                    <th colspan="2">EXIF Data</th>
                </tr>
            </thead>
            <tbody>
                <tr><th>Dimensions</th><td><?=$singleFile->getWidth()?>x<?=$singleFile->getHeight()?></td></tr>
                <tr><th>Filesize:</th><td><?=$singleFile->getFilesizeHumanReadable()?></td></tr>
                <? try { $exif = $singleFile->getExifData(); ?>
                    <tr><th>Make:</th><td><?=html($exif->getMake())?></td></tr>
                    <tr><th>Model:</th><td><?=html($exif->getModel())?></td></tr>
                    <? if ($exif->getOrientation()) { ?>
                        <tr><th>Orientation:</th><td><?=$exif->getOrientation()?></td></tr>
                    <? } ?>
                    <tr><th>Date/Time:</th><td><?=date('H:i, jS M Y', $exif->getDateTime())?></td></tr>
                    <? if (floatval($exif->getExposure()) > 0.0001) { ?>
                        <tr><th>Exposure:</th><td><?=$exif->getExposure()?> secs</td></tr>
                    <? } ?>
                    <? if ($exif->getFNumber() != "f/1") { ?>
                        <tr><th>F-Number:</th><td><?=$exif->getFNumber()?></td></tr>
                    <? } ?>
                    <tr><th>ISO Speed:</th><td><?=$exif->getIsoSpeed()?></td></tr>
                    <? if ($exif->getShutterSpeed() != "1 secs") { ?>
                        <tr><th>Shutter Speed:</th><td><?=$exif->getShutterSpeed()?></td></tr>
                    <? } ?>
                    <? if ($exif->getAperture() != "0.00") { ?>
                        <tr><th>Aperture:</th><td><?=$exif->getAperture()?></td></tr>
                    <? } ?>
                    <? if ($exif->getFlash()) { ?>
                        <tr><th>Flash:</th><td><?=$exif->getFlash()?></td></tr>
                    <? } ?>
                    <? if ($exif->getFocalLength() != '0.00') { ?>
                        <tr><th>Focal Length:</th><td><?=$exif->getFocalLength()?></td></tr>
                    <? } ?>
                    <? try { ?>
                        <? $exif->getCoordinates(); ?>
                        <tr><th>Location Name:</th><td><a href="http://maps.google.co.uk/maps?q=<?=htmlq(urlencode($exif->getCoordinates()->toPrettyCoordinates()))?>"><?=$exif->getCoordinates()->queryLocationName()?></a></td></tr>
                        <tr><th>Location Coordinates:</th><td><a href="http://maps.google.co.uk/maps?q=<?=htmlq(urlencode($exif->getCoordinates()->toPrettyCoordinates()))?>"><?=$exif->getCoordinates()->toPrettyCoordinates()?></a></td></tr>
                        <tr><th>Location Coordinates:</th><td><a href="http://maps.google.co.uk/maps?q=<?=htmlq(urlencode($exif->getCoordinates()->toPrettyCoordinates()))?>"><?=$exif->getCoordinates()->toDecimalString()?></a></td></tr>
                    <? } catch (Exception $ex) {} ?>
                <? } catch (NoExifDataException $ex) {} ?>
                <? foreach ($singleFile->getExtraMetaData() as $k => $v) { ?>
                    <tr><th><?=html($k)?>:</th><td><?=html($v)?></td></tr>
                <? } ?>
            </tbody>
        </table>
    </div>
    
    <p class="nav">
        <? if ($prevImage) { ?>
            <a href="?file=<?=htmlq($prevImage->getBasename(false))?>" class="prev" title="<?=htmlq($prevImage->getBasename(true))?>">&lt; Previous</a>
        <? } ?>
        <a href="./" class="up" title="Return to Gallery">Return</a>
        <? if ($nextImage) { ?>
            <a href="?file=<?=htmlq($nextImage->getBasename(false))?>" class="next" title="<?=htmlq($nextImage->getBasename(true))?>">Next &gt;</a>
        <? } ?>
    </p>
    
<? } else { ?>

<div class="header">
    <h1>Gallery of <?=html(basename(dirname($_SERVER['PHP_SELF'])))?>!</h1>
    <div class="options">
        <span class="desktop-option">
            <abbr title="Will be remembered across sessions">
                <input type="checkbox" id="animations" checked="checked" /><label for="animations">Enable animations</label>
                <input type="checkbox" id="exifhover" checked="checked" /><label for="exifhover">Enable EXIF pop-up</label>
            </abbr>
        </span>
        <span class="mobile-option">
            <input type="checkbox" id="showexif" checked="checked" /><label for="showexif">Show image details</label>
        </span>
    </div>
</div>

<div id="gallery">
    <? foreach ($images as /** @var IMediaInfo */ $img) { ?>
        <div class="img <? if ($img instanceof VideoInfo) { ?>movie<? } ?>">
            <div class="img-inner ani">
                <img
                <? if (USE_DATA_URIS) { ?>
                    src="data:image/jpeg;base64,<?=base64_encode($img->getThumbnail(200, 200, true))?>"
                <? } else { ?>
                    src="<?=$_SERVER['PHP_SELF']?>?thumb=<?=htmlq($img->getBasename())?>"
                <? } ?>
                <? $deg = (CSS_ROTATION ? $img->getRotationDegrees(true) : 0) + mt_rand(-5, 5); ?>
                <?=$img->getResizedImgTagSizesString(200, 200)?> alt="<?=htmlq($img->getBasename(true))?>" style="-webkit-transform:rotate(<?=$deg?>deg); -moz-transform:rotate(<?=$deg?>deg); -o-transform:rotate(<?=$deg?>deg);" />
                
                <? if ($img instanceof VideoInfo) { ?>
                    <h2><a href="./<?=htmlq($img->getBasename(false))?>"><?=html($img->getBasename(true))?></a></h2>
                <? } else { ?>
                    <h2><a href="?file=<?=htmlq($img->getBasename(false))?>"><?=html($img->getBasename(true))?></a></h2>
                <? } ?>
            </div>
            <div class="exif-data-enabled">
                <div class="exif-data">
                    <ul>
                        <li><strong>Dimensions:</strong> <?=$img->getWidth()?>x<?=$img->getHeight()?></li>
                        <li><strong>Filesize:</strong> <?=$img->getFilesizeHumanReadable()?></li>
                        <?
                        try {
                            $exif = $img->getExifData();
                            ?>
                            <li><strong>Make:</strong> <?=html($exif->getMake())?></li>
                            <li><strong>Model:</strong> <?=html($exif->getModel())?></li>
                            <? if ($exif->getOrientation()) { ?>
                                <li><strong>Orientation:</strong> <?=$exif->getOrientation()?></li>
                            <? } ?>
                            <li><strong>Date/Time:</strong> <?=date('H:i, j<\s\u\p>S</\s\u\p> M Y', $exif->getDateTime())?></li>
                            <? if (floatval($exif->getExposure()) > 0.0001) { ?>
                                <li><strong>Exposure:</strong> <?=$exif->getExposure()?> secs</li>
                            <? } ?>
                            <? if ($exif->getFNumber() != "f/1") { ?>
                                <li><strong>F-Number:</strong> <?=$exif->getFNumber()?></li>
                            <? } ?>
                            <li><strong>ISO Speed:</strong> <?=$exif->getIsoSpeed()?></li>
                            <? if ($exif->getShutterSpeed() != "1 secs") { ?>
                                <li><strong>Shutter Speed:</strong> <?=$exif->getShutterSpeed()?></li>
                            <? } ?>
                            <? if ($exif->getAperture() != "0.00") { ?>
                                <li><strong>Aperture:</strong> <?=$exif->getAperture()?></li>
                            <? } ?>
                            <? if ($exif->getFlash()) { ?>
                                <li><strong>Flash:</strong> <?=$exif->getFlash()?></li>
                            <? } ?>
                            <? if ($exif->getFocalLength() != '0.00') { ?>
                                <li><strong>Focal Length:</strong> <?=$exif->getFocalLength()?></li>
                            <? } ?>
                            <? try { ?>
                                <? $exif->getCoordinates(); ?>
                                <li><strong>Location Name:</strong> <?=$exif->getCoordinates()->queryLocationName()?></li>
                                <li><strong>Location Coordinates:</strong> <?=$exif->getCoordinates()->toPrettyCoordinates()?></li>
                                <li><strong>Location Coordinates:</strong> <?=$exif->getCoordinates()->toDecimalString()?></li>
                            <? } catch (Exception $ex) {} ?>
                            <?
                        } catch (NoExifDataException $ex) {}
                        ?>
                        <?php foreach ($img->getExtraMetaData() as $k => $v) { ?>
                            <li><strong><?=html($k)?>:</strong> <?=html($v)?></li>
                        <? } ?>
                    </ul>
                </div>
            </div>
        </div>
    <? } ?>
</div>

<h2 class="archivelinks">Download Archive of All Images as a <a href="?archive=zip">.zip</a> or a <a href="?archive=tar" title="Not a lot of point in compressing an archive of already-compressed files...">.tar</a></h2>

<? } ?>

<div class="footer">
    &copy; 2011
    <? if (date('Y') > 2011) { ?>
        - <?=date('Y')?>
    <? } ?>
    Andrew Gillard &amp; Jack Lewis
    
    <br />
    
    <span>Generated in <?=round(microtime(true)-$startTime, 4)?> seconds using <?=Formatting::formatFilesize(memory_get_peak_usage(true), 2)?> of memory</span>
</div>

<script type="text/javascript">
//<![CDATA[
//$('#info').text("clientWidth: " + document.documentElement.clientWidth + "\ninnerWidth: " + window.innerWidth + "\nScreen Width: " + screen.width + "\noffsetWidth: " + document.documentElement.offsetWidth);
if (document.documentElement.clientWidth > 600) {
    // Desktop browsers
    var exifHover = true;
    function toggleAni(ani) {
        if (ani == undefined)
            ani = $.fx.off;
        $.fx.off = !ani;
        $('body').toggleClass('ani', ani);
    }
    $('#animations').click(function(){
        toggleAni(this.checked);
        $.cookie('enableAni', (this.checked ? '1' : '0'), {expires:365, path: '/'});
    });
    function toggleExifHover(exifhover) {
        if (exifhover == undefined)
            exifhover = false;
        exifHover = exifhover;
    }
    $('#exifhover').click(function(){
        toggleExifHover(this.checked);
        $.cookie('exifHover', (this.checked ? '1' : '0'), {expires:365, path: '/'});
    });
    $(function(){
        var cv = $.cookie('enableAni');
        if (cv !== null) {
            var enabled = (cv == 1);
            toggleAni(enabled)
            $('#animations').attr('checked', enabled);
        }
        var cv = $.cookie('exifHover');
        if (cv !== null) {
            var enabled = (cv == 1);
            toggleExifHover(enabled)
            $('#exifhover').attr('checked', enabled);
        }
    });
    
    $('a[href^=http]').click(function(ev){
        window.open(this.href);
        ev.preventDefault();
        return false;
    });
    
    $('div.img-inner').click(function(ev){
        document.location.href = $(this).find('a').attr('href');
//        window.open($(this).find('a').attr('href'));
        ev.preventDefault();
    });

    $('div.img div.img-inner').mouseenter(function() {
        $('div.exif-data').hide();
        
        if (exifHover) {
            var box = $(this).closest('div.img').find('div.exif-data');
            var offset = (box.width()/2 + 20) + ' ' + (box.height()/2 + 20);
            box.fadeIn(200);
            $('body').mousemove(function(ev) {
                box.position({
                    my: 'top left',
                    at: 'center',
                    of: ev,
                    'offset': offset
                });
            });
            $(this).triggerHandler('mousemove');
        }
    }).mouseleave(function(){
        $(this).closest('div.img').find('div.exif-data').fadeOut(200);
        $('body').unbind('mousemove');
    });
} else {
    // Mobile browsers
    var showExif = true;
    function toggleExif(show) {
        if (show == undefined)
            show = !showExif;
        showExif = show;
        $('div.exif-data').toggle(showExif);
    }
    $('#showexif').click(function(){
        toggleExif(this.checked);
        $.cookie('showExif', (this.checked ? '1' : '0'), {expires:365, path: '/'});
    });
    $(function(){
        var cv = $.cookie('showExif');
        if (cv !== null) {
            var enabled = (cv == 1);
            toggleExif(enabled);
            $('#showexif').attr('checked', enabled);
        }
    });
    
    $('div.img-inner').click(function(ev){
        document.location.href = $(this).find('a').attr('href');
        ev.preventDefault();
    });
}
//]]>
</script>

</body>
</html>
