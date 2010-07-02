<?php
  require_once("db.conf.php");

  switch ($dbType) {
    default:
    case "mysql":
      mysql_connect($dbServer, $dbUser, $dbPass) or die('Database server connexion not possible.');
      mysql_select_db($dbBase)                   or die('Database connexion not possible.');
      break;
  }
  // Security if there is an hole in the remaining code:
  unset($dbServer);
  unset($dbBase);
  unset($dbUser);
  unset($dbPass);

  function db_query($query, $debug = false)
  {
    global $dbType;
    if ($debug)
      echo $query;
    switch ($dbType) {
      default:
      case "mysql":
        return mysql_query($query);
    }
  }

  function db_fetch_object($result)
  {
    global $dbType;
    switch ($dbType) {
      default:
      case "mysql":
        return mysql_fetch_object($result);
    }
  }

  function db_insert_id()
  {
    return mysql_insert_id();
  }
?>
