<?php
  $title = "View Comment";
  include("header.php");
  require_once("../locales_string.php");
?>
  <p class="header">
  </p>

  <div class="subBar Options">
   <a href="view.php?useSessionFilter=true"><img src="icons/gohome.png" width="32" height="32" alt=""></a> &nbsp; &nbsp;
   <strong><img src="icons/email.png" width="16" height="16" alt="" title="" /> E-Mail Options</strong> &nbsp; &nbsp; <?php echo $developer->login . "\n"; ?>
  </div>

  <div class="content">
   <form action="view.php?useSessionFilter=true" method="post">
    <p class="Options" style="padding: 5px"><label for="text"><strong>Your e-mail address: </strong></label><input type="text" name="email" id="email" value="<?php echo htmlentities($developer->email); ?>"></p>
    <div class="Options" style="padding: 5px">
    <p style="margin: 0"><strong>Receive e-mails when</strong> new comments matching those criteria are posted:</p>
    <table>
     <tr>
      <td style="vertical-align: top">
       <strong>Type:</strong><br>
       <input type="checkbox" name="MatchLike" id="MatchLike"<?php echo (matchType($developer->types, "Like") ? " checked=\"checked\"" : ""); ?>><label for="MatchLike">Like</label><br>
       <input type="checkbox" name="MatchDislike" id="MatchDislike"<?php echo (matchType($developer->types, "Dislike") ? " checked=\"checked\"" : ""); ?>><label for="MatchDislike">Do not like</label><br>
       <input type="checkbox" name="MatchBug" id="MatchBug"<?php echo (matchType($developer->types, "Bug") ? " checked=\"checked\"" : ""); ?>><label for="MatchBug">Bug</label><br>
       <input type="checkbox" name="MatchFeature" id="MatchFeature"<?php echo (matchType($developer->types, "Feature") ? " checked=\"checked\"" : ""); ?>><label for="MatchFeature">Feature</label>
      </td>
      <td style="vertical-align: top">
       <strong>Locale:</strong><br>
<?php
  $locales = db_query("SELECT locale FROM LikeBack GROUP BY locale ORDER BY locale ASC") or die(mysql_error());
  while ($line = db_fetch_object($locales)) {
    $locale = htmlentities($line->locale);
    $checked = (matchLocale($developer->locales, $locale) ? " checked=\"checked\"" : "");
    echo "       <input type=\"checkbox\" name=\"MatchLocale_$locale\" id=\"MatchLocale_$locale\"$checked><label for=\"MatchLocale_$locale\">$locale</label><br>\n";
  }
  $checked = (matchLocale($developer->locales, "*") ? " checked=\"checked\"" : "");
?>
       <input type="checkbox" name="MatchOtherLocales" id="MatchOtherLocales"<?php echo $checked; ?>><label for="MatchOtherLocales">Others</label>
      </td>
     </tr>
    </table>
    </div>
    <p style="text-align: center"><input type="submit" name="saveOptions" value="Ok"></p>
   </form>
   <script type="text/javascript">
     document.getElementById("email").focus();
   </script>
  </div>
 </body>
</html>
