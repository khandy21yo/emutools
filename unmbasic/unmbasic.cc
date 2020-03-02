// unbasic.c
//
//	De-tokenize CP/M basic code
//
// History
//	05/25/2006 - Kevin Handy
//		Author
//

#include <iostream>
#include <fstream>
#include <cstring>        /* for strncpy  */
#include <getopt.h>
#include <cstdlib>

void process(char *fn, bool debug);
void help_message();
int fmsbintoieee(float *src4, float *dest4);
int fieeetomsbin(float *src4, float *dest4);
int dmsbintoieee(double *src8, double *dest8);
int dieeetomsbin(double *src8, double *dest8);

int main(int argc, char* argv[])
{
	int c;
	bool debug = false;

	while ((c = getopt(argc, argv, "hd")) != -1)
	{
		switch(c)
		{
		case 'h':
			help_message();
			exit(1);

		case 'd':
			debug = true;
			break;
		}
	}
	for (int pl = optind; pl < argc; pl++)
	{
		process(argv[pl], debug);
	}
}

void process(char *fn, bool debug)
{
	char ch, ch2, ch3, ch4;
	bool keeplooping = true;

	//
	// Open source file
	//
	if (debug)
	{
		std::cout << "Opening: " << fn << std::endl;
	}
	std::ifstream source(fn, std::ios::binary);
	if (!source)
	{
		std::cerr << "Unable to open " << fn << "." << std::endl;
		return;
	}

	//
	// Header byte
	//
	source.read(&ch, 1);
	if (debug)
	{
		std::cout << "Flag byte: " << ((int)ch) * 0xFF <<  std::endl;
	}
	if (ch != -1)
	{
		std::cerr << "Not a MBASIC program " <<
			((int)ch & 0xff) << std::endl;
		exit(1);
	}

	//
	// 1st Line number (code dup's in case 0 below)
	//
	source.read(&ch3, 1);
	source.read(&ch4, 1);
	source.read(&ch, 1);
	source.read(&ch2, 1);
	if (debug)
	{
		std::cout << "<" << ((int)ch3 & 0xff) << ">" <<
			"<" << ((int)ch4 & 0xff) << ">";
	}
	std::cout << ((int)ch2 & 0xFF) * 256 +
		((int)ch & 0xff) << " ";

	while(keeplooping)
	{
		source.read(&ch, 1);
		if (debug && ((ch < 32) || (ch > 126)))
		{
			std::cout << "<" << ((int)ch & 0xff) << ">";
		}

		switch(((int)ch) & 0xff)
		{
		case 0:
			std::cout << "\n";

			//
			// Line number (code duped in initilization above)
			//
			source.read(&ch3, 1);
			source.read(&ch4, 1);
			source.read(&ch, 1);
			source.read(&ch2, 1);

			if (debug)
			{
				std::cout << "<" << ((int)ch3 & 0xff) << ">" <<
					"<" << ((int)ch4 & 0xff) << ">";
			}

			if ((ch3 == 0) && (ch4 == 0))
			{
				keeplooping = false;
				break;
			}

			std::cout << ((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff) << " ";

			break;

		case 7:		// Bell (Control/G)
		case 9:		// Tab
			std::cout << ch;
			break;

		case 10:	// Cariage return
			std::cout << std::endl;
			break;

		case 11:	// Two byte octal number
			source.read(&ch, 1);
			source.read(&ch2, 1);
			std::cout << "&O" << std::oct <<
				((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff) << std::dec;
			break;

		case 12:	// Two byte hex number
			source.read(&ch, 1);
			source.read(&ch2, 1);
			std::cout << "&H" << std::hex <<
				((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff) << std::dec;
			break;

		case 13:	// Two byte decimal number of unknown use
			source.read(&ch, 1);
			source.read(&ch2, 1);
			std::cout << ((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff);
			break;

		case 14:	// Two byte line number
			source.read(&ch, 1);
			source.read(&ch2, 1);
			std::cout << ((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff);
			break;

		case 15:	// One byte constant numeric
			source.read(&ch, 1);
			std::cout << ((unsigned int)ch & 0xFF);
			break;

		// 16 does really nasty things

		case 17:
			std::cout << "0";
			break;

		case 18:
			std::cout << "1";
			break;

		case 19:
			std::cout << "2";
			break;

		case 20:
			std::cout << "3";
			break;

		case 21:
			std::cout << "4";
			break;

		case 22:
			std::cout << "5";
			break;

		case 23:
			std::cout << "6";
			break;

		case 24:
			std::cout << "7";
			break;

		case 25:
			std::cout << "8";
			break;

		case 26:
			std::cout << "9";
			break;

		case 27:
			std::cout << "10";
			break;

		case 28:	// Two byte constant numeric
			source.read(&ch, 1);
			source.read(&ch2, 1);
			std::cout << ((int)ch2 & 0xFF) * 256 +
				((int)ch & 0xff);
			break;

		case 29:	// Floating point constant
			{
				union
				{
					char buff[4];
					float bufflt;
				};
				float cvtf;

				source.read(buff, 4);

				fmsbintoieee(&bufflt, &cvtf);
				std::cout << cvtf;
			}
			break;

		// 30 is unknown

		case 31:	// Double Floating point constant
				// \bug not always right?
			{
				union
				{
					char buff[8];
					double bufflt;
				};
				double cvtf;

				source.read(buff, 8);

				dmsbintoieee(&bufflt, &cvtf);
				std::cout << cvtf << "#";
			}
			break;

		// 32 to 127 are actual characters

		// 128 is '0'?

		case 129:
			std::cout << "END";
			break;

		case 130:
			std::cout << "FOR";
			break;

		case 131:
			std::cout << "NEXT";
			break;

		case 132:
			std::cout << "DATA";
			break;

		case 133:
			std::cout << "INPUT";
			break;

		case 134:
			std::cout << "DIM";
			break;

		case 135:
			std::cout << "READ";
			break;

		case 136:
			std::cout << "LET";
			break;

		case 137:
			std::cout << "GOTO";
			break;

		case 138:
			std::cout << "RUN";
			break;

		case 139:
			std::cout << "IF";
			break;

		case 140:
			std::cout << "RESTORE";
			break;

		case 141:
			std::cout << "GOSUB";
			break;

		case 142:
			std::cout << "RETURN";
			break;

		case 143:
			std::cout << "REM";
			break;

		case 144:
			std::cout << "STOP";
			break;

		case 145:
			std::cout << "PRINT";
			break;

		case 146:
			std::cout << "CLEAR";
			break;

		case 147:
			std::cout << "LIST";
			break;

		case 148:
			std::cout << "NEW";
			break;

		case 149:
			std::cout << "ON";
			break;

		case 150:
			std::cout << "NULL";
			break;

		case 151:
			std::cout << "WAIT";
			break;

		case 152:
			std::cout << "DEF";
			break;

		case 153:
			std::cout << "POKE";
			break;

		case 154:
			std::cout << "CONT";
			break;

		case 155:	// ??? Not 100% sure of this
			std::cout << "_";
			break;

		// 156 is bad code

		case 157:
			std::cout << "OUT";
			break;

		case 158:
			std::cout << "LPRINT";
			break;

		case 159:
			std::cout << "LLIST";
			break;

		// 160 is bad code (Q)?

		case 161:
			std::cout << "WIDTH";
			break;

		case 162:	/*! \bug Has a leading : */
			std::cout << "ELSE";
			break;

		case 163:
			std::cout << "TRON";
			break;

		case 164:
			std::cout << "TROFF";
			break;

		case 165:
			std::cout << "SWAP";
			break;

		case 166:
			std::cout << "ERASE";
			break;

		case 167:
			std::cout << "EDIT";
			break;

		case 168:
			std::cout << "ERROR";
			break;

		case 169:
			std::cout << "RESUME";
			break;

		case 170:
			std::cout << "DELETE";
			break;

		case 171:
			std::cout << "AUTO";
			break;

		case 172:
			std::cout << "RENUM";
			break;

		case 173:
			std::cout << "DEFSTR";
			break;

		case 174:
			std::cout << "DEFINT";
			break;

		case 175:
			std::cout << "DEFSNG";
			break;

		case 176:
			std::cout << "DEFDBL";
			break;

		case 177:
			std::cout << "LINE";
			break;

		// 178 is bad code (5)??
		// 179 does very weird things

		case 180:
			/*! \bug seems to be a trailing character after WHILE */
			std::cout << "WHILE";
			break;

		case 181:
			std::cout << "WEND";
			break;

		case 182:
			std::cout << "CALL";
			break;

		case 183:
			std::cout << "WRITE";
			break;

		case 184:
			std::cout << "COMMON";
			break;

		case 185:
			std::cout << "CHAIN";
			break;

		case 186:
			std::cout << "OPTION";
			break;

		case 187:
			std::cout << "RANDOMIZE";
			break;

		// 188 is bad code (triangle)

		case 189:
			std::cout << "SYSTEM";
			break;

		// 190 is badcode (G)

		case 191:
			std::cout << "OPEN";
			break;

		case 192:
			std::cout << "FIELD";
			break;

		case 193:
			std::cout << "GET";
			break;

		case 194:
			std::cout << "PUT";
			break;

		case 195:
			std::cout << "CLOSE";
			break;

		case 196:
			std::cout << "LOAD";
			break;

		case 197:
			std::cout << "MERGE";
			break;

		case 198:
			std::cout << "FILES";
			break;

		case 199:
			std::cout << "NAME";
			break;

		case 200:
			std::cout << "KILL";
			break;

		case 201:
			std::cout << "LSET";
			break;

		case 202:
			std::cout << "RSET";
			break;

		case 203:
			std::cout << "SAVE";
			break;

		case 204:
			std::cout << "RESET";
			break;

		// 205 is bad code (F)

		case 206:
			std::cout << "TO";
			break;

		case 207:
			std::cout << "THEN";
			break;

		case 208:
			std::cout << "TAB(";
			break;

		case 209:
			std::cout << "STEP";
			break;

		case 210:
			std::cout << "USR";
			break;

		case 211:
			std::cout << "FN";
			break;

		case 212:
			std::cout << "SPC(";
			break;

		case 213:
			std::cout << "NOT";
			break;

		case 214:
			std::cout << "ERL";
			break;

		case 215:
			std::cout << "ERR";
			break;

		case 216:
			std::cout << "STRING$";
			break;

		case 217:
			std::cout << "USING";
			break;

		case 218:
			std::cout << "INSTR";
			break;

		case 219:	/*! \bug Really should eat previous 2 tokens (: REM) */
			std::cout << "'";
			break;

		case 220:
			std::cout << "VARPTR";
			break;
			
		case 221:
			std::cout << "INKEY$";
			break;

		// 222 is bad code (o)
		// 223 is very bad code

		case 239:
			std::cout << ">";
			break;

		case 240:
			std::cout << "=";
			break;

		case 241:
			std::cout << "<";
			break;

		case 242:
			std::cout << "+";
			break;

		case 243:
			std::cout << "-";
			break;

		case 244:
			std::cout << "*";
			break;

		case 245:
			std::cout << "/";
			break;

		case 246:
			std::cout << "^";
			break;

		case 247:
			std::cout << "AND";
			break;

		case 248:
			std::cout << "OR";
			break;

		case 249:
			std::cout << "XOR";
			break;

		case 250:
			std::cout << "EQV";
			break;

		case 251:
			std::cout << "IMP";
			break;

		case 252:
			std::cout << "MOD";
			break;

		case 253:
			std::cout << "\\";
			break;

		case 255:
			source.read(&ch, 1);
			if (debug)
			{
				std::cout << "<" << ((int)ch & 0xff) << ">";
			}

			switch(((int)ch) & 0xff)
			{
			case 129:
				std::cout << "LEFT$";
				break;

			case 130:
				std::cout << "RIGHT$";
				break;

			case 131:
				std::cout << "MID$";
				break;

			case 132:
				std::cout << "SGN";
				break;

			case 133:
				std::cout << "INT";
				break;

			case 134:
				std::cout << "ABS";
				break;

			case 135:
				std::cout << "SQR";
				break;

			case 136:
				std::cout << "RND";
				break;

			case 137:
				std::cout << "SIN";
				break;

			case 138:
				std::cout << "LOG";
				break;

			case 139:
				std::cout << "EXP";
				break;

			case 140:
				std::cout << "COS";
				break;

			case 141:
				std::cout << "TAN";
				break;

			case 142:
				std::cout << "ATN";
				break;

			case 143:
				std::cout << "FRE";
				break;

			case 144:
				std::cout << "INP";
				break;

			case 145:
				std::cout << "POS";
				break;

			case 146:
				std::cout << "LEN";
				break;

			case 147:
				std::cout << "STR$";
				break;

			case 148:
				std::cout << "VAL";
				break;

			case 149:
				std::cout << "ASC";
				break;

			case 150:
				std::cout << "CHR$";
				break;

			case 151:
				std::cout << "PEEK";
				break;

			case 152:
				std::cout << "SPACE$";
				break;

			case 153:
				std::cout << "OCT$";
				break;

			case 154:
				std::cout << "HEX$";
				break;

			case 155:
				std::cout << "LPOS";
				break;

			case 156:
				std::cout << "CINT";
				break;

			case 157:
				std::cout << "CSNG";
				break;

			case 158:
				std::cout << "CDBL";
				break;

			case 159:
				std::cout << "FIX";
				break;

			// 160 is bad code

			case 171:
				std::cout << "CVI";
				break;

			case 172:
				std::cout << "CVS";
				break;

			case 173:
				std::cout << "CVD";
				break;

			case 175:
				std::cout << "EOF";
				break;

			case 176:
				std::cout << "LOC";
				break;

			case 177:
				std::cout << "LOF";
				break;

			case 178:
				std::cout << "MKI$";
				break;

			case 179:
				std::cout << "MKS$";
				break;

			case 180:
				std::cout << "MKD$";
				break;

			default:
				std::cout << " #" <<
					((unsigned int)ch & 0xFF) << "# ";
				break;
			}
			break;

		default:
			if (ch >= 32 && ch <= 126)
			{
				std::cout << ch;
			}
			else
			{
				std::cout << " *" <<
					((unsigned int)ch & 0xFF) << "* ";
			}
			break;
		}

		//
		// Give up if no more data in file
		//
		if (source.eof())
		{
			keeplooping = false;
		}
	}
}

//
// Usage statement
//
void help_message()
{
	std::cerr << "unmbasic -d -h <file>" << std::endl <<
		"  -d = print debug information" << std::endl <<
		"  -h = display this help message" << std::endl;
}

// From: http://community.borland.com/article/0,1410,16431,00.html
// Modified to lose the leading underscore.
//
//TI1431C.txt   Converting between Microsoft Binary and IEEE forma
//Category   :General
//Platform    :All
//Product    :C/C++  All
//
//Description:
//PRODUCT  :  Borland C++                           NUMBER  :  1400
//VERSION  :  All
//     OS  :  All
//   DATE  :  March 10, 1994                           PAGE  :  1/1
//  TITLE  :  Converting between Microsoft Binary and IEEE formats
//
// The following are implementations of Microsoft RTL functions
// not include in the Borland RTL.
//
// Functions:
//     _fmsbintoieee()
//     _fieeetomsbin()
//     _dmsbintoieee()
//     _dieeetomsbin()
//
// These functions convert back and forth from Microsoft Binary
// Format to IEEE floating point format.
//
// As with the Microsoft RTL functions,
//
// The argument srcX points to the value to be converted and the
// result is stored at the location given at destX.
//
// These routines do not handle IEE NAN's and infinities.  IEEE
// denormals are treated as 0's.
//
// Return:
//
// These functions return 0 if the conversion is successful and 1
// if the conversion causes an overflow.
//
//
//
// Examples of the use of these functions can be found on-line as
// MSBIN.ZIP.
//
//--------------------------------------------------------------------
int fmsbintoieee(float *src4, float *dest4)
   {
   unsigned char *msbin = (unsigned char *)src4;
   unsigned char *ieee = (unsigned char *)dest4;
   unsigned char sign = 0x00;
   unsigned char ieee_exp = 0x00;
   int i;
   /* MS Binary Format                         */
   /* byte order =>    m3 | m2 | m1 | exponent */
   /* m1 is most significant byte => sbbb|bbbb */
   /* m3 is the least significant byte         */
   /*      m = mantissa byte                   */
   /*      s = sign bit                        */
   /*      b = bit                             */
   sign = msbin[2] & 0x80;      /* 1000|0000b  */
   /* IEEE Single Precision Float Format       */
   /*    m3        m2        m1     exponent   */
   /* mmmm|mmmm mmmm|mmmm emmm|mmmm seee|eeee  */
   /*          s = sign bit                    */
   /*          e = exponent bit                */
   /*          m = mantissa bit                */
   for (i=0; i<4; i++) ieee[i] = 0;
   /* any msbin w/ exponent of zero = zero */
   if (msbin[3] == 0) return 0;
   ieee[3] |= sign;
   /* MBF is bias 128 and IEEE is bias 127. ALSO, MBF places   */
   /* the decimal point before the assumed bit, while          */
   /* IEEE places the decimal point after the assumed bit.     */
   ieee_exp = msbin[3] - 2;    /* actually, msbin[3]-1-128+127 */
   /* the first 7 bits of the exponent in ieee[3] */
   ieee[3] |= ieee_exp >> 1;
   /* the one remaining bit in first bin of ieee[2] */
   ieee[2] |= ieee_exp << 7;
   /* 0111|1111b : mask out the msbin sign bit */
   ieee[2] |= msbin[2] & 0x7f;
   ieee[1] = msbin[1];
   ieee[0] = msbin[0];
   return 0;
   }
int fieeetomsbin(float *src4, float *dest4)
   {
   unsigned char *ieee = (unsigned char *)src4;
   unsigned char *msbin = (unsigned char *)dest4;
   unsigned char sign = 0x00;
   unsigned char msbin_exp = 0x00;
   int i;
   /* See _fmsbintoieee() for details of formats   */
   sign = ieee[3] & 0x80;
   msbin_exp |= ieee[3] << 1;
   msbin_exp |= ieee[2] >> 7;
   /* An ieee exponent of 0xfe overflows in MBF    */
   if (msbin_exp == 0xfe) return 1;
   msbin_exp += 2;     /* actually, -127 + 128 + 1 */
   for (i=0; i<4; i++) msbin[i] = 0;
   msbin[3] = msbin_exp;
   msbin[2] |= sign;
   msbin[2] |= ieee[2] & 0x7f;
   msbin[1] = ieee[1];
   msbin[0] = ieee[0];
   return 0;
   }
int dmsbintoieee(double *src8, double *dest8)
   {
   unsigned char msbin[8];
   unsigned char *ieee = (unsigned char *)dest8;
   unsigned char sign = 0x00;
   unsigned int ieee_exp = 0x0000;
   int i;
   /* A manipulatable copy of the msbin number     */
   strncpy((char *)msbin,(char *)src8,8);
 /* MS Binary Format                                           */
 /* byte order =>  m7 | m6 | m5 | m4 | m3 | m2 | m1 | exponent */
 /* m1 is most significant byte => smmm|mmmm                   */
 /* m7 is the least significant byte                           */
 /*      m = mantissa byte                                     */
 /*      s = sign bit                                          */
 /*      b = bit                                               */
   sign = msbin[6] & 0x80;      /* 1000|0000b  */
 /* IEEE Single Precision Float Format                         */
 /*  byte 8    byte 7    byte 6    byte 5    byte 4  and so on */
 /* seee|eeee eeee|mmmm mmmm|mmmm mmmm|mmmm mmmm|mmmm ...      */
 /*          s = sign bit                                      */
 /*          e = exponent bit                                  */
 /*          m = mantissa bit                                  */
   for (i=0; i<8; i++) ieee[i] = 0;
   /* any msbin w/ exponent of zero = zero */
   if (msbin[7] == 0) return 0;
   ieee[7] |= sign;
   /* MBF is bias 128 and IEEE is bias 1023. ALSO, MBF places  */
   /* the decimal point before the assumed bit, while          */
   /* IEEE places the decimal point after the assumed bit.     */
   ieee_exp = msbin[7] - 128 - 1 + 1023;
   /* First 4 bits of the msbin exponent   */
   /* go into the last 4 bits of ieee[7]   */
   ieee[7] |= ieee_exp >> 4;
   /* The last 4 bits of msbin exponent    */
   /* go into the first 4 bits of ieee[6]  */
   ieee[6] |= ieee_exp << 4;
   /* The msbin mantissa must be shifted to the right 1 bit.   */
   /* Remember that the msbin number has its bytes reversed.   */
   for (i=6; i>0; i--)
       {
       msbin[i] <<= 1;
       msbin[i] |= msbin[i-1] >> 7;
       }
   msbin[0] <<= 1;
   /* Now the mantissa is put into the ieee array starting in  */
   /* the middle of the second to last byte.                   */
   for (i=6; i>0; i--)
       {
       ieee[i] |= msbin[i] >> 4;
       ieee[i-1] |= msbin[i] << 4;
       }
   ieee[0] |= msbin[0] >> 4;
 /* IEEE has a half byte less for its mantissa.  If the msbin */
 /* number has anything in this last half byte, then there is */
 /* an overflow.                                              */
   if (msbin[0] & 0x0f)
       return 1;
   else
       return 0;
   }
int dieeetomsbin(double *src8, double *dest8)
   {
   unsigned char ieee[8];
   unsigned char *msbin = (unsigned char *)dest8;
   unsigned char sign = 0x00;
   unsigned char any_on = 0x00;
   unsigned int msbin_exp = 0x0000;
   int i;
   /* Make a clobberable copy of the source number */
   strncpy((char *)ieee,(char *)src8,8);
   for (i=0; i<8; i++) msbin[i] = 0;
   /* If all are zero in src8, the msbin should be zero */
   for (i=0; i<8; i++) any_on |= ieee[i];
   if (!any_on) return 0;
   sign = ieee[7] & 0x80;
   msbin[6] |= sign;
   msbin_exp = (unsigned)(ieee[7] & 0x7f) * 0x10;
   msbin_exp += ieee[6] >> 4;
   if (msbin_exp-0x3ff > 0x80) return 1;
   msbin[7] = msbin_exp - 0x3ff + 0x80 + 1;
   /* The ieee mantissa must be shifted up 3 bits */
   ieee[6] &= 0x0f; /* mask out the exponent in the second byte
   */
   for (i=6; i>0; i--)
       {
       msbin[i] |= ieee[i] << 3;
       msbin[i] |= ieee[i-1] >> 5;
       }
   msbin[0] |= ieee[0] << 3;
   return 0;
   }
