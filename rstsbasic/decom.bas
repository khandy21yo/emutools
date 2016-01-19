1	! decom.bas &
	! &
	!	RSTS/E Basic+ Decompiler &
	! &

10	DEBUG% = -1% &

910	DIM  ADDR%(420), VAR$(420), DIM.IT$(420%), &
		LIN$(100%), LIN%(100%) &

930	DIM #1%, P0%(32767)\ DIM #2%, P1%(32767) &

940	DIM CDT%(255%) &

950	DIM STACK$(64%), STACK%(64%), CLOS$(1%) &

960	DIM #6%, PPCD$(256%)=16% &

1110	ON ERROR GOTO 19000 &

1120	! &
	! Ask for file to disassemble &
	! &
	PRINT "Compiled program "; &
	\ INPUT LINE BACFILE$ &
	\ BACFILE$=CVT$$(BACFILE$,-1%) &
	\ BACFILE$=BACFILE$+".BAC" UNLESS INSTR(1%,BACFILE$,".") &

1210	! &
	! Open necessary files &
	! &
	ON ERROR GOTO 1250 &
	\ JUNK$ = BACFILE$ &
	\ OPEN BACFILE$ FOR INPUT AS FILE 1% &
	\ OPEN BACFILE$ FOR INPUT AS FILE 2% &
	\ JUNK$ = "PUSPOP.OUT" &
	\ OPEN "PUSPOP.OUT" FOR INPUT AS FILE 6% &
	\ ON ERROR GOTO 19000 &
	\ GOTO 1260 &

1250	PRINT "?Can't open "; JUNK$; " -- "; FNERR$(ERR) &
	\ RESUME 19080 &

1260	! &
	! test to make sure file is actually a compiled program &
	! &
	SP% = FNW0%(514%) &
	\ Q% = FNB0%(SP%+1%) &
 !	\ IF ((SWAP%(CVT$%(MID(SYS(CHR$(6%)+CHR$(-8%) &
 !				+CHR$(1%)),7%,2%))) + 1%) AND 3%) <> 0% &
 !			OR Q% < 1% OR Q% > 4% THEN &
 !	  PRINT "?"; BACFILE$; " is not a compiled BASIC-PLUS I program" &
 !	  \ GOTO 19080 &

1310	RESTORE &
	\ ADDR.TOT%=0% &
	\ CLOS$(0%)=")" &
	\ CLOS$(1%) = "" &
	\ WHILE 1% &
	  \ READ VAR%, VAR$ &
	  \ GOTO 1315 IF VAR% = 0% &
	  \ ADDR.TOT% = ADDR.TOT% + 1% &
	  \ VAR$(ADDR.TOT%) = VAR$ &
	  \ ADDR%(ADDR.TOT%) = VAR% &
	\ NEXT &

1315	READ CDT%(Q%) FOR Q%=0% TO 255% &

1320	SPDA% = FNW0%(SP%+28%) &
	\ SPTA%,SCTH% = FNW0%(SP%+30%) &
	\ FPULEN% = FNB0%(SP%+38%) &
	\ FPUOFF% = 4%+2%*FPULEN%-2% &
	\ VARTAB% = SPDA%+1214% &

3010	! &
	! Read in the list of variable names &
	! &
	FOR LETTER% = 65% TO 90% &
	  \ W% = (LETTER% - 65%)*2% + VARTAB% &
	  \ Q% = FNW1%(W%) &
	  \ GOTO 3300 UNLESS Q% &
	  \ W%, W1% = W% + Q% &

3020	  VARNAME$ = CHR$(LETTER%) &
	  \ WHILE 1% &
	    \ W1% = W1% - 1% &
	    \ Q% = FNB0%(W1%) &
	    \ GOTO 3030 IF (Q% AND 128%) &
	    \ VARNAME$ = VARNAME$ + CHR$(Q%) IF Q% &
	  \ NEXT &

3030	  VAR$ = VARNAME$ &
	  \ W1% = W1% AND -2% &
	  \ TYPE% = FNB0%(W1%) &
	  \ IF		TYPE% AND 8% THEN VALOFFSET% = 28% &
	    ELSE IF	TYPE% AND 4% THEN VALOFFSET% = 08% &
	    ELSE IF	TYPE% AND 2% THEN VALOFFSET% = FPUOFF% &
	    ELSE			  VALOFFSET% = 04% &

3040	  DIM.IT$="" &
	  \ VAR$ = "FN"+VAR$	IF TYPE% AND 16% &
	  \ VAR$ = VAR$+"%"	IF TYPE% AND 1% &
	  \ VAR$ = VAR$+"$"	IF TYPE% AND 4% &
	  \ IF TYPE% AND 8% THEN &
	      VAR$ = VAR$+"(" &
	      \ DIM.IT$=VAR$+NUM1$(FNW0%(W1%-6%)) &
	      \ DIM.IT$ = DIM.IT$+","+NUM1$(FNW0%(W1%-4%)) IF FNW0%(W1%-4%) &
	      \ DIM.IT$ = DIM.IT$+")"\ Q% = FNB0%(W1%-18%) &
	      \ IF Q% THEN &
		  DIM.IT$ = DIM.IT$+"="+NUM1$(2%*FNW0%(W1%-8%)) IF TYPE% AND 4% &
		  \ DIM.IT$ = " #"+NUM1$(Q%/2%)+", "+DIM.IT$ &

3110	  ADDR.TOT% = ADDR.TOT% + 1% &
	  \ VAR$(ADDR.TOT%) = VAR$ &
	  \ DIM.IT$(ADDR.TOT%) = DIM.IT$ &
	  \ ADDR%(ADDR.TOT%) = W1% - VALOFFSET% - SPDA% &

3200	  Q% = FNW0%(W1%-2%)\ IF Q% THEN W1% = Q%+W1%-1%\ GOTO 3030 &

3210	  Q% = FNW0%(W%    )\ IF Q% THEN W%,W1% = Q%+W%\  GOTO 3020 &

3300	NEXT LETTER% &

4010	! &
	! Now, we can start looking at the program code &
	! &
	CODEHEAD% = SCTH% &

4020	! &
	! Start of a line number &
	! &
	T% = FNW0%(CODEHEAD%) &
	\ GOTO 4200 IF (T% = 0%) &
	\ CODEHEAD% = CODEHEAD%+T% &
	\ T% = FNB0%(CODEHEAD%+9%) &
	\ STOP IF T% = 5% &
	\ LINENO% = FNW0%(CODEHEAD%+10%) &
	\ ENDCODE%   = FNW0%(CODEHEAD%+04%) &
	\ GOTO 4020 UNLESS ENDCODE% &
	\ PRINT "Line: "; LINENO% IF DEBUG% &
	\ CODE%,IPC% = FNW0%(CODEHEAD%+02%)+CODEHEAD% &
	\ CODEOFFSET% = 0% &
	\ GOSUB 5100 IF LINENO%<>OLDLINE% &
	\ GOTO 4100 IF T% = 4% &
	\ IF T% = 1% THEN &
	    PRIN$ = "DIM " &
	    \ FOR W%=2% TO ENDCODE%-3% STEP 2% &
	    \ PRIN$ = PRIN$ + ", " IF W%<>2% &
	    \ JUNK$ = FNVAR$(FNI%(W%)) &
	    \ PRIN$ = PRIN$ + DIM.IT$(Q1%) &
	    \ NEXT W% &
	    \ GOSUB 5000 &
	    \ GOTO 4020 &

4030	FIELD.STARTED% = 0% &

4040	! &
	! Process Byte codes for line &
	! &
	NEXTCODE% = 0% &
	\ IPC% = CODE% + CODEOFFSET% &
	\ CDT%=FNB1%(IPC%) &
	\ PRINT "Op: "; CDT%, CDT%(CDT%), PPCD$(CDT%) IF DEBUG% &
	\ ON CDT%(CDT%) AND 127% GOSUB &
		6000,6010,6020,6030,6030,6040,6050,6060,6070,6080, &
		6100,6110,6120,6130,6140,6150,6160,6170,6180,6190, &
		6200,6210,6220,6230,6240,6250,6260,6270,6280,6290, &
		6320,6340,6350,6360,6370,6380,6390,6400,6410,6420, &
		6430,6440,6450,6460,6470,6480,6490 &

4050	CODEOFFSET% = CODEOFFSET%+NEXTCODE%+1% &
	\ IF CODEOFFSET% >= ENDCODE% THEN 4020 ELSE 4040 &

4100	! &
	! Data Statement &
	!
4110	PRIN$ = "DATA " &
	\ PRIN$ = PRIN$ + CHR$(FNB1%(I%)) &
		FOR I% = IPC% + 1% TO IPC% + ENDCODE% - 1% &
	\ GOSUB 5000 &
	\ GOTO 4020 &

4200	! &

4210	GOSUB 5100 &
	\ CLOSE 1%,2%,3%,4%,5% &
	\ GOTO 32767 &

5000	! &
	! Store lines &
	!
5010	FIELD.STARTED% = 0% &
	\ RETURN IF CDT% = 10% AND LIN%<>0%
5020	IF STMT.TYPE% = 0% OR STMT.TYPE% = -1% THEN &
	LIN% = LIN% + 1% &
	\ LIN$(LIN%) = PRIN$ &
	\ Q% = X2% &
	\ X2% = X1% &
	\ X1%, X2% = 0% IF X1% = Q% &
	\ RETURN
5030	IF STMT.TYPE% = -2% THEN &
	LIN$(LIN%) = LIN$(LIN%) + PRIN$ &
	\ STMT.TYPE% = -1% &
	\ RETURN
5040	IF STMT.TYPE% = -3% THEN &
	LIN$(LIN%) = PRIN$ + LIN$(LIN%) &
	\ STMT.TYPE% = -1% &
	\ RETURN
5090	RETURN &

5100	! &
	! Print line &
	!
5110	X1%, STACK% = 0% &
	\ IF OLDLINE% = 0% THEN OLDLINE%=LINENO% &
	\ RETURN
5115	PRINT NUM1$(OLDLINE%);" ";
5120	FOR Q% = 1% TO LIN% &
	\ GOTO 5180 IF LIN$(Q%)="!" AND Q%<>1% &
	\ PRINT CHR$(10%); CHR$(13%); CHR$(0%); CHR$(9%); IF Q%<>1% &
	\ PRINT "\ "; IF Q%<>1% AND LIN%(Q%)<>-1%
5130	PRINT CVT$$(LIN$(Q%),408%);
5180	LIN%(Q%)=0% &
	\ NEXT Q% &
	\ LIN% = 0% &
	\ OLDLINE% = LINENO% &
	\ PRINT &
	\ RETURN &

6000	PRINT "Unrecognized code "; CDT% &
	\ print "Test 1:"; FNVAR$(FNI%(1%)); " 2:"; &
		FNVAR$(FNI%(2%)); " 3:"; FNVAR$(FNI%(3%)); CDT% &
	\ print "Test("; iz%; ;")"; FNB1%(IPC% + iz%) for iz% = 0% to 10% &
	\ GOSUB 5100 &
	\ STOP &
	\ RETURN				! (1) Illegal &

6010	NEXTCODE% = CDT%(CDT%)/256% &
	\ RETURN				! (2) Ignore &

6020	STACK% = STACK% + 1% &
	\ VALUE$ = FNVAL$(FNI%(1%), CDT%(CDT%)) &
	\ STACK$(STACK%) = VALUE$ &
	\ NEXTCODE% = 2% &
	\ RETURN				! (3) Push onto stack &

6030	STACK% = STACK% - 1% &
	\ STACK$(STACK%) = PPCD$(CDT%) + STACK$(STACK%) + &
		CLOS$(CDT%(CDT%) AND 1%) &
	\ NEXTCODE% = 0% &
	\ RETURN				! (4,5) Functions &

6040	Q% = FNB1%(IPC%+1%) &
	\ RETURN IF CDT%=224% AND (Q%=29% OR Q%=30% OR Q%=31%) &
	\ STACK% = STACK% - 1% &
	\ STACK$(STACK%) = STACK$(STACK%) + " " + PPCD$(CDT%) + " " + &
		STACK$(STACK% + 1%) &
	\ RETURN				! (6) AND, OR, XOR, etc. &

6050	STACK% = STACK% - 2% &
	\ STACK$(STACK%) = PPCD$(CDT%) + STACK$(STACK%) + ", " + &
		STACK$(STACK% + 1%) + ")" &
	\ RETURN				! (7) Two operations &

6060	PRIN$ = PPCD$(CDT%) &
	\ PRIN$ = PRIN$ +  " " + STACK$(STACK%) IF STACK% > 0% &
	\ STACK% = STACK% - 1% IF STACK% > 0% &
	\ GOSUB 5000 &
	\ RETURN				! (8) Finish line &

6070	PRIN$ = "ON " + STACK$(STACK%) + " " + PPCD$(CDT%) + " " &
	\ STACK% = STACK% - 1% &
	\ W1%=FNB1%(IPC% + 1%) &
	\ PRIN$ = PRIN$ + FNLIN$(FNI%(W1%)) &
	\ PRIN$ = PRIN$ + ", " + FNLIN$(FNI%(W%)) &
		FOR W% = W1% - 2% TO 2% STEP -2% &
	\ GOSUB 5000 &
	\ NEXTCODE% = W1% + 1% &
	\ RETURN				! (9) ON GOTO, ON GOSUB &

6080	PRIN$=PPCD$(CDT%) + " " + FNVAR$(FNI%(1%)) + ADDOTH$ + " = " + STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ ADDOTH$="" &
	\ GOSUB 5000 &
	\ NEXTCODE% = 2% &
	\ RETURN				! (10) Set equil &

6100	STACK% = STACK% - 3% &
	\ STACK$(STACK%) = PPCD$(CDT%) + STACK$(STACK%) + ", " + &
		STACK$(STACK% + 1%) + ", "+ STACK$(STACK% + 2%) + ")" &
	\ RETURN				! (11) Three oper funct. &

6110	PRIN$ = PPCD$(CDT%) + " " + FNLIN$(FNI%(1%)) &
	\ GOSUB 5000 &
	\ NEXTCODE%=2% &
	\ RETURN				! (12) Goto, gosub, etc &

6120	PRIN$ = FNVAR$(FNI%(1%)) + FNIN.SUB$(Q1%) + ")" &
	\ STACK% = STACK% + 1% &
	\ STACK$(STACK%) = PRIN$ &
	\ NEXTCODE%=2% &
	\ RETURN				! (13) ARRAY() &

6130	PRIN$=PPCD$(CDT%) + " " + FNVAR$(FNI%(1%)) + FNIN.SUB$(Q1%) + ")" &
		+ ADDOTH$ + " = " + STACK$(STACK%) &
	\ ADDOTH$ = "" &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ NEXTCODE%=2% &
	\ RETURN				! (14) ARRAY() = &

6140	VAR$=FNVAR$(FNI%(1%)) &
	\ PRIN$="CHANGE "+STACK$(STACK%)+" TO "+ &
		LEFT(VAR$,INSTR(1%,VAR$,"(")-1%) &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ NEXTCODE%=2% &
	\ RETURN				! (15) CHANGE $ TO % &

6150	RETURN UNLESS STACK% &
	\ CHANNEL$ = STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ RETURN				! (16) Load channel &

6160	PRIN$ = "PRINT " &
	\ PRIN$ = PRIN$ + "#"+CHANNEL$ + ", " IF CHANNEL$<>"0" AND &
		CHANNEL$<>"0%" AND CHANNEL$ <> "" &
	\ PRIN$ = PRIN$ + PPPP$ &
	\ PPPP$ = "" &
	\ PRIN$ = PRIN$ + STACK$(STACK%) &
	\ PRIN$ = PRIN$ + ";" UNLESS CDT% = 255% &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ RETURN				! (17) DUMP BUFFER &

6170	Q%=0% &
	\ Q%=-1% IF INSTR(1%,";,",RIGHT(LIN$(LIN%),LEN(LIN$(LIN%)))) &
	\ LIN$(LIN%)=LEFT(LIN$(LIN%),LEN(LIN$(LIN%))+Q%)+PPCD$(CDT%) &
	\ RETURN				! (18) TAB, RETURN, ETC &

6180	STACK%=STACK%+1% &
	\ STACK$(STACK%) = PPCD$(CDT%) &
	\ RETURN				! (19) Push constant &

6190	PRIN$ = PPCD$(CDT%) + " #" + STACK$(STACK% - 4%) &
	\ PRIN$ = PRIN$ + ", RECORD " + STACK$(STACK% - 3%) &
		IF STACK$(STACK% - 3%) <> "0% &
	\ PRIN$ = PRIN$ + ", ??????" + STACK$(STACK% - 2%) &
		IF STACK$(STACK% - 2%) <> "0%" &
	\ PRIN$ = PRIN$ + ", COUNT " + STACK$(STACK% - 1%) &
		IF STACK$(STACK% - 1%) <> "0%" &
	\ PRIN$ = PRIN$ + ", USING " + STACK$(STACK%) &
		IF STACK$(STACK%) <> "0%" &
	\ STACK% = STACK% - 5% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 1% &
	\ RETURN				! (20) Get/Put &

6200	Q% = CDT% &
	\ Q% = FNB1%(IPC% + 1%) IF CDT% = 108% &
	\ PRIN$ = "OPEN " + STACK$(STACK% - 5%) + " " + PPCD$(Q%) + &
		" AS FILE " + STACK$(STACK% - 4%) &
	\ PRIN$ = PRIN$ + ", RECORDSIZE " + STACK$(STACK% - 3%) &
		IF STACK$(STACK% - 3%) <> "0%" &
	\ PRIN$ = PRIN$ + ", CLUSTERSIZE " + STACK$(STACK% - 2%) &
		IF STACK$(STACK% - 2%) <> "0%" &
	\ PRIN$ = PRIN$ + ", ???????? " + STACK$(STACK% - 1%) &
		IF STACK$(STACK% - 1%) <> "0%" &
	\ PRIN$ = PRIN$ + ", MODE "+ STACK$(STACK%) &
		IF STACK$(STACK%) <> "0%" &
	\ STACK% = STACK% - 6% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 1% IF CDT% = 108% &
	\ RETURN				! (21) Open file &

6210	NEXTCODE% = 0% &
	\ RETURN UNLESS STACK% &
	\ CHANNEL$ = STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ RETURN				! (22) Set up for input &

6220	Q% = FNB1%(IPC% + 1%) &
	\ PRIN$ = "INPUT " + PPCD$(Q%) + " " &
	\ PRIN$ = PRIN$ + "#" + CHANNEL$ + ", " &
		IF CHANNEL$ <> "" AND CHANNEL$ <> "0%" &
	\ PRIN$ = PRIN$ + FNVAR$(FNI%(3%)) &
	\ GOSUB 5000 &
	\ NEXTCODE% = 4% &
	\ RETURN				! (23) Input &

6230	STACK$(STACK%) = PPCD$(CDT%) + STACK$(STACK%) &
	\ RETURN				! (24) negate &

6240	JUNK$ = FNVAR$(FNI%(1%)) &
	\ PRIN$ = "CHANGE " + LEFT(JUNK$,INSTR(1%,JUNK$,"(")-1%) + &
		 " TO " + FNVAR$(FNI%(4%)) &
	\ GOSUB 5000 &
	\ NEXTCODE% = 5% &
	\ RETURN				! (25) Change % to $ &

6250	PPPP$ = PPPP$ + "RECORD " + STACK$(STACK%) + ", " &
	\ CHANNEL$ = STACK$(STACK%-1%) &
	\ STACK% = STACK% - 2% &
	\ RETURN				! (26) Set record for print &

6260	PRIN$ = "IF " + STACK$(STACK%) + " THEN " &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ LIN%(LIN%+1%)=-1% &
	\ NEXTCODE% = 2% &
	\ RETURN				! (27) IF xxx THEN xxx &

6270	GOTO 6275 IF FNB1%(IPC%+1%)=234% &
	\ PRIN$ = "UNLESS " + STACK$(STACK%) + " THEN " &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ LIN%(LIN%)=-1% &
	\ NEXTCODE% = 3% &
	\ RETURN &

6275	PRIN$ = "UNTIL " + STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ LIN%(LIN%)=-1% &
	\ NEXTCODE% = 1% &
	\ RETURN				! (28) UNLESS xxx THEN xxx &

6280	PRIN$ = " " + PPCD$(CDT%) + " " + STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ NEXTCODE% = 2% &
	\ GOTO 6285 IF FNB1%(IPC%+FNI%(1%)+2%)=87% &
	\ GOTO 6282 IF FNB1%(IPC%+FNI%(1%))=214% &
	\ GOSUB 5000 &
	\ STMT.TYPE% = -3% &
	\ RETURN &

6282	PRIN$ = " WHILE " &
	\ PRIN$ = " UNTIL " IF CDT% = 218% &
	\ PRIN$ = PRIN$ + STACK$(STACK% + 1%) &
	\ GOSUB 5000 &
	\ STMT.TYPE% = -3% &
	\ RETURN &

6285	PRIN$ = PRIN$ + " THEN " &
	\ GOSUB 5000 &
	\ LIN%(LIN%+1%)=-1% &
	\ RETURN				! (29) xxx IF xxx &

6290	IF FIELD.STARTED% = 0% THEN PRIN$ = "FIELD #" + CHANNEL$ &
	\ GOSUB 5000
6300	IF CDT% = 99% THEN &
		PRIN$ = ", " + STACK$(STACK%) + " AS " + FNVAR$(FNI%(1%)) &
	\ STACK% = STACK% - 1% &
	\ STMT.TYPE% = -2% &
	\ GOSUB 5000 &
	\ FIELD.STARTED% = -1% &
	\ NEXTCODE% = 2% &
	\ RETURN &

6310	IF CDT% = 100% OR CDT% = 101%  THEN &
	PRIN$ = FNVAR$(FNI%(1%)) + FNIN.SUB$(Q1%) + ")" &
	\ PRIN$ = ", " + STACK$(STACK%) + " AS " + PRIN$ &
	\ STACK% = STACK% - 1% &
	\ STMT.TYPE% = -2% &
	\ GOSUB 5000 &
	\ FIELD.STARTED% = -1% &
	\ NEXTCODE% = 2% &
	\ RETURN				! (30) FIELD xxx AS xxx &

6320	Q1% = VAL(LEFT(STACK$(STACK%),LEN(STACK$(STACK%))-1%)) &
	\ JUNK$="" &
	\ STACK% = STACK% - 1% &
	\ FOR Q% = 0% TO 4% &
	\ IF Q1%/(4%^Q%) AND 3% THEN JUNK$ = ", " + STACK$(STACK%) + JUNK$ &
	\ STACK% = STACK% - 1% &
	\ NEXT Q%
6330	JUNK$ = "(" + RIGHT(JUNK$,3%) + ")" IF JUNK$ <> "" &
	\ JUNK$ = FNVAR$(FNI%(1%)+4%) + JUNK$ &
	\ STACK% = STACK% + 1% &
	\ STACK$(STACK%) = JUNK$ &
	\ NEXTCODE% = 2% &
	\ RETURN		! (31) Function call &

6340	JUNK$ = "" &
	\ W% = FNW1%(FNI%(1%)+SPDA%+2%) &
	\ W1% = 2% &
	\ WHILE W% AND 3% &
	\ W1% = W1% + 2% &
	\ W% = W% / 4% &
	\ NEXT &
	\ FOR W% = W1% TO 4% STEP -2% &
	\ JUNK$ = JUNK$ + ", " + FNVAR$(FNI%(W%)) &
	\ NEXT W% &
	\ JUNK$ = "(" + RIGHT(JUNK$,3%) + ")" IF JUNK$ <> "" &
	\ PRIN$ = "DEF " + FNVAR$(FNI%(1%)+4%) + JUNK$ &
	\ GOSUB 5000 &
	\ NEXTCODE% = W1% + 1% &
	\ RETURN				! (32) DEF xxx &

6350	NEXTCODE% = 3% &
	\ RETURN				! (33) Push function name &

6360	PRIN$ = "FNEND" &
	\ GOSUB 5000 &
	\ NEXTCODE% = 0% &
	\ RETURN				! (34) FNEND &

6370	IF X1%=247% THEN PRIN$ = ", " + STACK$(STACK%) &
	\ STMT.TYPE% = -2% &
	\ GOSUB 5000 &
	\ RETURN
6375	PRIN$ = "CLOSE " + STACK$(STACK%) &
	\ X1%=247% &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ RETURN				! (35) CLOSE xxx &

6380	PRIN$ = "NAME " + STACK$(STACK% - 1%) + " AS " + STACK$(STACK%) &
	\ STACK% = STACK% - 2% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 0% &
	\ RETURN				! (36) NAME xxx AS xxx &

6390	PRIN$ = "CHAIN " + STACK$(STACK% - 1%) + " " + STACK$(STACK%) &
	\ STACK% = STACK% - 2% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 0% &
	\ RETURN				! (37) CHAIN xxx xxx &

6400	PRIN$ = " ELSE" &
	\ STMT.TYPE% = -2% &
	\ GOSUB 5000 &
	\ LIN%(LIN%+1%)=-1% &
	\ NEXTCODE% = 0% &
	\ RETURN				! (38) ELSE xxx &

6410	JUNK$ = FNVAR$(FNI%(1%)) &
	\ PRIN$ = "MAT " + LEFT(JUNK$, LEN(JUNK$) - 1%) + " = " + &
		PPCD$(CDT%) &
	\ JUNK$ = FNIN.SUB$(Q1%) &
	\ PRIN$ = PRIN$ + "(" + JUNK$ + ")" UNLESS FNB1%(IPC% - 1%) = 224% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 2% &
	\ STACK% = STACK% + FNB1%(IPC% - 1%)<>224% &
	\ RETURN				! (39) MAT xxx = IDN &

6420	JUNK$ = FNVAR$(FNI%(3%)) &
	\ PRIN$ = "MAT " + LEFT(JUNK$, LEN(JUNK$) - 1%) + " = " + &
		PPCD$(CDT%) &
	\ JUNK$ = FNVAR$(FNI%(1%)) &
	\ PRIN$ = PRIN$ + LEFT(JUNK$,LEN(JUNK$)-1%) + ")" &
	\ GOSUB 5000 &
	\ NEXTCODE% = 4% &
	\ RETURN				! (40) MAT xxx = TRN(xxx) &

6430	Q%=LIN%-1% &
	\ Q% = Q% - 1% IF LEFT(LIN$(Q%),3%)="  =" &
	\ PRIN$ = " FOR " + LIN$(Q%) + " TO " + RIGHT(LIN$(Q%+1%),5%) &
	\ PRIN$ = PRIN$ + " STEP " + RIGHT(LIN$(Q%+2%),5%) IF Q%=LIN%-2% &
	\ LIN% = Q% - 1% &
	\ GOSUB 5000 &
	\ STMT.TYPE% = -3% IF CDT% = 235% &
	\ NEXTCODE% = 6% &
	\ RETURN				! (41) FOR xxx=xxx TO xxx STEP xxx &

6440	PRIN$ = "NEXT " + FNVAR$(FNI%(1%)) &
	\ GOSUB 5000 &
	\ NEXTCODE% = 6% &
	\ RETURN				! (41) NEXT xxx &

6450	STACK% = STACK% + 1% &
	\ STACK$(STACK%) = NUM1$(CVT$F(STRING$(6%,0%)+CVT%$(FNI%(1%))))+"." &
	\ NEXTCODE% = 2% &
	\ RETURN				! (43) Push float const &

6460	ADDOTH$ = ", " +  FNVAR$(FNI%(1%)) + FNIN.SUB$(Q1%) + ")" + ADDOTH$ &
	\ NEXTCODE% = 2% &
	\ RETURN				! (44) Addothers &

6470	ADDOTH$ = ", "+ FNVAR$(FNI%(1%)) + ADDOTH$ &
	\ NEXTCODE% = 2% &
	\ RETURN				! (45) Add other &

6480	PRIN$ = PPCD$(CDT%) + " " + STACK$(STACK%) &
	\ STACK% = STACK% - 1% &
	\ GOSUB 5000 &
	\ NEXTCODE% = 2% &
	\ RETURN				! (46) While &

6490	CTR%=FNB1%(IPC% + 1%) &
\ PRINT "TEST READ"; CTR% &
\ print "Test("; iz%; ;")"; FNB1%(IPC% + iz%) for iz% = 0% to 10% &
	\ PRIN$ = PPCD$(CDT%) + " " + FNVAR$(FNI%(2%)) &
	\ GOSUB 5000 &
	\ NEXTCODE% = 3% &
	\ RETURN				! (47) Read &

12000	DEF FNVAL$(ADDR%, TYPE%) &
	\ VALUE$ = "" &
	\ VALUE$ = FNVAR$(ADDR%) IF (TYPE% AND 256%)<>0% &
	\ VALUE$ = FNINT$(ADDR%) IF (TYPE% AND 512%)<>0% AND VALUE$ = "" &
	\ VALUE$ = FNFLT$(ADDR%) IF (TYPE% AND 1024%)<>0% AND VALUE$ = "" &
	\ VALUE$ = FNSTR$(ADDR%) IF (TYPE% AND 2048%)<>0% AND VALUE$ = "" &
	\ FNVAL$ = VALUE$ &
	\ FNEND &

12010	DEF FNVAR$(ADDR%) &
	\ FOR Q1% = 1% TO ADDR.TOT% &
	\ GOTO 12015 IF ADDR%(Q1%) = ADDR% &
	\ NEXT Q1% &
	\ FNVAR$ = "" &
	\ GOTO 12016
12015	FNVAR$ = VAR$(Q1%)
12016	FNEND &

12020	DEF FNINT$(ADDR%) = NUM1$(ADDR%)+"%" &

12030	DEF FNFLT$(ADDR%) \ JUNK$ = "" &
	\ JUNK$ = CVT%$(FNW1%(SPDA% + ADDR% + Q1%)) + JUNK$ &
		FOR Q1% = 0% TO 7% STEP 2% &
	\ JUNK$ = NUM1$(CVT$F(JUNK$)) &
	\ JUNK$ = JUNK$ + "." IF INSTR(1%,JUNK$,".") = 0% &
	\ FNFLT$ = JUNK$ &
	\ FNEND &

12100	DEF FNSTR$(ADDR%)
12110	REHASH% = FNW0%(SPDA% + ADDR% + 2%) + SPDA% + ADDR% &
	\ JUNK$="" &
	\ IF FNW0%(SPDA% + ADDR% + 4%) > 128% THEN JUNK$="???" ELSE &
		JUNK$ = JUNK$ + CHR$(FNB1%(I%)) FOR I%=REHASH% &
		TO REHASH% + FNW0%(SPDA% + ADDR% + 4%) - 1% &

12120	UNLESS INSTR(1%, JUNK$, '"') THEN JUNK$ = '"' + JUNK$ + '"' &
	ELSE	JUNK$ = "'" + JUNK$ + "'"
12130	FNSTR$=JUNK$ &
	\ FNEND &

12200	DEF FNLIN$(ADDR%)
12210	A%=0% &
	\ A%=FNW0%(ADDR% + SPTA% + 10%) IF ADDR% &
	\ FNLIN$ = NUM1$(A%)
12220	FNEND &

12300	DEF FNIN.SUB$(Q1%) &
	\ JUNK$ = "" &
	\ Q%=INSTR(1%,DIM.IT$(Q1%),"(") &
	\ Q% = INSTR(Q%,DIM.IT$(Q1%),",") &
	\ Q%=-1% IF Q% &
	\ JUNK$ = STACK$(STACK% - 1%) + ", " IF Q% &
	\ JUNK$ = JUNK$ + STACK$(STACK%) &
	\ STACK% = STACK% - 1% + Q% &
	\ FNIN.SUB$ = JUNK$ &
	\ FNEND
14100	DEF FNW0%(A%) = P0%(A%/2% - 256%) &

14200	DEF FNW1%(A%) = P1%(A%/2% - 256%) &

14300	DEF FNB0%(A%) &

14310	Q% = P0%(A%/2% - 256%) &
	\ IF A% AND 1% &
	    THEN FNB0% = SWAP%(Q%) AND 255% &
	    ELSE FNB0% = Q% AND 255% &
		! GET THE CORRECT PIECE OF THE ACTION &

14320	FNEND &

14400	DEF FNB1%(A%) &

14410	Q% = P1%(A%/2% - 256%) &
	\ IF A% AND 1% &
	    THEN FNB1% = SWAP%(Q%) AND 255% &
	    ELSE FNB1% = Q% AND 255% &

14420	FNEND &

14500	DEF FNI%(A%) &

14510	A% = A% + IPC% &
	\ Q% = A%/2% - 256% &
	\ IF A% AND 1% &
	    THEN FNI%	= (P1%(Q%) AND -256%) + (P1%(Q%+1%) AND 255%) &
	    ELSE FNI%	= SWAP%(P1%(Q%)) &

14520	FNEND &

19000	! &

19010	RESUME 19020 &

19020	E0% = ERR\ E1% = ERL &
	\ ON ERROR GOTO 0 &
	\ PRINT "DECOM 	?Fatal error"; E0%; &
			"at line"; E1%; " -- "; FNERR$(E0%) &
	\ PRINT "CDT% ="; CDT% &
	\ GOSUB 5100 &

19080	CLOSE Q% FOR Q% = 0% TO 12% &

19090	GOTO 32767 &
		! CAN'T DO MORE &

19900	DEF FNERR$(Q%) &
		= CVT$$(RIGHT(SYS(CHR$(6%)+CHR$(9%)+CHR$(Q%)),3%),4%) &

20000	! &

20010	DATA	-24,DET,	-60,ERL,	-26,ERR, &
		-58,LINE,	-30,NUM,	-34,NUM2, &
		-16,PI,		-54,RECOUNT,	-62,STATUS, &
		  0,"" &

21200	DATA 0017,0018,0018,0008,0001,0001,0004,0004,0004,0004
21201	DATA 0008,0004,0004,0004,0004,0004,0008,0008,0771,0008
21202	DATA 0008,0012,0009,0015,0025,0001,0001,0001,0001,0039
21203	DATA 0039,0039,0040,0040,0001,0001,0001,0001,0001,0001
21204	DATA 0021,0021,0021,0036,0008,0002,0037,0007,0009,0012
21205	DATA 0002,0031,0022,0018,0033,0008,0001,0001,0001,0001
21206	DATA 0001,0001,0001,0001,0001,0001,0001,0001,0001,0001
21207	DATA 0001,0001,0001,0001,0001,0001,0001,0019,0019,0001
21208	DATA 0001,0019,0001,0001,0001,0001,0001,0038,0027,0004
21209	DATA 0004,0004,0004,0010,0014,0014,0001,0014,0014,0030
21210	DATA 0030,0030,0020,0020,0001,0011,0001,0007,0021,0007
21211	DATA 0026,0007,0004,0006,0007,0011,0011,0007,0007,0004
21212	DATA 0043,0028,0001,0001,0001,0001,0001,0001,0006,0006
21213	DATA 0006,0006,0001,0001,0006,0006,0006,0006,0004,0004
21214	DATA 0006,0006,0006,0006,0006,0001,0006,0006,0006,0006
21215	DATA 0006,0006,0006,0006,0006,0006,0006,0006,0006,0006
21216	DATA 0006,0001,0024,1283,0771,2307,0010,0010,0010,0001
21217	DATA 0045,0045,0013,0013,0014,0001,0044,0001,0010,0010
21218	DATA 0045,0045,0014,0014,0044,0044,0001,0001,0001,0001
21219	DATA 0019,0002,0002,0001,0004,0004,0004,0004,0004,0004
21220	DATA 0004,0004,0004,0004,0004,0007,0007,0011,0011,0004
21221	DATA 0004,0004,0004,0004,0514,0012,0029,0001,0029,0001
21222	DATA 0515,0034,0012,0008,0006,0006,0006,0006,0006,0006
21223	DATA 0047,0047,0047,0006,0046,0041,0001,0001,0001,0041
21224	DATA 1538,0001,0001,1538,0001,0042,0001,0035,0023,0016
21225	DATA 0001,0001,0001,0022,0001,0017 &

32767	END
