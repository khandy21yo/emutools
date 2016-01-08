// \file ansitpc.c
//
// \brief Write files into ANSI formatted TPC (tape) file.
//
// Writes a series of files into a tape container file
// for use with emulators, such as simh.
//
// \author Kevin Handy, Jan 2016
//
// \Note The ANSI format information comes from the RSTS/EProgramming
// manual, Appendix A.
// DEC-11-ORPMA-B-D_RSTS_E_Programming_Manual_V06B-02_Nov76.pdf
//
// \Note Ref. American National Standard, Magnetic Tape Labels and
// File Structure for Information Interchange, ANSI X3.27-1978
//
// Compile via
//
//	cc -o ansitpc ansitpc.c -lpopt
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <popt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>		/* XPG4.2 basename() */

//
// Prototypes
//
void *memcpyuc(char *dest, const char*source, int count);

//
// Global Data
//
//                                 123456789A123456789B123456789C123456789D123456789E123456789F123456789G123456789H
static const char* vol1_default = "VOL1                                 D%%B4431200200                            3";
static const char* hdr1_default = "HDR1                       00010001000100 00000 00000 000000DECRSTS/E           ";
static const char* hdr2_default = "HDR2F0051200512                     M             00                            ";
static char vol1[81];	/* Current VOL1 label */
static char hdr1[81];	/* Current HDR1 label */
static char hdr2[81];	/* Current HDR2 label */

//! \brief Usage message
//
void usage(poptContext optCon, int exitcode, char *error, char *addl)
{
	poptPrintUsage(optCon, stderr, 0);
	if (error)
	{
		fprintf(stderr, "%s: %s\n", error, addl);
	}
	exit(exitcode);
}

//! \brief Main Function
//
// Command line processing
//
int main(int argc, const char *argv[])
{
	int     c;		/* used for argument parsing */

	int blocksize = 512;		/* Size of each block */
	const char *output = "ansi.tpc";	/* Output file name */
	const char *label = "ANSI";	/* Tape label */

	const char *portname;
	poptContext optCon;	/* context for parsing command-line options */
	int fd;			/* tpc output channel */
	int seqNo = 0;

	struct poptOption optionsTable[] =
	{
		{ "blocksize", 'b', POPT_ARG_INT, &blocksize, 0,
			"block size", "BYTES" },
		{ "output", 'o', POPT_ARG_STRING, &output, 0,
			"output tpc file name", NULL },
		{ "label", 'l', POPT_ARG_STRING, &label, 0,
			"Tape Label", NULL },
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};

	optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
	poptSetOtherOptionHelp(optCon, "[OPTIONS]* <files>");
	if (argc < 2)
	{
		poptPrintUsage(optCon, stderr, 0);
		exit(1);
	}

	/* Now do options processing, get portname */
	while ((c = poptGetNextOpt(optCon)) >= 0)
	{
/* placeholder code */
		switch (c) {
		case 'c':
			break;
		}
	}

	if (c < -1)
	{
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		poptStrerror(c));
		return 1;
	}

	/*
	 * Create tpc output stream
	 */
	fd = creat(output, S_IRUSR | S_IWUSR);
	if (fd <= 0)
	{
		perror("Unable to open output file");
		exit(EXIT_FAILURE);
	}

	//
	// BOT Beginning Of Tape
	//
	write_vol1(fd, label);

	/* Print out options, portname chosen */
	printf("Options  chosen: ");
	if(blocksize) printf("-b %d ", blocksize);
	printf("Output to %s\n", output);

	//
	// Write all files
	//
	while (portname = poptGetArg(optCon))
	{
		printf("Processing %s\n", portname);
		seqNo++;
		write_file(fd, portname, seqNo);
	}

	//
	// EOT End Of Tape
	//
	write_tpcmark(fd);
	write_tpcmark(fd);

	close(fd);

	poptFreeContext(optCon);

	exit(0);
}

//! \brief Write ANSI VOL1 header to tape
//
// ANSI Tape Format
//
//	BOT [VOL1]
//	{ [HDR1] [HDR2] TM { [data] } TM [EOF1] [EOF2] TM }
//	TM
//

//
// [VOL1]
//	1-3	Label Identifier		"VOL"
//	4	Label Number			"1"
//	5-10	Volume identifier		1-6 alphanumeric
//	11	Accessibility			" " (no restrictions)
//	12-37	Reserved
//	38-51	Owner Identifier		Used for volumeprotection
//		D%%B4431JJJGGG			[jjj,ggg] = [proj,prog]
//						D%% = Digital
//						N = PDP-11
//						4431 = prot code (unused)
//	52-79	Reserved
//	80	Label-Standard Version		"3"
//
int write_vol1(int fd, const char *label)
{
	int  n;

	//
	// Fill in values
	//
	memcpy(vol1, vol1_default, 80);
	n = strlen(label);
	memcpyuc(vol1 + 4, label, n > 6 ? 6 : n);

	//
	// Write block
	//
	write_tpcblock(fd, vol1, 80);
}

//
// [HDR1]
//
//	1-3	Label Identifier		"HDR"
//	4	Label Number			"1"
//	5-21	File Identifier			alphanumeric + special
//	22-27	File Identifier			volume ID from header
//	28-31	File Section Number		"0001" <no segmented files>
//	32-35	File Sequence Number		nnnn numeric seq. no., from 1
//	36-39	Generation Number		"00001"
//	40-41	Generation Version		"00"
//	42-47	Creation date			<space>YYDDD
//						" 00000" if none
//	48-53	Expiration Date			<space>YYDDD
//						" 00000" if none
//	54	Accessibility			" "
//	55-60	BlockCount			"000000"
//	61-73	System Code			"DECRSTS/E"
//	74-80	Reserved                        "       "
//
int write_hdr1(int fd, const char* filename, int seqNo)
{
	char fileId[18];	/* Working buffer */
	char fileSeqNo[5];	/* Working buffer */
	char* baseName;
	int  n;

	//
	// Fill in values
	//
	memcpy(hdr1, hdr1_default, 80);
	n = strlen(filename);
	n = n > 17 ? 17 : n;
	memcpyuc(fileId, filename, n);
	fileId[n] = '\0';
	baseName = basename(fileId);
	memcpy(hdr1 + 4, baseName, strlen(baseName));
	memcpy(hdr1 + 21, vol1 + 4, 6);
	seqNo = seqNo > 9999 ? 9999 : seqNo;
	sprintf(fileSeqNo, "%04d", seqNo);
	memcpy(hdr1 + 31, fileSeqNo, 4);

	//
	// Write block
	//
	write_tpcblock(fd, hdr1, 80);
}

//
// [HDR2]
//
//	1-3	Label Identifier		"HDR"
//	4	Label Number			"2"
//	5	Record Format			"U" is default
//						"F" = fixed
//						"D" = variable
//						"S" = spanned
//						"U" = undefined
//	6-10	Block Length			512 is default
//						(fiesize)
//	11-15	Record Length			(clustersize)
//	16-50	System dependent		16-36 = spaces
//						37="A" record contains Fortran
//						  control characters
//						37=space,lf precedes and cr
//						  follows each record
//						37="M" record contains all
//						  formcontrol characters.
//						  (default)
//	51-52	Buffer Offset			"00"
//	53-80	Reserved
//
int write_hdr2(int fd)
{

	//
	// Fill in values
	//
	memcpy(hdr2, hdr2_default, 80);

	//
	// Write block
	//
	write_tpcblock(fd, hdr2, 80);
}

//
// [EOF1] same format as [HDR1], except
//
//	55-60	BlockCount			No. of data blocks
//
int write_eof1(int fd, int blockcount)
{
	char blockCount[7];

	//
	// Fill in values
	//
	memcpy(hdr1, "EOF", 3);
	blockcount = blockcount > 999999 ? 999999 : blockcount;
	sprintf(blockCount, "%06d", blockcount);
	memcpy(hdr1 + 54, blockCount, 6);

	//
	// Write block
	//
	write_tpcblock(fd, hdr1, 80);
}



//
// [EOF2] same format as [HDR2]
//
int write_eof2(int fd)
{

	//
	// Fill in values
	//
	memcpy(hdr2, "EOF", 3);

	//
	// Write block
	//
	write_tpcblock(fd, hdr2, 80);
}

//! Write block of data to tpc file
//
// Outputs one block of code to the tpc tape image file
//
int write_tpcblock(int fd, const char *buffer, unsigned int size)
{
	unsigned char blocksize[4];

	//
	// Block size
	//
	blocksize[0] = size & 0xff;
	blocksize[1] = (size >> 8) & 0xff;
	blocksize[2] = (size >> 16) & 0xff;
	blocksize[3] = (size >> 24) & 0xff;
	write(fd, blocksize, 4);

	//
	// Data
	//
	write(fd, buffer, size);

	//
	// Block size again
	//
	write(fd, blocksize, 4);
}

//! Write tape mark to tpc file
//
// Outputs a tape mark to the tpc tape image file
//
int write_tpcmark(int fd)
{
	unsigned char blocksize[4];

	//
	// Block size
	//
	blocksize[0] = 0;
	blocksize[1] = 0;
	blocksize[2] = 0;
	blocksize[3] = 0;
	write(fd, blocksize, 4);
}

int write_file(int fd, const char* filename, int seqNo)
{
	int fif;
	const int recordsize = 512;
	char ifbuffer[recordsize];
	int readlength;
	int blockcount = 0;

	fif = open(filename, O_RDONLY);
	if (fif <= 0)
	{
		perror("Unable to open input file");
		return EXIT_FAILURE;
	}

	//
	// Headers
	//
	write_hdr1(fd, filename, seqNo);
	write_hdr2(fd);
	write_tpcmark(fd);

	//
	// File data
	//
	memset(ifbuffer, 0, recordsize);
	while(readlength = read(fif, ifbuffer, recordsize))
	{
//		write_tpcblock(fd, ifbuffer, readlength);
		write_tpcblock(fd, ifbuffer, recordsize);
		blockcount++;
		memset(ifbuffer, 0, recordsize);
	}
	write_tpcmark(fd);

	//
	// eof marks
	//
	write_eof1(fd, blockcount);
	write_eof2(fd);
	write_tpcmark(fd);

	close(fif);
}

//! memcpy that upcases
//
// The return value is bogus.
//
void *memcpyuc(char *dest, const char*source, int count)
{
	while(count--)
	{
		*dest++ = toupper(*source++);
	}
	return dest;
}
