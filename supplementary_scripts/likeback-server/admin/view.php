<?php
  $title = "Comment List";
  include("header.php");


  require_once("../locales_string.php");
$type="Like";
$locale = "fr";
  $sendMailTo = "";
  $data = db_query("SELECT * FROM LikeBackDevelopers WHERE email!=''");
  while ($line = db_fetch_object($data)) {
    if (matchType($line->types, $type) && matchLocale($line->locales, $locale))
      $sendMailTo .= (empty($sendMailTo) ? "" : "; ") . $line->email;
  }



  if (isset($_POST['saveOptions'])) {
    $email = addslashes($_POST['email']);

    $types = "";
    if (isset($_POST['MatchLike']))
      $types .= (empty($types) ? "" : ";") . "Like";
    if (isset($_POST['MatchDislike']))
      $types .= (empty($types) ? "" : ";") . "Dislike";
    if (isset($_POST['MatchBug']))
      $types .= (empty($types) ? "" : ";") . "Bug";
    if (isset($_POST['MatchFeature']))
      $types .= (empty($types) ? "" : ";") . "Feature";

    $locales = "";
    $localesData = db_query("SELECT locale FROM LikeBack GROUP BY locale ORDER BY locale ASC") or die(mysql_error());
    while ($line = db_fetch_object($localesData)) {
      $locale = htmlentities($line->locale);
      if (isset($_POST["MatchLocale_$locale"]))
        $locales .= (empty($locales) ? "" : ";") . "+$locale";
      else
        $locales .= (empty($locales) ? "" : ";") . "-$locale";
    }
    if (isset($_POST['MatchOtherLocales']))
      $locales .= (empty($locales) ? "" : ";") . "+*";
    else
      $locales .= (empty($locales) ? "" : ";") . "-*";
    $locales = addslashes($locales);

    $login = htmlentities($developer->login, ENT_QUOTES, "UTF-8");

    db_query("UPDATE LikeBackDevelopers SET email='$email', types='$types', locales='$locales' WHERE login='$login'");
  }
?>
  <div id="statusMenu">
   <strong>Mark As:</strong>
   <a id="markAsNew"       href="#"><img src="icons/new.png"       width="16" height="16" alt="" />New</a>
   <a id="markAsConfirmed" href="#"><img src="icons/confirmed.png" width="16" height="16" alt="" />Confirmed</a>
   <a id="markAsProgress"  href="#"><img src="icons/progress.png"  width="16" height="16" alt="" />In progress</a>
   <a id="markAsSolved"    href="#"><img src="icons/solved.png"    width="16" height="16" alt="" />Solved</a>
   <a id="markAsInvalid"   href="#"><img src="icons/invalid.png"   width="16" height="16" alt="" />Invalid</a>
<!--
   <strong>Duplicate Of:</strong>
   <a id="markAsDuplicate" href="#"><img src="icons/duplicate.png" width="16" height="16" alt="" />Choose...</a>
   <div style="vertical-align: middle; padding: 2px"><img src="icons/id.png"     width="16" height="16" alt="" style="vertical-align: middle; padding: 1px 1px 3px 3px"/><input type="text" size="3"> <input type="submit" value="Ok"></div>
-->
  </div>

  <form action="view.php" method="post">
   <p class="header">
    <a href="options.php" class="link">E-Mail Options...</a>

    <strong>Version:</strong>
    <select name="version">
     <option>(All)</option>
<?php
  if (isset($_GET['useSessionFilter']) && $_GET['useSessionFilter'] == "true")
    $_POST = $_SESSION['postedFilter'];
  $_SESSION['postedFilter'] = $_POST;

  // Change the status of a comment:
  $existingStatus = array("New" => true, "Confirmed" => true, "Progress" => true, "Solved" => true, "Invalid" => true);
  if (isset($_GET['markAs']) && isset($existingStatus[$_GET['markAs']]) && isset($_GET['id'])) {
    $id = $_GET['id'];
    // Before, id was "comment_###":
    //$id = split("_", $_GET['id']);
    //$id = (isset($id[1]) ? $id[1] : "ERROR");
    if (isInteger($id)) {
      db_query("UPDATE LikeBack SET status='" . $_GET['markAs'] . "' WHERE id='$id'") or die(mysql_error());
      if (isset($_GET['isAJAX']))
        exit(); // The JavaScript will do the update. No reload wanted.
    }
  }

  // Figure out if we are filtering or if it is the first time:
  $filtering = isset($_POST['filtering']);

  $versionFilter = "";
  $versions = db_query("SELECT version FROM LikeBack GROUP BY version ORDER BY date DESC") or die(mysql_error());
  while ($line = db_fetch_object($versions)) {
    $version = htmlentities($line->version);
    // Only if the posted version is a valid version:
    $select = "";
    if (isset($_POST["version"]) && $_POST["version"] == "version_$version") {
      $versionFilter = $version;
      $select = " selected=\"selected\"";
    }
    echo "     <option value=\"version_$version\"$select>$version</option>\n";
  }
?>
    </select>
    &nbsp; &nbsp;

    <strong>Locale:</strong>
<?php
  $localesFilter = array();
  $locales = db_query("SELECT locale FROM LikeBack GROUP BY locale ORDER BY locale ASC") or die(mysql_error());
  while ($line = db_fetch_object($locales)) {
    $locale = htmlentities($line->locale);
    // Only if the posted locales are valid locales:
    $select = "";
    if (!$filtering || isset($_POST["locale_$locale"])) {
      $localesFilter[] = $locale;
      $select = " checked=\"checked\"";
    }
    echo "    <label for=\"locale_$locale\"><input type=\"checkbox\" id=\"locale_$locale\" name=\"locale_$locale\"$select>$locale</label>\n";
  }
?>
    <br>

    <strong>Status:</strong>
<?php
  $statusFilter = array();
  $newSelect = "";
  if (!$filtering || isset($_POST['New'])) {
    $statusFilter[] = "New";
    $newSelect = " checked=\"checked\"";
  }
  $confirmedSelect = "";
  if (!$filtering || isset($_POST['Confirmed'])) {
    $statusFilter[] = "Confirmed";
    $confirmedSelect = " checked=\"checked\"";
  }
  $progressSelect = "";
  if (!$filtering || isset($_POST['Progress'])) {
    $statusFilter[] = "Progress";
    $progressSelect = " checked=\"checked\"";
  }
  $solvedSelect = "";
  if (isset($_POST['Solved'])) { // Not shown by default, because it is of no importance anymore
    $statusFilter[] = "Solved";
    $solvedSelect = " checked=\"checked\"";
  }
  $invalidSelect = "";
  if (isset($_POST['Invalid'])) { // Not shown by default, because it is of no importance anymore
    $statusFilter[] = "Invalid";
    $invalidSelect = " checked=\"checked\"";
  }
?>
    <label for="New"><input type="checkbox" id="New" name="New"<?php echo $newSelect; ?>>New</label>
    <label for="Confirmed"><input type="checkbox" id="Confirmed" name="Confirmed"<?php echo $confirmedSelect; ?>>Confirmed</label>
    <label for="Progress"><input type="checkbox" id="Progress" name="Progress"<?php echo $progressSelect; ?>>In progress</label>
    <label for="Solved"><input type="checkbox" id="Solved" name="Solved"<?php echo $solvedSelect; ?>>Solved</label>
    <label for="Invalid"><input type="checkbox" id="Invalid" name="Invalid"<?php echo $invalidSelect; ?>>Invalid</label>
    <br>

    <strong>Type:</strong>
<?php
  $typesFilter = array();
  $likeSelect = "";
  if (!$filtering || isset($_POST['Like'])) {
    $typesFilter[] = "Like";
    $likeSelect = " checked=\"checked\"";
  }
  $dislikeSelect = "";
  if (!$filtering || isset($_POST['Dislike'])) {
    $typesFilter[] = "Dislike";
    $dislikeSelect = " checked=\"checked\"";
  }
  $bugSelect = "";
  if (!$filtering || isset($_POST['Bug'])) {
    $typesFilter[] = "Bug";
    $bugSelect = " checked=\"checked\"";
  }
  $featureSelect = "";
  if (!$filtering || isset($_POST['Feature'])) {
    $typesFilter[] = "Feature";
    $featureSelect = " checked=\"checked\"";
  }
?>
    <label for="Like"><input type="checkbox" id="Like" name="Like"<?php echo $likeSelect; ?>>Like</label>
    <label for="Dislike"><input type="checkbox" id="Dislike" name="Dislike"<?php echo $dislikeSelect; ?>>Do not like</label>
    <label for="Bug"><input type="checkbox" id="Bug" name="Bug"<?php echo $bugSelect; ?>>Bug</label>
    <label for="Feature"><input type="checkbox" id="Feature" name="Feature"<?php echo $featureSelect; ?>>Feature</label>
    &nbsp; &nbsp;

<?php
  $textFilter = "";
  $textValue  = "";
  if (isset($_POST['text'])) {
    $textFilter = htmlentities($_POST['text']);
    $textValue  = " value=\"$textFilter\"";
  }
?>
    <strong>Text:</strong>
    <input type="text" name="text" id="text" size="10"<?php echo $textValue; ?>>
    &nbsp; &nbsp;

    <input type="submit" name="filtering" value="Filter">
    &nbsp; &nbsp;
    <a href="view.php">Reset</a>
   </p>
  </form>

  <div class="subBar Options">
   <span id="loadingMessage">Loading...</span>
   <span id="countMessage">Number of displayed comments: <strong id="commentCount">NaN</strong></span>
  </div>

  <div class="content">
<?php
  $request = "";

  // Filter version:
  if (!empty($versionFilter))
    $request .= " AND version='$versionFilter'";

  // Filter locales:
  $localesRequest = "";
  foreach ($localesFilter as $locale) {
    if (empty($localesRequest))
      $localesRequest = "locale='$locale'";
    else
      $localesRequest .= " OR locale='$locale'";
  }
  if (!empty($localesRequest))
    $request .= " AND ($localesRequest)";

  // Filter types:
  $typesRequest = "";
  foreach ($typesFilter as $type) {
    if (empty($typesRequest))
      $typesRequest = "type='$type'";
    else
      $typesRequest .= " OR type='$type'";
  }
  if (!empty($typesRequest))
    $request .= " AND ($typesRequest)";

  // Filter status:
  $statusRequest = "";
  $statusJS = "";
  foreach ($statusFilter as $status) {
    if (empty($statusRequest)) {
      $statusRequest = "status='$status'";
      $statusJS      = "$status: true";
    } else {
      $statusRequest .= " OR status='$status'";
      $statusJS      .= ", $status: true";
    }
  }
  if (!empty($statusRequest))
    $request .= " AND ($statusRequest)";

  // Filter text:
  if (!empty($textFilter))
    $request .= " AND comment LIKE '%$textFilter%'";

  $data = db_query("SELECT   LikeBack.*, COUNT(LikeBackRemarks.id) AS remarkCount " .
                   "FROM     LikeBack LEFT JOIN LikeBackRemarks ON LikeBack.id=commentId " .
                   "WHERE    1=1$request ".
                   "GROUP BY LikeBack.id " .
                   "ORDER BY date DESC");

  echo "   <table id=\"data\">\n";
  echo "    <thead>\n";
  echo "     <tr>\n";
  echo "      <th></th>\n";
  echo "      <th>Id</th>\n";
  echo "      <th>Date</th>\n";
  echo "      <th>Version</th>\n";
  echo "      <th>Locale</th>\n";
  echo "      <th>Window</th>\n";
  echo "      <th>Context</th>\n";
  echo "      <th>Status</th>\n";
  echo "      <th>Comment</th>\n";
  echo "      <th>&nbsp;</th>\n";
  echo "     </tr>\n";
  echo "    </thead>\n";
  echo "    <tbody>\n";
  $commentCount = 0;
  while ($line = db_fetch_object($data)) {
    $commentCount++;
    if (empty($line->email))
      $emailCell = "";
    else {
      $email     = htmlentities($line->email,   ENT_QUOTES, "UTF-8");
      $emailCell = "<img src=\"icons/email.png\" width=\"16\" height=\"16\" title=\"$email\" />";
      $emailCell = "<a href=\"mailto:$email?subject=Your%20$line->type%20Comment\">$emailCell</a>";
    }

    $typeCell = iconForType($line->type);

    $date = split(" ", $line->date);
    $dateCell = "<div title=\"$date[0], at $date[1]\"><nobr>$date[0]</nobr></div>";

    $window = htmlentities($line->window,  ENT_QUOTES, "UTF-8");
    $lastSeparation = strrpos($window, "~~");
    if ($lastSeparation === false)
      $windowCell = "<div title=\"$window\"><nobr>$window</nobr></div>";
    else {
      $lastWindow = substr($window, $lastSeparation + 1);
      $windowCell = "<div title=\"$window\"><nobr>...$lastWindow</nobr></div>";
    }

    $id = $line->id;

    if ($line->status == "New")
      $statusCell = "<img src=\"icons/new.png\"       id=\"status_comment_$id\" width=\"16\" height=\"16\" title=\"New\" />";
    else if ($line->status == "Confirmed")
      $statusCell = "<img src=\"icons/confirmed.png\" id=\"status_comment_$id\" width=\"16\" height=\"16\" title=\"Confirmed\" />";
    else if ($line->status == "Progress")
      $statusCell = "<img src=\"icons/progress.png\"  id=\"status_comment_$id\" width=\"16\" height=\"16\" title=\"In progress\" />";
    else if ($line->status == "Solved")
      $statusCell = "<img src=\"icons/solved.png\"    id=\"status_comment_$id\" width=\"16\" height=\"16\" title=\"Solved\" />";
    else
      $statusCell = "<img src=\"icons/invalid.png\"   id=\"status_comment_$id\" width=\"16\" height=\"16\" title=\"Invalid\" />";

    $statusCell = "<a href=\"#\" onclick=\"return showStatusMenu(event)\">$statusCell</a>";
    $remarkCount = " <a title=\"Remark count\" href=\"comment.php?id=$id\"><img src=\"icons/remarks.png\" width=\"16\" height=\"16\" />$line->remarkCount</span>";
    if ($line->remarkCount == 0)
      $statusCell .= "<span class=\"noRemark\">$remarkCount</span>";
    else
      $statusCell .= $remarkCount;

    $commentCell = str_replace("\n", "<br>", htmlentities($line->comment, ENT_QUOTES, "UTF-8"));
    $commentCell = str_replace($textFilter, "<span class=\"found\">$textFilter</span>", $commentCell);

    echo "     <tr class=\"$line->type $line->status\" id=\"comment_$line->id\">\n";
    echo "      <td>$typeCell</td>\n";
    echo "      <td><a href=\"comment.php?id=$id\">#$id</a></td>\n";
    echo "      <td>$dateCell</td>\n";
    echo "      <td>" . htmlentities($line->version, ENT_QUOTES, "UTF-8") . "</td>\n";
    echo "      <td>" . htmlentities($line->locale,  ENT_QUOTES, "UTF-8") . "</td>\n";
    echo "      <td>$windowCell</td>\n";
    echo "      <td>" . htmlentities($line->context, ENT_QUOTES, "UTF-8") . "</td>\n";
    echo "      <td><nobr>$statusCell</nobr></td>\n";
    echo "      <td>$commentCell</td>\n";
    echo "      <td style=\"text-align: center\">$emailCell</td>\n";
    echo "     </tr>\n";
  }
  echo "    </tbody>\n";
  echo "   </table>\n";
?>
  <script type="text/javascript">
    document.getElementById("commentCount").innerHTML = <?php echo $commentCount; ?>;
    document.getElementById("loadingMessage").style.display = "none"; // Hide the span "Loading..."
    document.getElementById("countMessage").style.display = "inline"; // Shown the span "Number of displayed comments: X"

    var shownStatus = { <?php echo $statusJS; ?> };
  </script>
  </div>
 </body>
</html>
