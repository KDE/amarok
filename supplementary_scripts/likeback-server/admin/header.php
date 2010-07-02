<?php
  session_start();
  require_once("../db.conf.php");
  require_once("../db.php");
  require_once("../fix_magic_quotes.php");

  function isInteger($string)
  {
    for ($i = 0; $i < strlen($string); $i++)
      if (!ctype_digit($string[$i]))
        return false;
    return true;
  }

  /// From http://fr.php.net/manual/fr/function.date.php
  /// @Param $int_date Current date in UNIX timestamp
  function get_iso_8601_date($int_date) {
    $date_mod      = date('Y-m-d\TH:i:s', $int_date);
    $pre_timezone  = date('O', $int_date);
    $time_zone     = substr($pre_timezone, 0, 3).":".substr($pre_timezone, 3, 2);
    $date_mod     .= $time_zone;
    return $date_mod;
  }

  // TODO: Store in session
  $userName = addslashes($_SERVER['PHP_AUTH_USER']);
  $data = db_query("SELECT * FROM LikeBackDevelopers WHERE login='$userName' LIMIT 1");
  $developer = db_fetch_object($data);
  if (!$developer) {
    db_query("INSERT INTO LikeBackDevelopers(login, types, locales) VALUES('$userName', 'Like;Dislike;Bug;Feature', '+*')");
    $data = db_query("SELECT * FROM LikeBackDevelopers WHERE login='$userName' LIMIT 1");
    $developer = db_fetch_object($data);
  }

  function iconForType($type)
  {
    if ($type == "Like")
      return "<img src=\"icons/like.png\" width=\"16\" height=\"16\" alt=\"[Like]\" title=\"Like\" />";
    else if ($type == "Dislike")
      return "<img src=\"icons/dislike.png\" width=\"16\" height=\"16\" alt=\"[Do not like]\" title=\"Do not like\" />";
    else if ($type == "Bug")
      return "<img src=\"icons/bug.png\" width=\"16\" height=\"16\" alt=\"[Bug]\" title=\"Bug\" />";
    else
      return "<img src=\"icons/feature.png\" width=\"16\" height=\"16\" alt=\"[Feature]\" title=\"Feature\" />";
  }

  function messageForStatus($status)
  {
    if ($status == "New")
      return "New";
    else if ($status == "Confirmed")
      return "Confirmed";
    else if ($status == "Progress")
      return "In progress";
    else if ($status == "Solved")
      return "Solved";
    else if ($status == "Invalid")
      return "Invalid";
  }

?>
<html>
 <head>
  <title><?php echo $title; ?> - LikeBack</title>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <link rel="stylesheet" type="text/css" href="style.css">
  <script type="text/javascript" src="xmlhttprequest.js"></script>
  <script type="text/javascript" src="jsUtilities.js"></script>
  <script type="text/javascript" src="scripts.js"></script>
 </head>
 <body onmousedown="hideStatusMenu(event)">
