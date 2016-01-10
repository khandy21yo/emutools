/********* Ruler stuff not handled: H on decmate, L in file, hyphenation zone,
 > on decmate, B in file, right justified tab.  Probably others
**********/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


/*
 * Special definitions
 */
#define BOLD		1
#define UNDERLINE	2
#define SUBSCRIPT	4
#define SUPERSCRIPT	8
#define ITALICS		16

/*
 * File open
 */
FILE *infile;
int overstrike(char first, char second);
void addbuffer(const unsigned char *text, int len);
void altaddbuffer(const unsigned char *text, int len);

/*
 * Sector storage
 */
unsigned int lowword;
unsigned int highword;
unsigned int buffer[270];

/*
 * Master Index storage
 */
int index_count = 0;
unsigned int windex[256];

/*
 * Document index storage
 */
int docindex_count = 0;
unsigned int docindex[800];
int actdocnum = 0;

/*
 * Character processing
 */
int nextblock;
int nextchar;
int highlow;

/*
 * Flags, etc.
 */
FILE *destfd;
int cur_flags, ch_flags;
int cur_rn,cur_lm,cur_sp,cur_wm,cur_rm,cur_jf,cur_ts[140];
int rn,lm,sp,wm,rm,jf,ts[140];
int forceindent = 0;
int overstrikeflag = 0;

/*
 * Buffering area
 */
int wpbuffcount = 0, altbufflag = 0, altwpbuffcount = 0;
unsigned char wpbuffer[4096], altwpbuffer[4096];

/*
 * String semi-constants
 */
const unsigned char minprefix[] =
	"\xff\x57\x50\x43"
	 "\x4c\x00\x00\x00"
	 "\x01\x0a\x00\x00"
	 "\x00\x00\x00\x00"
	"\xfb\xff\x05\x00"
	 "\x32\x00\x00\x00"
	 "\x00\x00\x06\x00"
	 "\x08\x00\x00\x00"
	"\x42\x00\x00\x00"
	 "\x08\x00\x02\x00"
	 "\x00\x00\x4a\x00"
	 "\x00\x00\x00\x00"
	"\x00\x00\x00\x00"
	 "\x00\x00\x00\x00"
	 "\x00\x00\x00\x00"
	 "\x00\x00\x00\x00"
	"\x00\x00\x08\x00"
	 "\x7c\x00\x78\x00"
	 "\x00\x00\x00\x00";
unsigned char wp_margin[] = {208, 1, 12,0, 0,0, 0,0, 0,0, 0,0, 12,0, 1, 208}; /* Modifiable */
const unsigned char wp_tab[] = {193,0,0,0,0,0,0,0,193};
const unsigned char wp_marginrelease[] = {193,0200,0,0,0,0,0,0,193};
const unsigned char wp_center[] = {193,0350,0,0,0,0,0,0,193};
const unsigned char wp_boldon[] = {195,0xc,195};
const unsigned char wp_boldoff[] = {196,0xc,196};
const unsigned char wp_underlineon[] = {195,0xe,195};
const unsigned char wp_underlineoff[] = {196,0xe,196};
const unsigned char wp_italicson[] = {195,0x8,195};
const unsigned char wp_italicsoff[] = {196,0x8,196};
const unsigned char wp_resetpage[] = {211,4, 8,0, 0,0, 1,0, 8,0, 4,211};
const unsigned char wp_hardspace[] = {160};
const unsigned char wp_subscripton[] = {195,0x5,195};
const unsigned char wp_subscriptoff[] = {196,0x5,196};
const unsigned char wp_superscripton[] = {195,0x6,195};
const unsigned char wp_superscriptoff[] = {196,0x6,196};

/*
 * Main loop
 */
main(int argc, char *argv[])
{
/*	char fname[80]; */
	char *fname;
	int loop;

/*	printf("File name: "); */
/*	scanf("%s", &fname); */

	if (argc != 2)
	{
		printf("Invalid number of arguements!\n");
		exit(1);
	}
	fname = argv[1];

	if ((infile = fopen(fname,"rb")) == 0)
	{
		printf("Unable to open source file!\n");
		return(1);
	}

	/*
	 * Load in master index
	 */
	read_index();

	/*
	 * Let's look at all documents
	 */
	for (loop = 0; loop<index_count; loop++)
	{
		read_document(loop);
	}

	/*
	 * End of program
	 */
	return(0);
}

/*
 * Read in a sector, using the Decmate II sector numbering scheme
 */
int read_sector(unsigned int secnum)
{
	unsigned long secoff;
	int loop;
	unsigned int xx;

	printf("\nSector %d",secnum);

	/*
	 * Calculate location on disk
	 */
	secoff = ((long) secnum + 10L) * 512L;

	if (secnum >= 640)
	{
		loop = 1;
	}
	if ((xx = fseek(infile, secoff, SEEK_SET)) != 0)
	{
		printf("Error in seek for sector %ld\n",secnum);
		return(1);
	}

/*	printf("\nPosition = %ld ", ftell(infile)); */

	/*
	 * Remove four bytes at top of header
	 * (who knows what they mean)
	 */
#ifdef DJG
	lowword = fgetc(infile);
	lowword += (fgetc(infile) & 15) << 8;
	highword = fgetc(infile);
	highword += (fgetc(infile) & 15) << 8;
#endif

/*	printf("low=%d high=%d\n", lowword, highword); */

	/*
	 * Pull in 256 12 bit words
	 */
	for (loop = 0; loop <= 253; loop++)
	{
		/*
		 * Read in one word
		 */
		xx = fgetc(infile);
		xx += (fgetc(infile) & 15) << 8;
		buffer[loop] = xx;
	}

	/*
	 * Succussful completion
	 */
	return(0);
}

/*
 * Load in the index record
 */
int read_index()
{
	int loop;

	/*
	 * Suck in index sector
	 */
	read_sector(2);

#if 1
	printf("Index sector:\n");
	dump_buffer();
#endif

	/*
	 * Get all document pointers
	 */
	for (loop = 8; buffer[loop] != 0; loop++)
	{
		windex[index_count++] = buffer[loop];
	}
	windex[index_count] = 0;

	/*
	 * Write out a short note so we know whats going on
	 */
	printf("\nNumber of documents on disk = %d\n", index_count);

	/*
	 * Sucussful exit
	 */
	return(0);
}

/*
 * Read in and process one entire document
 */
int read_document(int docnum)
{
	char destname[80];
	int i;

	if (read_document_index(docnum) != 0)
	{
		printf("Skipping bad document.\n\n");
		return(1);
	}

	sprintf(destname, "wp%03d.wp", actdocnum);
	if ((destfd = fopen(destname, "wb")) == 0)
	{
		printf("Unable to open output file\n");
		return(1);
	}

	/*
	 * Initilize information (WP defaults)
	 */
	cur_lm=10;
	cur_sp=0;
	cur_wm=10;
	cur_rm=75;
	cur_jf=0;

	for (i = 0; i<=80; i++)
		cur_ts[i] = 0;
	for (i = 0; i<=80; i+=5)
		cur_ts[i] = 1;

	/*
	 * Output the standard prefix
	 */
	addbuffer(minprefix, sizeof(minprefix));
	dumpbuffer();

	read_text();

	dumpbuffer();

	fclose(destfd);

	destfd = stdin;

	return(0);
}

/*
 * Read in document index
 */
int read_document_index(int docnum)
{
	int loop;
	int retvalue = 0;
	int second, third;
	int seccnt;

	/*
	 * Suck in index sector
	 */
	docindex_count = 0;
	read_sector(windex[docnum]);

#if 1
	printf("\nDocument index:\n");
	dump_buffer();
#endif
	/*
	 * Secondary index pointers
	 */
	second = buffer[0];
	third = buffer[1];
	seccnt = buffer[3];

	/*
	 * Get all document pointers
	 */
	for (loop = 43; (buffer[loop] != 0) && (loop <= 255); loop++)
	{
		docindex[docindex_count++] = buffer[loop];
	}

	actdocnum = buffer[9];

	if (second != 0)
	{
		read_sector(second);
		for (loop = 0; (buffer[loop] != 0) && (loop <= 253); loop++)
		{
			docindex[docindex_count++] = buffer[loop];
		}
	}

	if (third != 0)
	{
		read_sector(third);
		for (loop = 0; (buffer[loop] != 0) && (loop <= 253); loop++)
		{
			docindex[docindex_count++] = buffer[loop];
		}
	}

	/*
	 * Write out a short note so we know whats going on
	 */
	printf("\n   Document %d(%d) uses %d(%d) sectors\n",
		docnum+1, actdocnum, docindex_count, seccnt);

	if (seccnt != docindex_count)
	{
		printf("Index sector count does not match document.\n");
		retvalue = 12;
	}
	if (actdocnum > 99)
	{
		printf("Internal document number out of range.\n");
		retvalue = 17;
	}
	if (actdocnum != docnum+1)
	{
		printf("Internal document number inconsistant.\n");
		retvalue=21;
	}
	if (docindex_count == 0)
	{
		printf("Document is empty.\n");
		retvalue=22;
	}

	/*
	 * Sucussful exit
	 */
	return(retvalue);
}

int read_text()
{
	int ch;
	int soft = 0;
	int xcase = 1;
	int tempch;
	int i;
	int tabflag=0;		/* Eat spaces because were in a tab? */

	overstrikeflag = 0;

	/*
	 * Force reading of the first sector for this file
	 */
	nextblock = 0;
	nextchar=9000;
	highlow = 0;

	/*
	 * Examine all characters in the file
	 */
	while ((ch = read_next()) != -1)
	{
reloop:		switch (ch)
		{
		/*
		 * Bad disk block
		 */
		case -1:
			dumpbuffer();
			printf("Error reading sector!\n");
			goto doneread;
		/*
		 * Blank character
		 */
		case 31:
/*			printf("<31>"); */
			break;
		/*
		 * Command character.
		 */
		case 94:
			ch = read_next();
			switch (ch)
			{
			/*
			 * Filler
			 */
			case '^':
				break;
			case '(':
				soft = 1;
				break;
			case ')':
				soft = 0;
				break;
			case '&':
				overstrikeflag = 1;
				break;
			case '\'':
				overstrikeflag = 0;
				break;
			case '/':
				/*
				 * <CtrMkr> - Center line
				 */
				tabflag=1;
				insertbuffer(wp_center, sizeof(wp_center), 0);
				break;
			case '+':
				/*
				 * Page - Hard page
				 */
				if ((ch_flags & SUBSCRIPT) != 0)
				{
					/*
					 * <PcbMkr-134>
					 */
					wp_setflags(0, 'Z');
					dumpbuffer();
					altbufflag = -1;
				}
				else
				{
					if (altbufflag == 0)
					{
						tabflag=0;
						wp_setflags(0, 'Z');
						pushbuffer(12);	/* hard page */
						dumpbuffer();
					}
					else
					{
						/*
						 * <PceMkr-136>
						 */
						wp_setflags(0,'Z');
						dumpbuffer();
						altbufflag = 0;
						pcehandler();
					}
				}
				break;

			case '*':
				if ((ch_flags & UNDERLINE) != 0)
				{
					/*
					 * Hyphenation - Let's lose it!
					 */
					tabflag = 1;
				}
				else
				{
					if (soft != 0)
					{
						tabflag = 1;
					}
					else
					{
						tabflag=0;
						/*
						 * <CarMkr> - Hard return
						 */
						wp_setflags(0, 'Z');
						/*
						 * Let's lose spaces, tabs at the end of a line
						 * just to shorten up the file some.
						 */
						tempch = -1;
						while (tempch)
						{
							ch = tempch = popbuffer();
							switch(tempch)
							{
							/*
							 * Lose a space
							 */
							case ' ':
								break;
							/*
							 * Lose a tab
							 */
							case 193:
								for (i=1; i<=8; i++)
								{
									tempch = popbuffer();
								}
								break;
							default:
								tempch = 0;
								break;
							}
						}
						if (ch != 0)
							pushbuffer(ch);
						pushbuffer(10);
						dumpbuffer();
					}
				}
				break;
			case '.':
				 /*
				  * <CtrMkr> - Center line
				  */
				 tabflag=0;
				 while ((wpbuffer[0] == ' ') && (wpbuffcount >0))
				 {
					deletebuffer(0,1);
				 }
				 insertbuffer(wp_center, sizeof(wp_center), 0);
				 break;

			case '!':
				/*
				 * <BldMkr> - Bold on - next single character
				 */
				wp_setflag(BOLD);
				break;
			case '"':
				/*
				 * <BldMkr> - Bold on - next single character
				 */
				wp_resetflag(BOLD);
				break;
			case '1':
				wp_outattr(ch);
				pushbuffer('[');
				break;
			case '2':
				/*
				 * Vertical line
				 */
				wp_outattr(ch);
				pushbuffer('\\');
				break;
			case '3':
				wp_outattr(ch);
				pushbuffer(']');
				break;
			case '4':
				wp_outattr(ch);
				pushbuffer('~');
				break;
			case '5':
				/*
				 * Underline character?
				 */
				tabflag=0;
				wp_outattr(ch);
				pushbuffer('_');
				break;
			case ',':
				/*
				 * Subscript on - next single character
				 */
				wp_setflag(SUBSCRIPT);
				break;
			case '-':
				/*
				 * Subscript off - next single character
				 */
				wp_resetflag(SUBSCRIPT);
				break;
			case '#':
				/*
				 * <UdlMkr> - Underline marker - next single character
				 */
				wp_setflag(UNDERLINE);
				break;
			case '$':
				/*
				 * <UdlMkr> - Underline marker - next single character
				 */
				wp_resetflag(UNDERLINE);
				break;
			case '%':
				/*
				 * <Tab> - Tab over
				 */
				while ((ch = read_next()) == ' ');
				tabflag=1;
				addbuffer(wp_tab,sizeof(wp_tab));
				goto reloop;
			case '7':
				/*
				 * <RulMkr> - Ruler Marker
				 */
				while ((ch = read_next()) != '@')
				{
					switch (ch)
					{
					case '[':
						xcase = 1;
						break;
					case ']':
						xcase = 0;
						break;
					}
				}

				lm = 0;		/* Left Margin */
				sp = 0;		/* Spacing */
				wm = -1;	/* Wrap Margin? */
				rm = 0;		/* Right Margin */
				jf = 0;		/* Justify */
				for (i=0; i<=80; i++)
					ts[i] = 0;
				i = 0;

				while ((ch = read_next()) != 94)
				{
					if (ch < 65)
					{
						i = (i * 16) + (ch & 15);
					}
					else
					{
 printf("(T-%d%c)",i,ch);
						switch (ch)
						{
						case '[':
							xcase = 1;
							break;
						case ']':
							xcase = 0;
							break;
						case 'C':
							if (i <= 80)
								ts[i] = 1;
							break;
						case 'H':
							if (i <= 80)
								ts[i] = 2;
							break;
						case 'I':
							if (i <= 80)
								ts[i] = 2;
							break;
						case 'A':
							if (i <= 80)
								ts[i] = 2;
							break;
						case 'D':
							lm = i;
							break;
						case 'E':
							rm = i;
							break;
						case 'G':/*W*/
							rm = i;
							break;
						case '?':/*J*/
							rm = i;
							jf=1;
							break;
						default:
							printf("\n<Ruler error %c-%d>\n",ch,i);
							break;
						}
						i=0;
					}
				}
				if (lm < 0)
					lm = 0;
				if (wm < 0)
					wm = lm;
				if (lm <= 80)
					ts[lm] = 1;
				if (wm <= 80)
					ts[wm] = 1;
				wp_setruler();
				dumpbuffer();
				/*
				 * Eat '8' (hopefully) 1
				 */
				ch = read_next();
 printf("\n");
				break;
			default:
				printf("<94-%c>",ch);
				break;
			}
			break;
		/*
		 * Force uppercase
		 */
		case '[':
			xcase = 1;
			break;
		/*
		 * Force lowercase
		 */
		case ']':
			xcase = 0;
			break;
		/*
		 * Spaces
		 */
		case ' ':
			if (tabflag == 0)
				pushbuffer(' ');
			break;
		/*
		 * Must be normal text
		 */
		default:
			tabflag=0;
			if (xcase == 0)
			{
				ch = tolower(ch);
			}
			wp_outattr(ch);
			pushbuffer(ch);
		}
	}

 doneread:
	return(0);
}

/*
 * Pull one character from the file
 */
int read_next()
{
	int ch;

	if (highlow == 1)
	{
		highlow = 0;
		ch = (buffer[nextchar++] & 63) + 31;
	}
	else
	{
		if (nextchar > 253)
		{
			if (nextblock >= docindex_count)
			{
				return(-1);
			}
			else
			{
				read_sector(docindex[nextblock++]);
				nextchar = 0;
			}
		}
		highlow = 1;
		ch = (buffer[nextchar] >> 6) + 31;
	}
	return(ch);
}

/*
 * Output regular text character to wp file
 */
wp_outattr(int ch)
{
	/*
	 * Check for any attributes that need to be changed
	 */
	if (ch_flags != cur_flags)
	{
		wp_setflags(ch_flags, ch);
	}
}


wp_setflags(int new, int ch)
{
	int tempch;

	/*
	 * Should we turn on bold?
	 *	Doesn't change for spaces.
	 */
	if (((new & BOLD) == BOLD) && ((cur_flags & BOLD) != BOLD))
	{
		addbuffer(wp_boldon, sizeof(wp_boldon));
	}

	/*
	 * Should we turn on bold?
	 *	Doesn't change for spaces.
	 */
	if (((new & ITALICS) == ITALICS) && ((cur_flags & ITALICS) != ITALICS))
	{
		addbuffer(wp_italicson, sizeof(wp_italicson));
	}

	/*
	 * Should we turn off bold?
	 *	Doesn't change for spaces.
	 */
	if (((new & BOLD) != BOLD) && ((cur_flags & BOLD) == BOLD))
	{
		addbuffer(wp_boldoff, sizeof(wp_boldoff));
	}

	/*
	 * Should we turn off bold?
	 */
	if (((new & ITALICS) != ITALICS) && ((cur_flags & ITALICS) == ITALICS))
	{
		addbuffer(wp_italicsoff, sizeof(wp_italicsoff));
	}

	/*
	 * Should we turn on superscript?
	 *	Doesn't change for spaces.
	 */
	if (((new & SUPERSCRIPT) == SUPERSCRIPT) && ((cur_flags & SUPERSCRIPT) != SUPERSCRIPT))
	{
		addbuffer(wp_superscripton, sizeof(wp_superscripton));
	}

	/*
	 * Should we turn off superscript?
	 *	Doesn't change for spaces.
	 */
	if (((new & SUPERSCRIPT) != SUPERSCRIPT) && ((cur_flags & SUPERSCRIPT) == SUPERSCRIPT))
	{
		addbuffer(wp_superscriptoff, sizeof(wp_superscriptoff));
	}

	/*
	 * Should we turn on subscript?
	 *	Doesn't change for spaces.
	 */
	if (((new & SUBSCRIPT) == SUBSCRIPT) && ((cur_flags & SUBSCRIPT) != SUBSCRIPT))
	{
		addbuffer(wp_subscripton, sizeof(wp_subscripton));
	}

	/*
	 * Should we turn off subscript?
	 *	Doesn't change for spaces.
	 */
	if (((new & SUBSCRIPT) != SUBSCRIPT) && ((cur_flags & SUBSCRIPT) == SUBSCRIPT))
	{
		addbuffer(wp_subscriptoff, sizeof(wp_subscriptoff));
	}

	/*
	 * Should we turn on underline?
	 */
	if (((new & UNDERLINE) == UNDERLINE) && ((cur_flags & UNDERLINE) != UNDERLINE))
	{
		addbuffer(wp_underlineon, sizeof(wp_underlineon));
	}

	/*
	 * Should we turn off bold?
	 */
	if (((new & UNDERLINE) != UNDERLINE) && ((cur_flags & UNDERLINE) == UNDERLINE))
	{
		addbuffer(wp_underlineoff, sizeof(wp_underlineoff));
	}

	cur_flags = new;

}

wp_setflag(int fl)
{
	ch_flags |= fl;
}

wp_resetflag(int fl)
{
	ch_flags = ch_flags & ~fl;
}

/*
 * DumpBuffer
 */
dumpbuffer()
{
	int tabadj, i;

	/*
	 * Adjust the tabbing enviornment
	 */
	if ((printable() != 0) && (altbufflag == 0))
	{
		tabadj = forceindent;

		if (tabadj < 0)
		{
			for (i = tabadj; i != 0; i++)
			{
				if ((wpbuffer[0] == 193) && (wpbuffer[1] == 0))
				{
					deletebuffer(0,9);
					tabadj++;
				}
			}
			for (i = tabadj; i != 0; i++)
			{
				insertbuffer(wp_marginrelease, sizeof(wp_marginrelease),0);
			}
		}
		else
		{
			for (i = 1; i<= tabadj; i++)
			{
				insertbuffer(wp_tab, sizeof(wp_tab),0);
			}
		}
	}

	if (altbufflag == 0)
	{
		fwrite(wpbuffer, wpbuffcount, 1, destfd);
	}
	else
	{
		altaddbuffer(wpbuffer, wpbuffcount);
	}
	wpbuffcount = 0;
}

/*
 * AddBuffer
 */
void addbuffer(const unsigned char *text, int len)
{
	while (len--)
	{
		wpbuffer[wpbuffcount++] = *text++;
	}
}

/*
 * AltAddBuffer
 */
void altaddbuffer(const unsigned char *text, int len)
{
	while (len--)
	{
		altwpbuffer[altwpbuffcount++] = *text++;
	}
}

/*
 * AltDeleteBuffer
 */
altdeletebuffer(int start, int count)
{
	int i;

	for (i = start + count; i < altwpbuffcount; i++)
	{
		altwpbuffer[i-count] = altwpbuffer[i];
	}
	altwpbuffcount -= count;
}

/*
 * InsertBuffer
 */
insertbuffer(unsigned char *text, int len, int offset)
{
	int tmp;
	for (tmp = wpbuffcount; tmp >= offset; tmp--)
	{
		wpbuffer[tmp+len] = wpbuffer[tmp];
	}

	wpbuffcount+=len;

	while (len--)
	{
		wpbuffer[offset++] = *text++;
	}
}

/*
 * PopBuffer
 */
popbuffer()
{
	if (wpbuffcount != 0)
	{
		return(wpbuffer[--wpbuffcount]);
	}
	else
	{
		return(0);
	}
}

/*
 * Pushbuffer
 */
pushbuffer(int ch)
{
	int ch1,ch2;

	wpbuffer[wpbuffcount++] = ch;
	switch (overstrikeflag)
	{
	case 0:
		break;
	case 1:
		overstrikeflag = 2;
		break;
	case 2:
		ch1 = popbuffer();
		ch2 = popbuffer();
		overstrikeflag = 0;
		overstrike(ch1,ch2);
		overstrikeflag = 2;
		break;
	}
}

/*
 * DeleteBuffer
 */
deletebuffer(int start, int count)
{
	int i;

	for (i = start + count; i < wpbuffcount; i++)
	{
		wpbuffer[i-count] = wpbuffer[i];
	}
	wpbuffcount -= count;
}

/*
 * strnstr
 */
int strnstr(char *source, char *match, int count)
{
	int i;
	int len;

	len = strlen(match);

	for (i = 0; i < count - len; i++)
	{
		if (strncmp(source+i, match, len) == 0)
			return(i);
	}
	return(0);
}

/*
 * Is there something printable in the buffer?
 */
int printable()
{
	if (wpbuffcount == 0)
	{
		return(0);
	}
	else
	{
		switch (wpbuffer[0])
		{
		case 10:
		case 12:
		case 208:
		case 211:
		case 213:
		case 217:
		case 255:
			return(0);
		case 193:
			if (wpbuffer[1] == 0)
				return(1);
			else
				return(0);
		default:
			return(1);
		}
	}
}

/*
 * Handle overstruck characters
 */
struct overstr
{
	char first;
	char second;
	int table;
	int item;
};
struct overstr overlist[] =
{
	'A','\'',1, 26,
	'a','\'',1, 27,
	'A','^', 1, 28,
	'a','^', 1, 29,
	'A','"', 1, 30,
	'a','"', 1, 31,
	'A','`', 1, 32,
	'a','`', 1, 33,
	'E','\'', 1, 40,
	'e','\'', 1, 41,
	'E','^', 1, 42,
	'e','^', 1, 43,
	'E','"', 1, 44,
	'e','"', 1, 45,
	'E','`', 1, 46,
	'e','`', 1, 47,
	'I','\'',1, 48,
	'i','\'',1, 49,
	'I','^', 1, 50,
	'i','^', 1, 51,
	'I','"', 1, 52,
	'i','"', 1, 53,
	'I','`', 1, 54,
	'i','`', 1, 55,
	'N','~', 1, 56,
	'n','~', 1, 57,
	'O','\'',1, 58,
	'o','\'',1, 59,
	'O','^', 1, 60,
	'o','^', 1, 61,
	'O','"', 1, 62,
	'o','"', 1, 63,
	'O','`', 1, 64,
	'o','`', 1, 65,
	'U','`', 1, 66,
	'u','`', 1, 67,
	'U','^', 1, 68,
	'u','^', 1, 69,
	'U','"', 1, 70,
	'u','"', 1, 71,
	'U','`', 1, 72,
	'u','`', 1, 73,
	'Y','\"',1, 74,
	'D','-', 1, 78,
	'O','/', 1, 80,
	'o','/', 1, 81,
	'O','~', 1, 82,
	'o','~', 1, 83,
	'Y','\'',1, 84,
	'y','\'',1, 85,
	'C','\'',1, 96,
	'c','\'',1, 97,
	'C','^', 1,100,
	'c','^', 1,101,
	'G','\'',1,114,
	'g','\'',1,115,
	'G','^', 1,122,
	'g','^', 1,123,
	'H','^', 1,126,
	'h','^', 1,127,
	'I','~', 1,136,
	'i','~', 1,137,
	'J','^', 1,140,
	'j','^', 1,141,
	'L','\'',1,144,
	'l','\'',1,145,
	'N','\'',1,154,
	'n','\'',1,155,
	'R','\'',1,168,
	'r','\'',1,169,
	'S','\'',1,174,
	's','\'',1,175,
	'S','^', 1,180,
	's','^', 1,181,
	'U','~', 1,198,
	'u','~', 1,199,
	'W','^', 1,200,
	'w','^', 1,201,
	'Y','^', 1,202,
	'y','^', 1,203,
	'Z','\'',1,204,
	'z','\'',1,205,
	'R','`', 1,218,
	'r','`', 1,219,
	'Y','`', 1,226,
	'y','`', 1,227,
	0,0,0,0
};

int overstrike(char first, char second)
{
	int loop;

	if ((first == ' ') && (second == ' '))
	{
		pushbuffer(' ');
		goto goodfinish;
	}

	/*
	 * See if we can find it in the table
	 */
	for (loop = 0; overlist[loop].first !=0; loop++)
	{
		if (((overlist[loop].first == first) && (overlist[loop].second == second)) ||
		    ((overlist[loop].first == second) && (overlist[loop].second == first)))
		{
			pushbuffer(192);
			pushbuffer(overlist[loop].item);
			pushbuffer(overlist[loop].table);
			pushbuffer(192);
			goto goodfinish;
		}
	}
	/*
	 * If we can't find it, just slap in the two characters
	 * and hope for the best.
	 */
	pushbuffer(first);
	pushbuffer(second);
goodfinish:
	return(0);
}

/*
 * Let's see what we can do about the rulers
 */
wp_setruler()
{
	int wpu;
	int flag;
	int i;
	int tmp;
	int tbuff[40];
	int tcount = 0;

	/*
	 * Left and right margins
	 */
	if ((cur_lm != lm) || (cur_rm != rm))
	{
		wpu = wm * 120;			/* Wrap margin */
		wp_margin[8] = wpu & 255;
		wp_margin[9] = wpu >> 8;
		wpu = (85 - rm) * 120;		/* Right Margin */
		wp_margin[10] = wpu & 255;
		wp_margin[11] = wpu >> 8;
		addbuffer(wp_margin, sizeof(wp_margin));
		cur_lm = lm;
		cur_rm = rm;
		cur_wm = wm;
	}

	/*
	 * See if the rulers changed
	 */
	for (i = 0; i<40; i++)
		tbuff[i] = 0;

	flag = 0;
	for (i=1; i<=80; i++)
	{
		if (ts[i] != cur_ts[i])
			flag = 1;
		if (ts[i] != 0)
		{
			tbuff[tcount++] = 120 * i;
		}
		cur_ts[i] = ts[i];
	}
	if (flag)
	{
		pushbuffer(208);
		pushbuffer(4);
		pushbuffer(204 & 255);		/* Length */
		pushbuffer(204 >> 8);
		for (i=0; i<=39; i++)		/* Old Tab Positions */
		{
			pushbuffer(0);
			pushbuffer(0);
		}
		for (i=0; i<=39; i += 2)	/* Old tab types */
			pushbuffer(0);
		for (i=0; i<=39; i++)           /* New tab positions */
		{
			if (tbuff[i] == 0)
				tmp = -1;
			else
				tmp = tbuff[i];
			pushbuffer(tmp & 255);
			pushbuffer(tmp >> 8);
		}
		for (i=0; i<39; i += 2)		/* New tab types */
		{
			pushbuffer(0);
		}
		pushbuffer(204 & 255);		/* Length */
		pushbuffer(204 >> 8);
		pushbuffer(4);
		pushbuffer(208);
		dumpbuffer();

		/*
		 * Calculate forced indent
		 */
		forceindent = 0;
		if (lm < wm)
		{
			for (i = lm+1; i<= wm; i++)
			{
				if (ts[i] != 0)
					forceindent--;
			}
		}
		if (lm > wm)
		{
			for (i = lm+1; i<= lm; i++)
			{
				if (ts[i] != 0)
					forceindent++;
			}
		}
	}
}

dump_buffer()
{
	int loop1,loop2;

	for (loop1 = 0; loop1 <= 255; loop1+=10)
		{
		printf("%4u:",loop1);
		for (loop2=0; loop2 <= 9; loop2++)
		{
			printf("%5u", buffer[loop1+loop2]);
		}
		printf(" :");
		for (loop2 = 0; loop2 <= 9; loop2++)
		{
			printf("%c%c",
				((buffer[loop1+loop2] >> 6) & 63) + 31,
				(buffer[loop1+loop2] & 63) + 31);
		}
		printf(":\n");
	}
	printf("\n");
}
/*
 * pce handler
 */
pcehandler()
{
	int i;

	/*
	 * Reset Page Number
	 */
	if (strncmp(altwpbuffer, "RESET", 5) == 0)
	{
		addbuffer(wp_resetpage, sizeof(wp_resetpage));
		dumpbuffer();
	}

	/*
	 * Top of page format
	 */
	if (strncmp(altwpbuffer, "TOP", 3) == 0)
	{
		while ((i = strnstr(altwpbuffer, "\\p", altwpbuffcount)) != 0)
		{
			altwpbuffer[i] = 2;	/* Page # */
			altdeletebuffer(i+1,1);	/* Lose one character */
		}

		/*
		 * Create information
		 */
		pushbuffer(213);
		pushbuffer(0);
		pushbuffer((altwpbuffcount-4+22) & 255);	/* Size in bytes */
		pushbuffer((altwpbuffcount-4+22) / 255);
		pushbuffer(0);			/* Old Occurence flag */
		pushbuffer(0);			/* Old formatter lines */
		pushbuffer(0);
		pushbuffer(0);			/* Old position */
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(1);			/* New occurence flag */
		pushbuffer(0);			/* New formatter lines */
		pushbuffer(0);
		pushbuffer(0);			/* New position */
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);			/* # boxes */
		pushbuffer(0);
		pushbuffer(0);			/* Hash value */
		pushbuffer(0);
		for (i = 4; i < altwpbuffcount; i++)
		{
			pushbuffer(altwpbuffer[i]);
		}
		pushbuffer((altwpbuffcount-4+22) & 255);	/* Size in bytes */
		pushbuffer((altwpbuffcount-4+22) / 255);
		pushbuffer(0);
		pushbuffer(213);
		dumpbuffer();
	}

	/*
	 * Bottom of page format
	 */
	if (strncmp(altwpbuffer, "BOTTOM", 6) == 0)
	{
		while ((i = strnstr(altwpbuffer, "\\p", altwpbuffcount)) != 0)
		{
			altwpbuffer[i] = 2;	/* Page # */
			altdeletebuffer(i+1,1);	/* Lose one character */
		}

		/*
		 * Create information
		 */
		pushbuffer(213);
		pushbuffer(2);
		pushbuffer((altwpbuffcount-7+22) & 255);	/* Size in bytes */
		pushbuffer((altwpbuffcount-7+22) / 255);
		pushbuffer(0);			/* Old Occurence flag */
		pushbuffer(0);			/* Old formatter lines */
		pushbuffer(0);
		pushbuffer(0);			/* Old position */
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(1);			/* New occurence flag */
		pushbuffer(0);			/* New formatter lines */
		pushbuffer(0);
		pushbuffer(0);			/* New position */
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);
		pushbuffer(0);			/* # boxes */
		pushbuffer(0);
		pushbuffer(0);			/* Hash value */
		pushbuffer(0);
		for (i = 7; i < altwpbuffcount; i++)
		{
			pushbuffer(altwpbuffer[i]);
		}
		pushbuffer((altwpbuffcount-7+22) & 255);	/* Size in bytes */
		pushbuffer((altwpbuffcount-7+22) / 255);
		pushbuffer(2);
		pushbuffer(213);
		dumpbuffer();
	}
	altwpbuffcount = 0;
}
