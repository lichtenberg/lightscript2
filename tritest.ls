idle IDLEWHITE on TRI1;

defmacro MAC1 {
   at -0.5 on TRI1 do BLINK;
   };

at 0.0 do OFF on NEON2;
//at 0.1 on TRI1 do BLINK color GREEN speed 500;
at 0.1 do MOVINGGRADIENT on [NEON1, NEON2] speed 750;
//at 0.1 do MOVINGGRADIENT on [TRI1, SMALLTRI] color PRGB speed 750;
//at 1.0 macro MAC1;
at 20.0 do OFF on TRI1;








