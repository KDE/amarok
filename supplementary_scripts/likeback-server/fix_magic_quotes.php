<?php
  function fix_magic_quotes ($var = NULL, $sybase = NULL)
  {
    // si $sybase n'est pas spécifié, on regarde la configuration ini
    if ( !isset ($sybase) )
    {
      $sybase = ini_get ('magic_quotes_sybase');
    }

    // si $var n'est pas spécifié, on corrige toutes les variables superglobales
    if ( !isset ($var) )
    {
      // si les magic_quotes sont activées
      if ( get_magic_quotes_gpc () )
      {
        // tableaux superglobaux a corriger
        $array = array ('_REQUEST', '_GET', '_POST', '_COOKIE');
        if ( substr (PHP_VERSION, 0, 1) <= 4 )
        {
          // PHP5 semble ne pas changer _ENV et _SERVER
          array_push ($array, '_ENV', '_SERVER');
          // les magic_quotes ne changent pas $_SERVER['argv']
          $argv = isset($_SERVER['argv']) ? $_SERVER['argv'] : NULL;
        }
        foreach ( $array as $var )
        {
          $GLOBALS[$var] = fix_magic_quotes ($GLOBALS[$var], $sybase);
        }
        if ( isset ($argv) )
        {
          $_SERVER['argv'] = $argv;
        }
        // désactive les magic quotes dans ini_set pour que les
        // scripts qui y sont sensibles fonctionnent
        ini_set ('magic_quotes_gpc', 0);
      }

      // idem, pour magic_quotes_sybase
      if ( $sybase )
      {
        ini_set ('magic_quotes_sybase', 0);
      }

      // désactive magic_quotes_runtime
      set_magic_quotes_runtime (0);
      return TRUE;
    }

    // si $var est un tableau, appel récursif pour corriger chaque élément
    if ( is_array ($var) )
    {
      foreach ( $var as $key => $val )
      {
        $var[$key] = fix_magic_quotes ($val, $sybase);
      }

      return $var;
    }

    // si $var est une chaine on utilise la fonction stripslashes,
    // sauf si les magic_quotes_sybase sont activées, dans ce cas on
    // remplace les doubles apostrophes par des simples apostrophes
    if ( is_string ($var) )
    {
      return $sybase ? str_replace ('\'\'', '\'', $var) : stripslashes ($var);
    }

    // sinon rien
    return $var;
  }

  fix_magic_quotes();
?>
