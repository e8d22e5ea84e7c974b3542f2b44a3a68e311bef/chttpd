<?php
    ini_set('display_errors', '1');

    $x = rand(1,16);
    $y = rand(1,16);

    exec("convert c.png -background Blue  -wave $x" . "x$y /tmp/c.jpg");
    header("Content-Type: image/jpeg");

    $lol = writeToImage("/tmp/c.jpg", (isset($_GET["text"])) ? $_GET["text"] : "THIS IS AN IMAGE");
    // Send Image to Browser
    imagejpeg($lol);

    // Clear Memory
    //imagedestroy($jpg_image);


    function writeToImage($imagefile, $text){
        /*** make sure the file exists ***/
        if(file_exists($imagefile))
            {    
            /*** create image ***/
            $im = imagecreatefromjpeg($imagefile);
            $width  = imagesx($im);
            $height = imagesy($im);

            $mid_w = round($width/4);
            $mid_y = round($height/2);

        
            /*** create the text color ***/
            $text_color = imagecolorallocate($im, 233, 14, 91);
        
            /*** splatter the image with text ***/
            imagestring($im, 5, $mid_w, $mid_y,  "$text", $text_color);
            }
        else
            {
            /*** if the file does not exist we will create our own image ***/
            /*** Create a black image ***/
            $im  = imagecreatetruecolor(150, 30); /* Create a black image */
        
            /*** the background color ***/
            $bgc = imagecolorallocate($im, 255, 255, 255);
        
            /*** the text color ***/
            $tc  = imagecolorallocate($im, 0, 0, 0);
        
            /*** a little rectangle ***/
            imagefilledrectangle($im, 0, 0, 150, 30, $bgc);
        
            /*** output and error message ***/
            imagestring($im, 1, 5, 5, "Error loading $imagefile", $tc);
            }
        return $im;
        }

?>