10	!
	! PRINTER OTHELO PROGRAM -- KEVIN HANDY
	!
100	! INITIALIZE
105 PRINT CHR$(155%)+"E"
110 RANDOM \ READ V%(X%,Y%) FOR X%=1% TO 4% FOR Y%=1% TO 4% &
	\ V%(X%,Y%)=V%(9%-X%,Y%) FOR X%=5% TO 8% FOR Y%=1% TO 4% &
	\ V%(X%,Y%)=V%(X%,9%-Y%) FOR X%=1% TO 8% FOR Y%=5% TO 8% &
	\ DIM V%(8%,8%)
120 READ D%(X%,1%),D%(X%,2) FOR X%=1% TO 8%
150 DATA	110,22,30,30, &
		22,-3,10,10, &
		30,10,25,20, &
		30,10,20,20
160 DATA 1,1,-1,-1,1,-1,-1,1,1,0,0,1,-1,0,0,-1
300	! INIT MATRICE
305 T5%,T6%=2%
310 B%(4%,4%),B%(5%,5%)=1% \ B%(4%,5%),B%(5%,4%)=-1 \ GOSUB 10000 &
	\ DIM B%(9%,9%)
1000	! &
	! ENTER OPPONENTS MOVE &
	!
1010 GOSUB 6000 \ PRINT FNP$(20%,10%);"YOUR MOVE      "+STRING$(6%,8%); &
	\ INPUT M% \ GOTO 1010 IF M%>88% &
	\ GOTO 2000 IF M%=0% \ X1%=M%/10% \ Y1%=M%-X1%*10%
1015 IF B%(X1%,Y1%)<>0% THEN PRINT FNP$(20%,10%);"ILLEGAL MOVE"; \ SLEEP 1% &
	\ GOTO 1000
1020 P%=-1% \ GOSUB 10100 \ IF F%=0% THEN PRINT FNP$(20%,10%);"ILLEGAL MOVE"; &
	\ SLEEP 1% \ GOTO 1000
1030 GOSUB 10200 \ T5%=T5%-F% \ T6%=T6%+F%+1%
1040 GOSUB 10000 \ GOSUB 6100 \ GOSUB 6000
2000	!
	! COMPUTERS MOVE
	!
2010 P%=1% \ Q%=0% \ FOR X1%=1% TO 8% \ FOR Y1%=1% TO 8%\ F%=0% &
	\ GOTO 2020 IF B%(X1%,Y1%)<>0% \ GOSUB 10100 &
	\ IF F% THEN Q%=Q%+1% &
	\ M1%(Q%)=X1% \ M2%(Q%)=Y1% \ M3%(Q%)=T% \ M4%(Q%)=F% &
	\ DIM M1%(20%),M2%(20%),M3%(20%),M4%(20%)
2020 NEXT Y1% \ NEXT X1% &
	\ IF Q%=0% THEN PRINT FNP$(20%,50%); &
	  "I CAN NOT MOVE!!!"; \ GOTO 1000
2040 P%=-1% \ MAT B1%=B% \ Q1%=-32767% \ FOR I1%=1% TO Q% \ Q2%=-500% &
	\ X1%=M1%(I1%) \ Y1%=M2%(I1%) \ GOSUB 10200
2050 FOR X1%=1% TO 8% \ FOR Y1%=1% TO 8% \ GOTO 2060 IF B%(X1%,Y1%)<>0% &
	\ GOSUB 10100 \ Q2%=T% IF T%>Q2%
2060 NEXT Y1% \ NEXT X1% \ M3%(I1%)=M3%(I1%)-Q2%-Q2%/4%
2070 MAT B%=B1%
2080 NEXT I1% \ Q1%=-32767% \ Q2%=0% \ FOR I1%=1% TO Q% &
	\ IF M3%(I1%)>Q1% THEN Q1%=M3%(I1%) \ Q2%=I1%
2090 NEXT I1% \ STOP IF Q2%=0%
3000	! &
	! COMPUTER HAS DECIDED ON HIS MOVE, SO NOW HE MOVES &
	!
3005 T5%=T5%+M4%(Q2%)+1% \ T6%=T6%-M4%(Q2%)
3010 P%=1% \ X1%=M1%(Q2%) \ Y1%=M2%(Q2%) &
	\ PRINT FNP$(20%,50%);"MY MOVE";X1%*10%+Y1%; &
	\ GOSUB 10200 \ GOSUB 10000 \ GOSUB 6100 \ GOTO 1000
6000	! &
	! END OF GAME &
	!
6010 IF T5%+T6%<>64% THEN RETURN
6020 IF T5%<T6% THEN PRINT "YOU WIN !!!!!!!!"
6030 IF T5%=T6% THEN PRINT "TIE GAME !!!!!!!"
6040 IF T5%>T6% THEN PRINT "I WIN !!!!!!!!!!"
6050 GOTO 32767
6100	! &
	! CORNER MOVES - CHANGE SURROUNGING SQUARES &
	!
6110 IF X1%=1% AND Y1%=1% THEN V%(1%,2%)=30% \ V%(2%,1%)=30% \ V%(2%,2%)=10% &
	\ RETURN
6120 IF X1%=1% AND Y1%=8% THEN V%(1%,7%)=30% \ V%(2%,8%)=30% \ V%(2%,7%)=10% &
	\ RETURN
6130 IF X1%=8% AND Y1%=1% THEN V%(7%,1%)=30% \ V%(8%,2%)=30% \ V%(7%,2%)=10% &
	\ RETURN
6140 IF X1%=8% AND Y1%=1% THEN V%(8%,7%)=30% \ V%(7%,8%)=30% \ V%(7%,7%)=10% &
	\ RETURN
6150 RETURN
6200	! &
	! LOWER EDJE MOVES BESIDE OPPOSITE COLOR &
	!
6210 IF (X1%=1% OR X1%=8%) AND &
	  ((B%(X1%,Y1%-1%)=-P% AND (D% AND 256%)=0%) XOR &
	   (B%(X1%,Y1%+1%)=-P% AND (D% AND 32% )=0%)) THEN T%=T%-20%
6220 IF (Y1%=1% OR Y1%=8%) AND &
	  ((B%(X1%-1%,Y1%)=-P% AND (D% AND 64%)=0%) XOR &
	   (B%(X1%+1%,Y1%)=-P% AND (D% AND 16%)=0%)) THEN T%=T%-20%
6230 IF (X1%=1% OR X1%=8%) AND &
	  ((B%(X1%,Y1%-1%)=-P% AND (D% AND 256%)=0%) AND &
	   (B%(X1%,Y1%+1%)=-P% AND (D% AND 32% )=0%)) THEN T%=T%+15%
6240 IF (Y1%=1% OR Y1%=8%) AND &
	  ((B%(X1%-1%,Y1%)=-P% AND (D% AND 64%)=0%) AND &
	   (B%(X1%+1%,Y1%)=-P% AND (D% AND 16%)=0%)) THEN T%=T%+15%
6250 IF (X1%=1% OR X1%=8%) AND (Y1%<>1% AND Y1%<>1%) THEN IF &
	((B%(X1%,Y1%-1%)=0% AND B%(X1%,Y1%-2%)=P%) OR &
	(B%(X1%,Y1%+1%)=0% AND B%(X1%,Y1%+2%)=P%)) THEN T%=T%-15%
6260 IF (Y1%=1% OR Y1%=8%) AND (X1%<>1% AND X1%<>8%) THEN IF &
	((B%(X1%-1%,Y1%)=0% AND B%(X1%-2%,Y1%)=P%) OR &
	(B%(X1%+1%,Y1%)=0% AND B%(X1%+2%,Y1%)=P%)) THEN T%=T%-15%
6290 RETURN
10000	! PRINT BOARD
10010 PRINT FNP$(0%,0%);"   1  2  3  4  5  6  7  8";
10020 FOR I1%=1% TO 8% \ PRINT \ PRINT USING"#  ",I1%; &
	\ FOR I2%=1% TO 8% \ B%=B%(I1%,I2%) &
	\ IF B%=-1% THEN PRINT"W  "; ELSE IF B%=0% THEN PRINT"-  "; &
	  ELSE IF B%=1% THEN PRINT"B  "; ELSE PRINT "*  ";
10030 NEXT I2% \ NEXT I1%
10040 PRINT FNP$(21%,13%);T6%;FNP$(21%,51%);T5%
10090 RETURN
10100	! &
	! Value moves &
	!
10110 F%=0% \ T%=0% \ F3%,F9%=0% \ D%=0%
10120 FOR J1%=1% TO 8% \ X2%=X1% \ Y2%=Y1% \ T9%=0%
10130 X2%=X2%+D%(J1%,1%) \ Y2%=Y2%+D%(J1%,2%) \ IF B%(X2%,Y2%)=0% THEN &
		GOTO 10170
10140 IF B%(X2%,Y2%)=P% THEN T%=T%+T9% \ T%=0% IF F3%=0% \ F%=F%+F3% &
	\ D%=D% OR 2%^J1% \ GOTO 10170
10160 F3%=F3%+1% \ T9%=T9%+V%(X2%,Y2%) \ GOTO 10130
10170 F3%,T9%=0% \ NEXT J1% \ T%=T%/4%+V%(X1%,Y1%)+RND*2 \ GOSUB 6200 \ RETURN
10200	! &
	! IT IS A LEGAL MOVE, SO MOVE &
	!
10210 B%(X1%,Y1%)=P% \ FOR J1%=1% TO 8% \ X2%=X1% \ Y2%=Y1%
10220 X2%=X2%+D%(J1%,1%) \ Y2%=Y2%+D%(J1%,2%) &
	\ GOTO 10280 IF B%(X2%,Y2%)=0% &
	\ GOTO 10250 IF B%(X2%,Y2%)=P% &
	\ GOTO 10220
10250 X3%=X1% \ Y3%=Y1%
10260 B%(X3%,Y3%)=P% \ X3%=X3%+D%(J1%,1%) \ Y3%=Y3%+D%(J1%,2%) &
	\ GOTO 10260 UNLESS X3%=X2% AND Y3%=Y2%
10280 NEXT J1%
10290 RETURN
20000 DEF FNP$(X%,Y%) \ PRINT \ FNEND
32767 END
