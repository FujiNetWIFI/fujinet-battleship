  CONST PLAYERS = 4
  CONST SHIPS = 4
  
  DIM shipx(PLAYERS * SHIPS)
  DIM shipy(PLAYERS * SHIPS)
  DIM shipdir(PLAYERS * SHIPS)

  REM Color stack where we use white and orange twice each
  MODE 0,7,10,7,10:WAIT

  FOR i = 0 TO PLAYERS - 1
    FOR j = 0 TO SHIPS - 1
	  DO
        sd = RANDOM(2)
		slx = abs((sd=0) * (j+1))
		sly = abs((sd=1) * (j+1))
        sx = RANDOM(10 + slx)
        sy = RANDOM(10 + sly)
      LOOP WHILE (sx+slx > 9 OR sy+sly > 9)

      idx = i * PLAYERS + j
      shipx(idx) = sx
	  shipy(idx) = sy
	  shipdir(idx) = sd
	NEXT
  NEXT

textscreen:
  REM For debugging purposes
  IF 1 THEN
  CLS
  FOR i = 0 TO PLAYERS - 1
    FOR j = 0 TO SHIPS - 1
      idx = i * PLAYERS + j
	  PRINT AT i*60 + j*10 COLOR 1,<>shipx(idx)," ",<>shipy(idx)," ",<>shipdir(idx)
	NEXT
  NEXT	

  WHILE CONT.BUTTON:WEND
  WHILE CONT.BUTTON = 0:WEND
  END IF

  CLS

  REM Paint the screen area with blue colored squares
  FOR i = 0 TO 20
    FOR j = 0 TO 21
	  rx = 1 + j/2
	  ry = i/2
	  #BACKTAB(ry*20 + rx) = $1249
	NEXT
  NEXT

  REM Insert color stack advance and "draw" crossing lines
  FOR i = 0 TO 11
      IF i>0 THEN
	    #BACKTAB(i*20) = $2000
		#BACKTAB(100 + i) = 0
	  END IF
	  #BACKTAB(i*20 + 6) = 0
	  #BACKTAB(i*20 + 13) = $2000
  NEXT	  

  PRINT AT 14,"*****"
  PRINT AT 54,"ABCD"
  PRINT AT 74,"ghij"

  WAIT

  FOR pl = 0 TO PLAYERS - 1
    FOR j = 0 TO SHIPS - 1
      sx = shipx(pl * PLAYERS + j)
  	  sy = shipy(pl * PLAYERS + j)
	  sd = shipdir(pl * PLAYERS + j)

	  FOR k = 0 TO j+1
        rx = 1 + ((pl%2)*6) + sx/2
		ry = 0 + ((pl/2)*6) + sy/2

		REM Mask the block we want to change
        IF (sx%2 = 1) THEN
		  IF (sy%2 = 0) THEN
		    #mask = $F7C7
		  ELSE
			#mask = $D1FF
		  END IF
		ELSE
		  IF (sy%2 = 0) THEN
  		    #mask = $F7F8
		  ELSE
		    #mask = $F63F
		  END IF
		END IF

		#bt = (#BACKTAB(rx + ry*20) AND #mask)

		REM Here each ship has its color, but the colors could be used
		REM to indicate which is a ship, hit or miss. We can use primary
		REM colors 0-6 plus color 7 means current color from the stack.

		#bp = (2 + j) * (1 + (sx%2)*7) * (1 + (sy%2)*63)

        REM Move bit #11 to bit #13
		IF (#bp AND $0800) THEN #bp = (#bp AND $07FF) OR $2000

		#BACKTAB(rx + ry*20) = #bt + #bp : WAIT

	    IF sd = 0 THEN sx = sx + 1
	    IF sd = 1 THEN sy = sy + 1
	  NEXT
    NEXT
  NEXT

  WHILE CONT.BUTTON:WEND
  WHILE CONT.BUTTON = 0:WEND
  GOTO textscreen

