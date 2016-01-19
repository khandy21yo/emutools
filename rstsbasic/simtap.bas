10	DEVICE$="SY:"           !Temp file storage device &
\	TAPCHN%=1% &
\	DATCHN%=3% &
\	DFE% = 0% &
\	BELL$=CHR$(7%) &
\	ON ERROR GOTO 32000 &
	! &
	! Define channel numbers for tape I/O, record length file, and &
	! tape data file.  Enable error trapping. &
	! &

100	PRINT "What is the input device <MS0:> "; &
\	INPUT LINE MSIN$ &
\	MSIN$ = CVT$$(MSIN$, -1%) &
\	MSIN$ = "MS0:" IF MSIN$ = "" &
\	INPUT "Mount the tape to copy FROM and press RETURN"; Z$ &
\	OPEN MSIN$ FOR INPUT AS FILE TAPCHN%, RECORDSIZE 4096% &
\	Z%=INSTR(1%,Z$,":") &
\	IF Z% &
	THEN	DEVICE$=LEFT(Z$,Z%) &
\		Z$=RIGHT(Z$,Z%+1%) &

110	!ELSE &
	INPUT "Input tape density <1600>"; Z% &
\	Z%=FNC.DENSITY%(Z%,1600%) &
\	GOTO 110  IF Z%=0% &
\	Z%=MAGTAPE(6%,Z%,TAPCHN%) &
\	Z%=MAGTAPE(3%,0%,TAPCHN%) &
	&

120	print "Output File Name "; &
\	INPUT LINE oname$ &
\	oname$ = CVT$$(oname$, -1%) &
\	OPEN oname$ for output AS FILE DATCHN% &

128 ! &

130	GET #TAPCHN% &
\	RECLEN%=RECOUNT &
\	blocount% = blocount% + 1% &
\	EOF% = 0% &

131	print "!"; &
\	print if ccpos(0%) >= 50% &

135	field #tapchn%, reclen% as dxata$ &
\	PRINT #DATCHN%, cvt%$(swap%(reclen%)); cvt%$(0%); &
		dxata$; &
		cvt%$(swap%(reclen%)); cvt%$(0%);

170	GOTO 130 &

200	PRINT BELL$; "Finished" &
\	print blocount%; " Blocks" &
\	print num1$(reccount); " Records" &
\	PRINT &
\	GOTO 32767 &

10000	! FNC.DENSITY% == Parse magtape density value &
	! &
	! Returns the value to use to set the required density in the &
	! MAGTAPE() function. &
	! &
	! Input: &
	!  VALUE%               Value to parse (800, 1600, default, or error) &
	!  DEFAULT%             Default density if VALUE%=0 &
	! &
	! Output: &
	!  FNC.DENSITY%         MAGTAPE() function parameter value.  Returned &
	!                       as 0 if no valid passed in VALUE%. &
	&
	DEF* FNC.DENSITY%(VALUE%,DEFAULT%) &
\		VALUE%=DEFAULT%  IF VALUE%=0% &
\		IF VALUE%=800% &
		THEN	FNC.DENSITY%=12% &
		ELSE	IF VALUE%=1600% &
			THEN	FNC.DENSITY%=256% &
			ELSE	PRINT "?Invalid density.  "; &
					"Please enter 800 or 1600." &
\				FNC.DENSITY%=0% &

10010	FNEND &
	! Use default value if no density specified.  Use a value of 12 for &
	! 800 BPI and 256 for 1600 BPI.  Return 0 for invalid density. &

32000	! ERROR TRAPPING &

32010	IF (ERR=11% AND ERL=100%) &
	THEN	CLOSE TAPCHN%, DATCHN% &
\		RESUME 32767 &
	! If ^Z or ^C, close our temp files and exit. &

32017	IF ERL = 130% AND ERR = 13% &
	THEN	PRINT "Data Format Error In Data Record" &
\		DFE% = 1% &
\		RESUME 128 &
	! Data format error &

32019	IF ERR=11% and EOF% &
	THEN	EOF%=-1% &
\		PRINT "[EOF]" &
\		print #datchn%, cvt%$(0%); cvt%$(0%); &
\		RESUME 200 &
	! EOF while reading magtape: &
	! If the last record also was not an EOF, write an "EOF read" &
	! flag to the record length file and count it as a record. &

32020	IF ERR=11% &
	THEN	EOF%=-1% &
\		PRINT "[EOF]" &
\		print #datchn%, cvt%$(0%); cvt%$(0%); &
\		RESUME 130 &
	! EOF while reading magtape: &
	! If the last record also was not an EOF, write an "EOF read" &
	! flag to the record length file and count it as a record. &

32099	CLOSE #DATCHN% &
\	PRINT "ERROR#"; ERR &
\	ON ERROR GOTO 0 &
	! Any other error is fatal. &

32767	END
