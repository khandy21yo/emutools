1	  EXTEND
10	  DIM A%(4000%) &
	\ T%=0% &
	\ INPUT "LIMIT";J% &
	\ T=TIME(0%) &
		\ FOR I%=3% TO J% STEP 2% &
			\ M%=SQR(I%) &
			\ FOR K%=1% TO T% &
				\ GOTO 50 IF A%(K%)>M% &
				\ GOTO 60 IF (I%/A%(K%))*A%(K%)=I% &
			\ NEXT K% &

50			  T%=T%+1% &
			\ A%(T%)=I% &

60		  NEXT I% &
	\ PRINT "ELAPSED TIME: "; (TIME(0%)-T); "SECONDS" &
	\ SLEEP 5 &
	\ PRINT A%(I%), FOR I%=1% TO T% &
	\ PRINT &
	\ END
