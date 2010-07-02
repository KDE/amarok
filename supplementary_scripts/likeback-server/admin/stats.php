<?php
  $title = "Stats";
  include("header.php");


  echo "<b>Total:</b> ";
  $data = db_query("SELECT COUNT(*) FROM LikeBack");
  $line = mysql_fetch_array($data);
  $count = $line[0];
  echo "$count<br>\n";

  echo "<b>Like:</b> ";
  $data = db_query("SELECT COUNT(*) FROM LikeBack WHERE type='Like'");
  $line = mysql_fetch_array($data);
  $count = $line[0];
  echo "$count<br>\n";

  echo "<b>Do not like:</b> ";
  $data = db_query("SELECT COUNT(*) FROM LikeBack WHERE type='Dislike'");
  $line = mysql_fetch_array($data);
  $count = $line[0];
  echo "$count<br>\n";

  echo "<b>Bug:</b> ";
  $data = db_query("SELECT COUNT(*) FROM LikeBack WHERE type='Bug'");
  $line = mysql_fetch_array($data);
  $count = $line[0];
  echo "$count<br>\n";

  echo "<b>Feature:</b> ";
  $data = db_query("SELECT COUNT(*) FROM LikeBack WHERE type='Feature'");
  $line = mysql_fetch_array($data);
  $count = $line[0];
  echo "$count<br>\n";
?>
