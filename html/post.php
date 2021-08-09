<?php
ini_set('display_errors', '1');
$whatwasit = "You said nothing before.";
if (isset($_COOKIE["WhatYouSaid"]))
{
    $whatwasit = $_COOKIE["WhatYouSaid"];
}

setcookie("WhatYouSaid", $_POST["data"], time()+3600);

echo "<h1>You said before this: $whatwasit</h1><br>";
echo "<h1>This time you said: " . $_POST["data"] . "</h1>";
?>