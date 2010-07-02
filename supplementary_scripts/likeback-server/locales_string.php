<?php
  function matchLocale($localeList, $localeToTest)
  {
    if (empty($localeList))
      $localeList = "+*";
    $localeList = ";$localeList;";

    // Test if the developer explicitely checked the locale:
    $matchLocale = !( strstr($localeList, ";+$localeToTest;") === false );
    if ($matchLocale)
      return true;

    // Test if the developer explicitely discarded a locale:
    $matchLocale = !( strstr($localeList, ";-$localeToTest;") === false );
    if ($matchLocale)
      return false;

    // Test if the developer implicitely discard other locales:
    $matchLocale = !( strstr($localeList, ";-*;") === false );
    if ($matchLocale)
      return false;

    // The developer implicitely accept other locales:
    return true;
  }

  function matchType($typeList, $typeToTest)
  {
    $typeList = ";$typeList;";
    return !( strstr($typeList, ";$typeToTest;") === false );
  }
?>