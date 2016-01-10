/*
 * RawRead.c - Read a Rainbow floppy disk into a data file.
 *
 * Kevin Handy (kth@srv.net)
 * Software Solutions, Inc.
 * Idaho Falls, Idaho
 *
 * This program is designed to read a raw RX50 floppy disk into
 * a data file containing an image of the disk.  It must be used
 * along with RAINDISK, which will map an IBM-PC format floppy
 * into a Rainbow RX50 format.  This program is hardcoded with
 * the Rainbow parameters, and must be modified to handle any
 * other drive format.
 *
 * This program was originally based upon rawrute.c, which comes
 * with many Linux distributions, but has been extensively
 * rewritten to go in the opposite direction.
 *
 * This program is provided as-is, no warrenty.
 * Please send me any enhancements made to the program so that
 * I may merge them all together.
 */
#include <alloc.h>
#include <bios.h>
#include <ctype.h>
#include <dir.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define FALSE	0
#define TRUE	(!FALSE)

#define SECTORSIZE	512

#define	RESET	0
#define	LAST	1
#define	READ	2
#define	WRITE	3
#define	VERIFY	4
#define	FORMAT	5

int	done;

/*
 Catch ^C and ^Break.
*/
int	handler(void)
{
  done = TRUE;
  return(0);
}
void msg(char (*s))
{
	fprintf(stderr, "%s\n", s);
	_exit(1);
}
/*
 Identify the error code with a real error message.
*/
void Error(int (status))
{
  switch (status) {
    case 0x00:	msg("Operation Successful");				break;
	 case 0x01:	msg("Bad command");					break;
    case 0x02:	msg("Address mark not found");				break;
    case 0x03:	msg("Attempt to write on write-protected disk");	break;
	 case 0x04:	msg("Sector not found");				break;
    case 0x05:	msg("Reset failed (hard disk)");			break;
    case 0x06:	msg("Disk changed since last operation");		break;
	 case 0x07:	msg("Drive parameter activity failed");			break;
    case 0x08:	msg("DMA overrun");					break;
    case 0x09:	msg("Attempt to DMA across 64K boundary");		break;
    case 0x0A:	msg("Bad sector detected");				break;
    case 0x0B:	msg("Bad track detected");				break;
    case 0x0C:	msg("Unsupported track");				break;
    case 0x10:	msg("Bad CRC/ECC on disk read");			break;
    case 0x11:	msg("CRC/ECC corrected data error");			break;
    case 0x20:	msg("Controller has failed");				break;
    case 0x40:	msg("Seek operation failed");				break;
    case 0x80:	msg("Attachment failed to respond");			break;
    case 0xAA:	msg("Drive not ready (hard disk only");			break;
    case 0xBB:	msg("Undefined error occurred (hard disk only)");	break;
	 case 0xCC:	msg("Write fault occurred");				break;
    case 0xE0:	msg("Status error");					break;
	 case 0xFF:	msg("Sense operation failed");				break;
  }
printf("\n");
  //  _exit(1);
}

void main(void)
{
  char	 fname[MAXPATH];
  char	*buffer, *pbuf;
  int	 fdin, drive, status, spt, buflength;
  long ns;
  char buffer1[SECTORSIZE * 2];

  puts("RaWread 1.0 - Read raw floppy diskette to disk file\n");
  ctrlbrk(handler);
  printf("Enter source file name: ");
  scanf("%s", fname);
  _fmode = O_BINARY;
  if ((fdin = open(fname, O_WRONLY | O_CREAT)) <= 0) {
	  perror(fname);
	  exit(1);
  }

  printf("Enter source drive: ");
  scanf("%s", fname);
  drive = fname[0];
  drive = (islower(drive) ? toupper(drive) : drive) - 'A';
  printf("Please insert diskette into ");
  printf("drive %c: and press -ENTER- :", drive + 'A');
  while (bioskey(1) == 0) ;				/* Wait...	*/
  if ((bioskey(0) & 0x7F) == 3) exit(1);		/* Check for ^C	*/
  putchar('\n');
  done = FALSE;
/*
 * Determine number of sectors per track and allocate buffers.
 */
//  spt = nsects(drive);
  spt = 20;
  buflength = spt * SECTORSIZE;
  buffer = (char *)malloc(buflength);
  printf("Number of sectors per track for this disk is %d\n", spt);
  printf("Writing image to drive %c:.  Press ^C to abort.\n", drive+'A');

  /*
 * Start reading data to diskette until there is no more data to read.
 */
	pbuf = buffer;
	for (ns = 0; ns < 800; ns++)			// Hard coded 800 sectors
	{
		printf("Reading sector %ld ", ns);
		status = absread(drive, 1, ns, pbuf);
		if (status != 0)
		{
			printf("Error %d\n", errno);
		}
		else
		{
			printf("Ok\n");
		}
		pbuf += SECTORSIZE;

		if ((ns % 20) == 19)
		{
			biosdisk(READ, 1, 0, 0, 1, 1, &buffer1);	// Force a drive B reset
																	// because RAINDISK messes up
																	// and forgets to switch back
			write(fdin, buffer, buflength);
			pbuf = buffer;
		}
	}
	biosdisk(READ, 1, 0, 0, 1, 1, &buffer1);	// Force a drive B reset
															// because RAINDISK messes up
															// and forgets to switch back
	close(fdin);


}	/* end main */
