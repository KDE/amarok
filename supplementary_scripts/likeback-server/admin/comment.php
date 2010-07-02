<?php
  $title = "View Comment";
  include("header.php");
?>
  <p class="header">
  </p>

<?php
  if (!isset($_GET['id']) || !isInteger($_GET['id'])) {
    //header("Location: view.php?useSessionFilter=true");
    echo "<script>window.location = 'view.php?useSessionFilter=true'</script>";
    exit();
  }

  $id = $_GET['id'];
  $data = db_query("SELECT * FROM LikeBack WHERE id=$id LIMIT 1");
  $comment = db_fetch_object($data);

  if (!$comment) {
    //header("Location: view.php?useSessionFilter=true");
    echo "<script>window.location = 'view.php?useSessionFilter=true'</script>";
    exit();
  }
?>
  <div class="subBar <?php echo $comment->type; ?>">
   <a href="view.php?useSessionFilter=true"><img src="icons/gohome.png" width="32" height="32" alt=""></a> &nbsp; &nbsp;
   <strong><?php echo iconForType($comment->type) . " #$comment->id"; ?></strong> &nbsp; &nbsp; <?php echo $comment->date . "\n"; ?>
  </div>
<?php
  if (isset($_POST['newRemark'])) {
    db_query("INSERT INTO LikeBackRemarks(dateTime, developer, commentId, remark) " .
             "VALUES('" . get_iso_8601_date(time()) . "', " .
                    "'$developer->id', " .
                    "'$id', " .
                    "'" . addslashes($_POST['newRemark']) . "')");
  }
?>

<?php
  $email = htmlentities($comment->email, ENT_QUOTES, "UTF-8");
  if (!empty($email))
    $email = "<a href=\"mailto:$email?subject=Your%20$comment->type%20Comment\">$email</a>";
?>
  <div class="content">
   <table class="summary">
    <tr><th>Version:</th> <td><?php echo htmlentities($comment->version, ENT_QUOTES, "UTF-8"); ?></td></tr>
    <tr><th>Locale:</th>  <td><?php echo htmlentities($comment->locale,  ENT_QUOTES, "UTF-8"); ?></td></tr>
    <tr><th>Window:</th>  <td><?php echo htmlentities($comment->window,  ENT_QUOTES, "UTF-8"); ?></td></tr>
    <tr><th>Context:</th> <td><?php echo htmlentities($comment->context, ENT_QUOTES, "UTF-8"); ?></td></tr>
    <tr><th>Status:</th>  <td><?php echo messageForStatus(htmlentities($comment->status,  ENT_QUOTES, "UTF-8")); ?></td></tr>
    <tr><th>E-Mail:</th>  <td><?php echo $email; ?></td></tr>
   </table>
   <?php echo htmlentities($comment->comment, ENT_QUOTES, "UTF-8"); ?>

   <h2><img src="icons/remarks.png" width="16" height="16" alt=""> Remarks:</h2>
   <div class="remark <?php echo $comment->type; ?>">
    <form action="comment.php?id=<?php echo $id; ?>" method="post">
     <textarea name="newRemark" id="newRemark" style="width: 50%; height: 100px; vertical-align: middle"></textarea>
     <input type="submit" value="Add New Remark" style="vertical-align: middle">
    </form>
   </div>
   <script type="text/javascript">
     document.getElementById("newRemark").focus();
   </script>

<?php
  $data = db_query("SELECT   LikeBackRemarks.*, login " .
                   "FROM     LikeBackRemarks, LikeBackDevelopers " .
                   "WHERE    LikeBackDevelopers.id=developer AND commentId=$id " .
                   "ORDER BY dateTime DESC");
  while ($line = db_fetch_object($data)) {
    echo "   <div class=\"remark $comment->type\">\n";
    echo "    <h3>On <strong>$line->dateTime</strong>, by <strong>" . htmlentities($line->login, ENT_QUOTES, "UTF-8") . "</strong></h3>\n";
    echo "    <p>" . htmlentities($line->remark, ENT_QUOTES, "UTF-8") . "</h3>\n";
    echo "   </div>\n";
  }
?>
  </div>
 </body>
</html>
