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

//
// Prototypes
//
void *memcpyuc(char *dest, const char*source, int count);

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
// Command lineprocessing
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
		write_file(fd, portname, label);
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

//! \brief Write vol1 header to tape
//
// ANSI Tape format
//
//	[bot] [vol1] [eof] 
//	{ [hdr1] [hdr2] [eof] { [data] } [eof] [eof1][eof2] [rof]}
//	[eof] [eof]
//

//
// [vol]
//	1-3	Label Identifier		"VOL"
//	4	Label Number			"1"
//	5-10	Volume identifier		1-6 alphanumeric
//	11	Accessibility			" " (no restrictions)
//	12-37	Reserved
//	38-51	Owner Identifier		Used for volumeprotection
//		D%%B4431JJJGGG			[jjj,ggg] = [proj,prog]
//						D%% = Digital
//						N = pdp11
//						4431 = prot code (unused)
//	52-79	Reserved
//	80	Label Standard Version		"1"
//
int write_vol1(int fd, const char *label)
{
	const char* vol1_default = "VOL1                                 D%B4431001002                             3";
	char buffer[80];	/* Working buffer */

	//
	// Fill in values
	//
	memcpy(buffer, vol1_default, 80);
	memcpyuc(buffer + 4, label, strlen(label));

	//
	// Write block
	//
	write_tpcblock(fd, buffer, 80);
}

//
// [hdr1]
//
//	1-3	Label Identifier		"HDR"
//	4	Label Number			"1"
//	5-21	File Identifier			alphanumeric + special
//	22-27	File Identifier			volume ID from header
//	28-31	File Section Number		"0001"
//	32-35	File Sequence Number		"0001"
//	36-39	Generation Number		"00001"
//	40-41	Generation Version		"00"
//	42-47	Creation date			<space>YYDDD
//						" 000000" if none
//	48-53	Expiration Date			<space>YYDDD
//						" 00000: if none
//	54	Accessibility			" "
//	55-60	BlockCount			"000000"
//	61-73	System Code			"DECRSTS/E"
//	74-80	Reserved
//
int write_hdr1(int fd, const char* filename, const char *label)
{
	const char* hdr1_default = "HDR1                       00010001000100 00000 00000 000000DECRSTS/E           ";
	char buffer[80];	/* Working buffer */

	//
	// Fill in values
	//
	memcpy(buffer, hdr1_default, 80);
	memcpyuc(buffer + 4, filename, strlen(filename));
	memcpyuc(buffer + 21, label, strlen(label));

	//
	// Write block
	//
	write_tpcblock(fd, buffer, 80);
}



//
// [hdr2]
//
//	1-3	Label Identifier		"HDR"
//	4	Label Number			"2"
//	5	Record Format			"U" is default
//						"F" = fixed
//						"D"=variable
//						"S"= spanned
//						"U"= undefined
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
int write_hdr2(int fd, const char* filename)
{
	const char* hdr2_default = "HDR2F0051200512                     M             00                            ";
	char buffer[80];	/* Working buffer */

	//
	// Fill in values
	//
	memcpy(buffer, hdr2_default, 80);

	//
	// Write block
	//
	write_tpcblock(fd, buffer, 80);
}



//
// [eof1] (same format as [hdr1]
//
//	1-3	Label Identifier		"EOF"
//	4	Label Number			"1"
//	5-21	File Identifier			alphanumeric + special
//	22-27	File Identifier			volume ID from header
//	28-31	File Section Number		"0001"
//	32-35	File Sequence Number		"0001"
//	36-39	Generation Number		"00001"
//	40-41	Generation Version		"00"
//	42-47	Creation date			<space>YYDDD
//						" 000000" if none
//	48-53	Expiration Date			<space>YYDDD
//						" 00000: if none
//	54	Accessibility			" "
//	55-60	BlockCount			"000000"
//	61-73	System Code			"DECRSTS/E"
//	74-80	Reserved
//
int write_eof1(int fd, const char* filename, int blockcount, const char *label)
{
	const char* eof1_default = "EOF1                       00010001000100 00000 00000 000000DECRSTS/E           ";
	char buffer[80];	/* Working buffer */
	char strbuf[10];

	//
	// Fill in values
	//
	memcpy(buffer, eof1_default, 80);
	memcpyuc(buffer + 4, filename, strlen(filename));
	memcpyuc(buffer + 21, label, strlen(label));
	sprintf(strbuf, "%06d", blockcount);
	memcpy(buffer + 54, strbuf, 6);

	//
	// Write block
	//
	write_tpcblock(fd, buffer, 80);
}



//
// [eof2] same format as hdr2
//
//	1-3	Label Identifier		"EOF"
//	4	Label Number			"2"
//	5	Record Format			"U" is default
//						"F" = fixed
//						"D"=variable
//						"S"= spanned
//						"U"= undefined
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
int write_eof2(int fd, const char* filename)
{
	const char* eof2_default = "EOF2F0051200512                     M             00                            ";
	char buffer[80];	/* Working buffer */

	//
	// Fill in values
	//
	memcpy(buffer, eof2_default, 80);

	//
	// Write block
	//
	write_tpcblock(fd, buffer, 80);
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
// Outputs one block of code to the tpc tape image file
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

int write_file(int fd, const char* filename,const char *label)
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
	write_hdr1(fd, filename, label);
	write_hdr2(fd, filename);
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
	write_eof1(fd, filename, blockcount, label);
	write_eof2(fd, filename);
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
