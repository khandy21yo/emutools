#define UNIX
#define WORD
#undef DEBUG
#undef  DUMP
/*
*** Rulers in headers apply to body and visa versa? bb009/7
*** Default ruler wrong?  Many in gb0002
*** Left margin needs to be set for footers if not set
********* Ruler stuff not handled: H on decmate, L in file, hyphenation zone,
 > on decmate, B in file, right justified tab.  Probably others
**********

 * dmprocess.c - Program to convert Decmate II DecWord files to WordPerfedt
 *
 * Kevin Handy (kth@srv.net)
 * Software Solutions, Inc.
 * Idaho Falls, Idaho  83401
 *
 * This software may be used to convert a Decmate II(tm) DecWord(tm)
 * disk image file into several WordPerfect(tm) documents. This
 * program was developed by examining disk images, and hacking
 * what I figured out into a conversion, so the program may be missing
 * many features.
 *
 * This software is provided as-is, with no warrenty.
 * Please submit any enhancements made to this software to me so that
 * I may merge all changes together into one single version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef UNIX
#define FAR static far
#include <dos.h>
#include <bios.h>
#include <sys\stat.h>
#include <math.h>
#define strncasecmp(a,b,c) strnicmp(a,b,c)
#else
#define FAR
#include <unistd.h>
#endif
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

#define ARRAYSIZE(x)   (sizeof(x) / sizeof(x[0]))
/*
 * Floppy or data file
 */
#ifndef DISKFILE
#define DISKFILE 2      /* 1 - Disk, 2 - File */
#endif

   /* If set disk is a rx01 disk in 8 bit mode dump format */
int rx01_disk = 0;
int filename_file = 0;


#ifdef DEBUG
#define DBPC(x) { filename_check(x); printf("%c",x);}
#define DBPS(x) printf("%s",x)
#else
#define DBPC(x) filename_check(x)
#define DBPS(x)
#endif
/*
 * Special definitions
 */
#define BOLD      1
#define UNDERLINE 2
#define SUBSCRIPT 4
#define SUPERSCRIPT  8
#define ITALICS   16

/*
 * Prototypes
 */
void set_default_ruler();
int read_sector(unsigned int secnum);
int read_index(void);
int read_document(int docnum);
int read_document_index(int docnum);
int read_text(void);
int read_next(void);
void wp_outattr(int ch);
void wp_setflags(int new, int ch);
void wp_setflag(int fl);
void wp_resetflag(int fl);
void dumpbuffer(void);
void addbuffer(const unsigned char *text, int len);
void altaddbuffer(const unsigned char *text, int len);
void altdeletebuffer(int start, int count);
void insertbuffer(const unsigned char *text, int len, int offset);
int popbuffer(void);
void pushbuffer(int ch,int count);
void deletebuffer(int start, int count);
int strncasestr(const char *source, const char *match, int count);
int printable(void);
int overstrike(char first, char second);
void wp_setruler(void);
void dump_buffer(void);
void dump_pgbuff(int insert);
void pcehandler(void);
void change_font(double font_cpi);
void filename_check(int ch);
int search_ahead(void);


/*
 * File open
 */
FILE *infile;
FILE *dirfile;
#ifdef DUMP
FILE *dumpfile;
#endif

double pagewidth_inch;
   /* character width for WP */
double font_toinch = 120;
double toinch = 1200;
double min_left_margin = .3;
double min_right_margin = .3;
double left_margin_shift;
#ifdef WORD
int make_abs_tabs = 0;
#else
int make_abs_tabs = 1;
#endif
/*
 * Sector storage
 */
unsigned int lowword;
unsigned int highword;
unsigned int buffer[270];
int drv;
unsigned char intrack[512];
int drvoff;

#define fgetcx(x) intrack[drvoff++]

/*
 * Master Index storage
 */
int index_count = 0;
// Was 80 for RX01
#define MAX_DOC 90
unsigned int indexv[MAX_DOC];
#define MAX_FILENAME 128
struct {
   int doc_num;
   int size;
   int version;
   int last_min;
   int last_hour;
   int total_min;
   int total_hour;
   int mod_day;
   int mod_month;
   int mod_year;
   int mod_hour;
   int mod_min;
   int create_day;
   int create_month;
   int create_year;
   char filename[MAX_FILENAME];
   char *comment;
} dir[MAX_DOC];
int dir_count = 0;
char disk_name[7];

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
#define TAB_NONE 0
#define TAB_LEFT 1
#define TAB_UKN 2
#define TAB_RIGHT 3
#define TAB_DEC 4
#define TAB_LEN 160
struct {
   int char_in_line;
   int ignore_count;
   double font_cpi,cur_cpi;
   int cur_flags, ch_flags;
   int cur_rn,cur_lm,cur_sp,cur_wm,cur_rm,cur_jf,cur_ts[TAB_LEN+1];
   int rn,lm,sp,wm,rm,jf,ts[TAB_LEN+1];
   int overstrikeflag;
   int last_tab;
   int in_right_tab;
} info[3];
int itype = 0;
#define INFO_NORM 0
#define INFO_HDR 1
#define INFO_FOOT 2

// Not currently used
int forceindent = 0;
/*
 * Buffering area
 */
int wpbuffcount = 0, altbufflag = 0, altwpbuffcount = 0;
int altfirstchar = -1;
int altlastret = -1;
FAR unsigned char wpbuffer[16384], altwpbuffer[16384];

int pgbuffcount = 0;
FAR unsigned char pgbuff[256000];

/*
 * String semi-constants
 */
#if 0
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
#else
const unsigned char minprefix[] = {
#if 1
#if defined(UNIX) & !defined(WORD)
  255, 87, 80, 67,220, 2, 0, 0, 1, 10, 0, 0, 0, 0, 0, 0,
  251,255, 5, 0, 50, 0,160, 2, 0, 0, 12, 0, 90, 0, 0, 0,
  66, 0, 0, 0, 3, 0, 1, 0, 0, 0,156, 0, 0, 0, 48, 0,
  3, 2, 0, 0,157, 0, 0, 0,255,255, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 88, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 88, 2,120, 0,254, 21, 54, 16, 88, 7, 0, 0,
  0, 4, 17, 64, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 64,130,171, 0, 88,
  2,120, 0,254, 21, 54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0,
  0, 0, 0, 1, 0, 1, 0, 88, 2, 64, 60, 0,166, 24,244, 16,
  4, 6, 0, 0, 1, 57, 16, 0,112, 96, 0, 43, 5, 0, 0, 20,
  28, 0, 67, 0,111, 0,117, 0,114, 0,105, 0,101, 0,114, 0,
  45, 0, 87, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 38, 2,110, 0,254, 21,
  54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0, 0, 1, 0, 1, 0,
  1, 0, 38, 2, 64, 60, 0,166, 24,244, 16, 4, 6, 0, 0, 1,
  57, 16, 0,112, 96, 0, 43, 5, 0, 0, 20, 28, 0, 67, 0,111,
  0,117, 0,114, 0,105, 0,101, 0,114, 0, 45, 0, 87, 0, 80,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 38, 2, 88, 2,120, 0,254, 21, 54, 16, 88, 7, 0,
  0, 0, 4, 17, 64, 0, 0, 2, 0, 1, 0, 1, 0, 88, 2, 64,
  60, 0,166, 24,244, 16, 4, 6, 0, 0, 1, 57, 16, 0,112, 96,
  0, 43, 5, 0, 0, 20, 28, 0, 67, 0,111, 0,117, 0,114, 0,
  105,  0,101, 0,114, 0, 45, 0, 87, 0, 80, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 2,
  251,255, 5, 0, 50, 0, 0, 0, 0, 0, 6, 0, 8, 0, 0, 0,
  210,  2, 0, 0, 8, 0, 2, 0, 0, 0,218, 2, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 8, 51,124, 0,120, 0, 0, 0, 0, 0
#else
    255, 87, 80, 67, 49, 2, 0, 0, 1, 10, 0, 0, 0, 0, 0, 0,
    251, 255, 5, 0, 50, 0, 245, 1, 0, 0, 12, 0, 90, 0, 0, 0,
    66, 0, 0, 0, 3, 0, 1, 0, 0, 0, 156, 0, 0, 0, 48, 0,
    88, 1, 0, 0, 157, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0,
    0, 0, 72, 80, 32, 76, 97, 115, 101, 114, 74, 101, 116, 32, 73, 73,
    73, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 88, 2, 78, 0, 244, 26, 92, 18, 26, 9, 0, 0,
    0, 16, 32, 80, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 80, 130, 171, 0, 88,
    2, 78, 0, 244, 26, 92, 18, 26, 9, 0, 0, 0, 16, 32, 80, 0,
    0, 0, 0, 1, 0, 1, 0, 88, 2, 80, 40, 0, 214, 30, 195, 15,
    57, 8, 0, 0, 17, 9, 0, 0, 0, 90, 0, 27, 1, 0, 139, 20,
    54, 0, 84, 0, 105, 0, 109, 0, 101, 0, 115, 0, 32, 0, 78, 0,
    101, 0, 119, 0, 32, 0, 82, 0, 111, 0, 109, 0, 97, 0, 110, 0,
    32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 88, 2, 120, 0, 254, 21,
    54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0, 0, 1, 0, 1, 0,
    1, 0, 88, 2, 64, 60, 0, 190, 28, 189, 14, 81, 10, 0, 0, 1,
    57, 0, 0, 0, 90, 0, 43, 5, 0, 139, 20, 46, 0, 67, 0, 111,
    0, 117, 0, 114, 0, 105, 0, 101, 0, 114, 0, 32, 0, 78, 0, 101,
    0, 119, 0, 32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97,
    0, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 88, 2, 251, 255, 5, 0, 50, 0, 0, 0, 0, 0, 6,
    0, 8, 0, 0, 0, 39, 2, 0, 0, 8, 0, 2, 0, 0, 0, 47,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 8, 51, 124, 0, 120, 0, 0, 0, 0,
    0
#endif
#else
    255, 87, 80, 67, 137, 7, 0, 0, 1, 10, 0, 0, 0, 0, 0, 0,
    251, 255, 5, 0, 50, 0, 77, 7, 0, 0, 12, 0, 90, 0, 0, 0,
    66, 0, 0, 0, 3, 0, 1, 0, 0, 0, 156, 0, 0, 0, 48, 0,
    176, 6, 0, 0, 157, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0,
    0, 0, 72, 80, 32, 76, 97, 115, 101, 114, 74, 101, 116, 32, 73, 73,
    73, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 88, 2, 78, 0, 244, 26, 92, 18, 26, 9, 0, 0,
    0, 16, 32, 80, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 80, 130, 171, 0, 88,
    2, 78, 0, 244, 26, 92, 18, 26, 9, 0, 0, 0, 16, 32, 80, 0,
    0, 0, 0, 1, 0, 1, 0, 88, 2, 80, 40, 0, 214, 30, 195, 15,
    57, 8, 0, 0, 17, 9, 0, 0, 0, 90, 0, 27, 1, 0, 139, 20,
    54, 0, 84, 0, 105, 0, 109, 0, 101, 0, 115, 0, 32, 0, 78, 0,
    101, 0, 119, 0, 32, 0, 82, 0, 111, 0, 109, 0, 97, 0, 110, 0,
    32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 88, 2, 88, 2, 120, 0, 254, 21,
    54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0, 0, 1, 0, 1, 0,
    1, 0, 88, 2, 64, 60, 0, 190, 28, 189, 14, 81, 10, 0, 0, 1,
    57, 0, 0, 0, 90, 0, 43, 5, 0, 139, 20, 46, 0, 67, 0, 111,
    0, 117, 0, 114, 0, 105, 0, 101, 0, 114, 0, 32, 0, 78, 0, 101,
    0, 119, 0, 32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97,
    0, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 88, 2, 38, 2, 110, 0, 254, 21, 54, 16, 88, 7, 0,
    0, 0, 4, 17, 64, 0, 0, 2, 0, 1, 0, 1, 0, 38, 2, 64,
    60, 0, 190, 28, 189, 14, 81, 10, 0, 0, 1, 57, 0, 0, 0, 90,
    0, 43, 5, 0, 139, 20, 46, 0, 67, 0, 111, 0, 117, 0, 114, 0,
    105, 0, 101, 0, 114, 0, 32, 0, 78, 0, 101, 0, 119, 0, 32, 0,
    82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 2,
    244, 1, 100, 0, 254, 21, 54, 16, 88, 7, 0, 0, 0, 4, 17, 64,
    0, 0, 3, 0, 1, 0, 1, 0, 244, 1, 64, 60, 0, 190, 28, 189,
    14, 81, 10, 0, 0, 1, 57, 0, 0, 0, 90, 0, 43, 5, 0, 139,
    20, 46, 0, 67, 0, 111, 0, 117, 0, 114, 0, 105, 0, 101, 0, 114,
    0, 32, 0, 78, 0, 101, 0, 119, 0, 32, 0, 82, 0, 101, 0, 103,
    0, 117, 0, 108, 0, 97, 0, 114, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 244, 1, 194, 1, 90, 0, 254,
    21, 54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0, 0, 4, 0, 1,
    0, 1, 0, 194, 1, 64, 60, 0, 190, 28, 189, 14, 81, 10, 0, 0,
    1, 57, 0, 0, 0, 90, 0, 43, 5, 0, 139, 20, 46, 0, 67, 0,
    111, 0, 117, 0, 114, 0, 105, 0, 101, 0, 114, 0, 32, 0, 78, 0,
    101, 0, 119, 0, 32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0,
    97, 0, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 194, 1, 144, 1, 80, 0, 254, 21, 54, 16, 88, 7,
    0, 0, 0, 4, 17, 64, 0, 0, 5, 0, 1, 0, 1, 0, 144, 1,
    64, 60, 0, 190, 28, 189, 14, 81, 10, 0, 0, 1, 57, 0, 0, 0,
    90, 0, 43, 5, 0, 139, 20, 46, 0, 67, 0, 111, 0, 117, 0, 114,
    0, 105, 0, 101, 0, 114, 0, 32, 0, 78, 0, 101, 0, 119, 0, 32,
    0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 144,
    1, 94, 1, 70, 0, 254, 21, 54, 16, 88, 7, 0, 0, 0, 4, 17,
    64, 0, 0, 6, 0, 1, 0, 1, 0, 94, 1, 64, 60, 0, 190, 28,
    189, 14, 81, 10, 0, 0, 1, 57, 0, 0, 0, 90, 0, 43, 5, 0,
    139, 20, 46, 0, 67, 0, 111, 0, 117, 0, 114, 0, 105, 0, 101, 0,
    114, 0, 32, 0, 78, 0, 101, 0, 119, 0, 32, 0, 82, 0, 101, 0,
    103, 0, 117, 0, 108, 0, 97, 0, 114, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 94, 1, 44, 1, 60, 0,
    254, 21, 54, 16, 88, 7, 0, 0, 0, 4, 17, 64, 0, 0, 7, 0,
    1, 0, 1, 0, 44, 1, 64, 60, 0, 190, 28, 189, 14, 81, 10, 0,
    0, 1, 57, 0, 0, 0, 90, 0, 43, 5, 0, 139, 20, 46, 0, 67,
    0, 111, 0, 117, 0, 114, 0, 105, 0, 101, 0, 114, 0, 32, 0, 78,
    0, 101, 0, 119, 0, 32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108,
    0, 97, 0, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 44, 1, 250, 0, 50, 0, 254, 21, 54, 16, 88,
    7, 0, 0, 0, 4, 17, 64, 0, 0, 8, 0, 1, 0, 1, 0, 250,
    0, 64, 60, 0, 190, 28, 189, 14, 81, 10, 0, 0, 1, 57, 0, 0,
    0, 90, 0, 43, 5, 0, 139, 20, 46, 0, 67, 0, 111, 0, 117, 0,
    114, 0, 105, 0, 101, 0, 114, 0, 32, 0, 78, 0, 101, 0, 119, 0,
    32, 0, 82, 0, 101, 0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    250, 0, 200, 0, 40, 0, 254, 21, 54, 16, 88, 7, 0, 0, 0, 4,
    17, 64, 0, 0, 9, 0, 1, 0, 1, 0, 200, 0, 64, 60, 0, 190,
    28, 189, 14, 81, 10, 0, 0, 1, 57, 0, 0, 0, 90, 0, 43, 5,
    0, 139, 20, 46, 0, 67, 0, 111, 0, 117, 0, 114, 0, 105, 0, 101,
    0, 114, 0, 32, 0, 78, 0, 101, 0, 119, 0, 32, 0, 82, 0, 101,
    0, 103, 0, 117, 0, 108, 0, 97, 0, 114, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 251, 255, 5,
    0, 50, 0, 0, 0, 0, 0, 6, 0, 8, 0, 0, 0, 127, 7, 0,
    0, 8, 0, 2, 0, 0, 0, 135, 7, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8,
    51, 124, 0, 120, 0, 0, 0, 0, 0
#endif
};
#endif

   /* Modified, 176,4 replaced with 1200 * indent in inch */
unsigned char wp_first_line_indent[] = {
    211, 6, 9, 0, 130, 0, 0, 176, 4, 9, 0, 6, 211
};
const unsigned char wp_hanging_indent[] = {
   194, 0, 0, 0, 15, 0, 8, 7, 15, 0, 194
};

const unsigned char wp_hyphen[] = {
   173, 144
};

const unsigned char wp_date[] = {
   216, 0, 11, 0, 50, 47, 49, 47, 37, 53, 0, 11,0, 0, 216
};


   /* Modified, 176,4 replaced with 1200 * indent in inch */
unsigned char wp_para_left_marg[] = {
   211, 12, 14, 0, 0, 2, 0, 176, 4, 0, 0, 14, 0, 211
};
/*
const unsigned char wp_font_courier10[] = {
   209, 1, 35, 0, 0, 88, 2, 120, 0, 254, 21, 54, 16, 88, 7, 0, 0, 0,
   4, 17, 64, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 88, 2, 64, 35, 0, 1, 209};
*/
/* Modified ****/
unsigned char wp_font_courier_new[] = {
   209, 1, 32, 0, 0, 88, 2, 120, 0, 254, 21, 54, 16, 88, 7, 0, 0, 0,
   4, 17, 64, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 32, 0, 1, 209};
const unsigned char wp_underline_tab[] = {
   211, 1, 6, 0, 1, 3, 6, 0, 1, 211};

#define TOP_LOC 8
#define BOT_LOC 10
// loc and loc + 1 = margin * to_inch
unsigned char wp_bottopmargin[] = {208, 5, 12, 0, 176, 4, 176, 4, 176, 4,
    176, 4, 12, 0, 5, 208}; /* Modifiable */
unsigned char wp_margin[] = {208, 1, 12,0, 0,0, 0,0, 0,0, 0,0, 12,0, 1, 208}; /* Modifiable */
const unsigned char wp_tab[] = {193,74,10,0,0,0,0,0,193};
/*
const unsigned char wp_tab[] = {193,0,0,0,0,0,0,0,193};
const unsigned char wp_tab_left[] = {193,0,0,0,0,0,0,0,193};
const unsigned char wp_tab_right[] = {193,74,10,0,0,0,0,0,193};
const unsigned char wp_tab_decimal[] = {193,66,10,0,0,0,0,0,193};
*/

const unsigned char wp_marginrelease[] = {193,0200,0,0,0,0,0,0,193};
/*
const unsigned char wp_center[] = {193,0350,0,0,0,0,0,0,193};
*/
#define HDR_FOOT_JUST_LOC 5
const unsigned char wp_center_justify[] = {208, 6, 6, 0, 1, 2, 6, 0, 6, 208};
const unsigned char wp_left_justify[] = {208, 6, 6, 0, 1, 0, 6, 0, 6, 208};
const unsigned char wp_full_justify[] = {208, 6, 6, 0, 1, 1, 6, 0, 6, 208};
unsigned char *wp_current_justify = (unsigned char *) wp_left_justify;
int wp_current_justify_len = sizeof(wp_left_justify);
const unsigned char wp_boldon[] = {195,0xc,195};
const unsigned char wp_boldoff[] = {196,0xc,196};
const unsigned char wp_underlineon[] = {195,0xe,195};
const unsigned char wp_underlineoff[] = {196,0xe,196};
const unsigned char wp_italicson[] = {195,0x8,195};
const unsigned char wp_italicsoff[] = {196,0x8,196};
const unsigned char wp_resetpage[] = {211,4, 8,0, 0,0, 1,0, 8,0, 4,211};
const unsigned char wp_hardspace[] = {160};
const unsigned char wp_subscripton[] = {195,0x6,195};
const unsigned char wp_subscriptoff[] = {196,0x6,196};
const unsigned char wp_superscripton[] = {195,0x5,195};
const unsigned char wp_superscriptoff[] = {196,0x5,196};
/*
const unsigned char wp_page_lineprinter[] = {
   208, 11, 197, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 176, 79, 144, 51, 1, 11, 108, 105, 110, 101, 112, 114,
   105, 110, 116, 101, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 255, 0, 0, 197, 0, 11, 208};
*/
const unsigned char wp_page_lineprinter[] = {
   208, 11, 197, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 144, 51, 176, 79, 1, 11, 108, 105, 110, 101, 112, 114,
   105, 110, 116, 101, 114, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 255, 0, 0, 197, 0, 11, 208};

#if defined(UNIX) & !defined(WORD)
// Only 13.14 on fixed
double courier_new_cpi[] = { 30, 30, 30, 30, 30, 23.08, 20, 17.65,
                         15, 13.34, 12.0, 10.9,    10};
#else
#ifdef WORD
double courier_new_cpi[] = { 30, 30, 30, 30, 30, 23.08, 20, 17.65,
                         15, 13.30, 12.0, 10.90,    10};
#else
double courier_new_cpi[] = { 30, 30, 30, 30, 30, 23.08, 20, 17.65,
                         15, 13.04, 12.0, 10.71,    10};
#endif
#endif

#define WP_LINE_SPACING_LOC 7
unsigned char wp_line_spacing[] = { 208, 2, 8, 0, 0, 1, 0, 0, 8, 0,
    2, 208};

/*
 * Main loop
 */
main(int argc, char *argv[])
{
/* char fname[80]; */
   char *fname;
   int loop;
   char drvnam[32];
   struct stat stat_buf;

#if DISKFILE == 1
   printf("Please enter the drive for Decmate II Drive: ");
   gets(drvnam);
   drv = toupper(drvnam[0]) - 'A';

   while (1)
   {
      printf("\n\n     Please put Decmate disk in drive %c and a blank\n", drv+'A');
      printf("     formatted disk in drive B, then press return\n");
      gets(fname);
      printf("\n\n     Please wait while I work\n\n");
#else
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

   if (fstat(fileno(infile),&stat_buf) != 0) {
      printf("Unable to stat file\n");
      return(1);
   }
   if (stat_buf.st_size == 256256) {
      rx01_disk = 1;
   }

#endif
   if ((dirfile = fopen("dir.txt", "w")) == 0)
   {
      printf("Unable to open directory file\n");
      return(1);
   }
#ifdef DUMP
   if ((dumpfile = fopen("dump.txt", "wb")) == 0)
   {
      printf("Unable to open dump file\n");
      return(1);
   }
   setvbuf(dumpfile,NULL,_IONBF,0);
#endif

      /*
       * Load in master index
       */
      read_index();

      /*
       * Let's look at all documents
       */
      filename_file = 1;
      for (loop = 0; loop<index_count; loop++)
      {
         if (indexv[loop] != 0) {
            read_document(loop);
             filename_file = 0;
         }
      }
#if DISKFILE == 1
   }
#endif
   read_sector(255);

   fprintf(dirfile,"Name: %s, # of Docs: %d, Blocks left: %d (of %d)\n",
      disk_name,dir_count,buffer[1],buffer[0]);
   fprintf(dirfile,"-------------------------------------------------------------------------------\n");
   fprintf(dirfile,"Number  Name                 Created  Modified       Size Version  Last   Total\n");
   fprintf(dirfile,"-------------------------------------------------------------------------------\n");

   for (loop = 0; loop < dir_count; loop++) {
       fprintf(dirfile,"%3d     %s\n",dir[loop].doc_num,
       dir[loop].filename);
       fprintf(dirfile,
"%30d/%02d/%02d %2d/%02d/%02d %2d:%02d  %-4d %-4d    %2d:%02d  %2d:%02d\n\n",
       dir[loop].create_month,dir[loop].create_day,
       dir[loop].create_year,dir[loop].mod_month,
       dir[loop].mod_day,dir[loop].mod_year,
       dir[loop].mod_hour,dir[loop].mod_min,
       dir[loop].size,dir[loop].version,
       dir[loop].last_hour,dir[loop].last_min,
       dir[loop].total_hour,dir[loop].total_min);
   }

   for (loop = 0; loop < dir_count; loop++) {
      if (dir[loop].comment != NULL) {
         fprintf(dirfile,"\nDocument %d comment:\n   %s",
            dir[loop].doc_num,dir[loop].comment);
         free(dir[loop].comment);
      }
   }
   fclose(dirfile);

   /*
    * End of program
    */
   return(0);
}

#if DISKFILE == 1
/*
 * Read in a sector, using the Decmate II sector numbering scheme
 */
int read_sector(unsigned int secnum)
{
   unsigned int secoff;
   int loop;
   unsigned int xx;
   int track, sector;

#ifndef DEBUG
   printf("Reading Sector %d          \r",secnum);
#endif

   /*
    * Calculate location on disk
    */
   secoff = ((long) secnum + 10L);
   drvoff = 0;

   if ((xx = absread(drv, 1, secoff, &intrack)) != 0)
   {
      printf("Error in seek for sector %ld\n",secnum);
      return(1);
   }

/* printf("\nPosition = %ld ", ftell(infile)); */

   /*
    * Remove four bytes at top of header
    * (who knows what they mean)
    */
   lowword = fgetcx(infile);
   lowword += (fgetcx(infile) & 15) << 8;
   highword = fgetcx(infile);
   highword += (fgetcx(infile) & 15) << 8;

/* printf("low=%d high=%d\n", lowword, highword); */

   /*
    * Pull in 256 12 bit words
    */
   for (loop = 0; loop <= 253; loop++)
   {
      /*
       * Read in one word
       */
      xx = fgetcx(infile);
      xx += (fgetcx(infile) & 15) << 8;
      buffer[loop] = xx;
   }

   /*
    * Succussful completion
    */
   return(0);
}
#else

#define do_interleave(sect) (((((sect) % 26) * 3) % 26) + ((sect) / 26) * 26)

/*
 * Read in a sector, using the VT78 RX01 sector numbering scheme
 */
int read_sector_rx01(unsigned int secnum)
{
   unsigned char buf1[128],buf2[256];
   int incntr,outcntr;
   unsigned int high;

   /* Skip track 0 */
   secnum = (secnum * 3) + 26;

   if (fseek(infile, do_interleave(secnum) * 128L, SEEK_SET) != 0) {
      printf("Error in seek for sector a %d\n",secnum);
      return(1);
   }
   if (fread(buf1,1,128,infile) != 128) {
      printf("Error in read for sector a %d\n",secnum);
      return(1);
   }
   if (fseek(infile, do_interleave(secnum + 1) * 128L, SEEK_SET) != 0) {
      printf("Error in seek for sector b %d\n",secnum + 1);
      return(1);
   }
   if (fread(&buf2[0],1,128,infile) != 128) {
      printf("Error in read for sector b %d\n",secnum + 1);
      return(1);
   }
   if (fseek(infile, do_interleave(secnum + 2) * 128L, SEEK_SET) != 0) {
      printf("Error in seek for sector c %d\n",secnum + 2);
      return(1);
   }
   if (fread(&buf2[128],1,128,infile) != 128) {
      printf("Error in read for sector c %d\n",secnum + 2);
      return(1);
   }

   for (incntr = 1,outcntr = 0; incntr < 128; incntr++, outcntr += 2) {
      high = ((buf1[incntr] & 0xf0) >> 2) |
    (buf2[incntr * 2] >> 6);
      buffer[outcntr] = (buf2[incntr*2] & 0x3f) | (high << 6);
      high = ((buf1[incntr] & 0xf) << 2) |
     (buf2[incntr * 2 + 1] >> 6);
      buffer[outcntr+1] = (buf2[incntr*2+1] & 0x3f) | (high << 6);
   }

   return (0);
}


/*
 * Read in a sector, using the Decmate II sector numbering scheme
 */
int interleave[20] = {0,2,4,6,8,1,3,5,7,9};
#define do_interleave2(sect) (interleave[sect % 10] + ((sect) / 10) * 10)

int read_sector(unsigned int secnum)
{
   unsigned long secoff;
   int loop;
   unsigned int xx;

   /* If in rx01 format go read it */
   if (rx01_disk)
      return (read_sector_rx01(secnum));

#ifdef DEBUG
   //printf("Sector %d   \n",secnum);
#endif

   /*
    * Calculate location on disk
    */
#if 1
   secoff = ((long) do_interleave2(secnum) + 10L) * 512L;
#else
   secoff = ((long) secnum ) * 512L;
#endif

   if (secnum >= 640)
   {
      loop = 1;
   }
   if ((xx = fseek(infile, secoff, SEEK_SET)) != 0)
   {
      printf("Error in seek for sector %ld\n",secnum);
      return(1);
   }

/* printf("\nPosition = %ld ", ftell(infile)); */

   /*
    * Remove four bytes at top of header
    * (who knows what they mean)
    */
   lowword = fgetc(infile);
   lowword += (fgetc(infile) & 15) << 8;
   highword = fgetc(infile);
   highword += (fgetc(infile) & 15) << 8;

/* printf("low=%d high=%d\n", lowword, highword); */

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
//printf("Word %x\n",xx);
      buffer[loop] = xx;
   }

   /*
    * Succussful completion
    */
   return(0);
}


#endif

/*
 * Load in the index record
 */
int read_index()
{
   int loop;
   int doc_count = 0;

   /*
    * Suck in index sector
    */
   read_sector(2);

   for (loop = 0; loop < 3; loop++) {
      disk_name[loop*2] = ((buffer[loop] & 07700) >> 6) + 31;
      disk_name[loop*2+1] = (buffer[loop] & 077) + 31;
   }
   disk_name[6] = 0;
printf("Disk name %s\n",disk_name);
#if 0
   printf("Index sector:\n");
   dump_buffer();
#endif

   /*
    * Get all document pointers
    */
/* for (loop = 8; buffer[loop] != 0; loop++) */
   for (loop = 8; loop <= MAX_DOC; loop++)
   {
      indexv[index_count++] = buffer[loop];
      if (buffer[loop] != 0)
         doc_count++;
   }
   indexv[index_count] = 0;

   /*
    * Write out a short note so we know whats going on
    */
   printf("\nNumber of documents on disk = %d\n\n", doc_count);

      /* Read document directories */
   for (loop = 0; loop < index_count; loop++) {
      if (indexv[loop] != 0) {
         read_sector(indexv[loop]);
         dir[dir_count].filename[0] = 0;
         dir[dir_count].size = buffer[3];
         dir[dir_count].version = buffer[8];
         //dir[dir_count].doc_num = buffer[9];
         dir[dir_count].doc_num = loop+1;

         dir[dir_count].last_min = buffer[12] & 077;
         dir[dir_count].last_hour = (buffer[12] & 07700) >> 6;
         dir[dir_count].total_min = buffer[13] & 077;
         dir[dir_count].total_hour = (buffer[13] & 07700) >> 6;

         dir[dir_count].mod_month = buffer[6] & 077;
         dir[dir_count].mod_day = (buffer[6] & 07700) >> 6;
         dir[dir_count].mod_year = buffer[7];
         dir[dir_count].mod_hour = (buffer[10] & 07700) >> 6;
         dir[dir_count].mod_min = buffer[10] & 077;

         dir[dir_count].create_month = buffer[4] & 077;
         dir[dir_count].create_day = (buffer[4] & 07700) >> 6;
         dir[dir_count].create_year = buffer[5];
         dir[dir_count].comment = NULL;

         dir_count++;
      }
   }

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
   int i,cntr;

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
   wp_bottopmargin[TOP_LOC] = 176;
   wp_bottopmargin[TOP_LOC+1] = 4;
   wp_bottopmargin[BOT_LOC] = 176;
   wp_bottopmargin[BOT_LOC+1] = 4;
   memset(info, 0, sizeof(info));
   for (cntr = 0; cntr < ARRAYSIZE(info); cntr++) {
      info[cntr].cur_lm=10;
      info[cntr].cur_sp=1;
      info[cntr].cur_wm=10;
      info[cntr].cur_rm=75;
      info[cntr].cur_jf=0;
      info[cntr].font_cpi = 10.0;
      info[cntr].cur_cpi = 0.0;
      info[cntr].last_tab = TAB_LEFT;
      for (i = 0; i<=TAB_LEN; i++)
         info[cntr].cur_ts[i] = TAB_NONE;
      for (i = 0; i<=TAB_LEN; i+=5)
         info[cntr].cur_ts[i] = TAB_LEFT;
   }
   font_toinch = 120.0;
   pagewidth_inch = 8.5;


   /*
    * Output the standard prefix
    */
   addbuffer(minprefix, sizeof(minprefix));
   addbuffer(wp_underline_tab, sizeof(wp_underline_tab));

   change_font(info[itype].font_cpi);

   dumpbuffer();
   dump_pgbuff(0);

   //set_default_ruler();

   read_text();

   dumpbuffer();
   dump_pgbuff(0);

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
   read_sector(indexv[docnum]);

#if 0
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
#ifdef DEBUG
   printf("\n\n\n*********************************************\n\n");
#endif
   printf("   Document %d(%d) uses %d(%d) sectors\n",
      docnum+1, actdocnum, docindex_count, seccnt);

   if (actdocnum != docnum+1)
   {
      printf("Internal document number inconsistant, set to expected.\n");
      actdocnum = docnum + 1;
      buffer[9] = actdocnum;
      //retvalue=21;
   }
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

void set_default_ruler()
{

int itype = INFO_NORM;
int i;
            info[itype].lm = 0;  /* Left Margin */
            info[itype].wm = 0; /* Wrap Margin? */
            info[itype].sp = 1;  /* Spacing */
            info[itype].rm = 78;  /* Right Margin */
            info[itype].jf = 0;  /* Justify */
            for (i = 0; i<=TAB_LEN; i++)
               info[itype].ts[i] = TAB_NONE;
            for (i = 0; i<=TAB_LEN; i+=5)
               info[itype].ts[i] = TAB_LEFT;


            if (info[itype].lm < 0)
               info[itype].lm = 0;
            info[itype].char_in_line = info[itype].lm;
            info[itype].ignore_count = 0;
            info[itype].last_tab = TAB_LEFT;
            if (info[itype].rm <= TAB_LEN)
               info[itype].ts[info[itype].rm+1] = TAB_RIGHT;
            wp_setruler();
            dumpbuffer();

}

int read_text()
{
   int ch;
   int soft = 0;
   int xcase = 1;
   int tempch;
   int i;
   int tabflag=0;    /* Eat spaces because were in a tab? */
   int srtflag = 0;
   int center_active = 0;

   info[itype].overstrikeflag = 0;

   /*
    * Force reading of the first sector for this file
    */
   nextblock = 0;
   nextchar=9000;
   highlow = 0;
   center_active = 0;
   info[itype].in_right_tab = 0;

   /*
    * Examine all characters in the file
    */
   while ((ch = read_next()) != -1)
   {

reloop:  switch (ch)
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
/*       printf("<31>"); */
         break;
      /*
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
            DBPS("<?^>\n");
            break;
         case '(':
            soft = 1;
            break;
         case ')':
            soft = 0;
            break;
         case '&':
            info[itype].overstrikeflag = 1;
            break;
         case '\'':
            info[itype].overstrikeflag = 0;
            break;
         case '/':
#if 0
            /*
             * <CtrMkr> - Center line
             */
            tabflag=1;
            insertbuffer(wp_center, sizeof(wp_center), 0);
            DBPS("<Ctr>");
            break;
#else
            /*
             * Subscript off - next single character
             */
            if (info[itype].ch_flags & SUBSCRIPT) {
               wp_resetflag(SUBSCRIPT);
               DBPS("<SubOff>");
            }
            break;
#endif
         case '+':
            /*
             * Page - Hard page
             */
            if ((info[itype].ch_flags & SUPERSCRIPT) != 0)
            {
               wp_resetflag(SUPERSCRIPT);
               /*
                * <PcbMkr-134>
                */
               //wp_setflags(0, 'Z');
               wp_outattr(0);
               dumpbuffer();
               altbufflag = -1;
               altfirstchar = -1;
               altlastret = -1;
               srtflag = 0;
               DBPS("\n<PcbMkr>\n");
               itype = search_ahead();
            }
            else
            {
               if (altbufflag == 0)
               {
                  tabflag=1;
                  //wp_setflags(0, 'Z');
                  wp_outattr(0);
                  pushbuffer(12,1); /* hard page */
                  dump_pgbuff(0);
                  dumpbuffer();
                  srtflag = 0;
                  DBPS("\n<Hpg>\n");
               }
               else
               {
                  /*
                   * <PceMkr-136>
                   */
                  if ((info[itype].ch_flags & SUBSCRIPT) == 0) {
                     printf("Non super/sub PCE end\n");
                  } else
                     wp_resetflag(SUBSCRIPT);

                  //wp_setflags(0,'Z');
                  wp_outattr(0);
                  dumpbuffer();
                  altbufflag = 0;
                  pcehandler();
                  DBPS("\n<PceMkr>\n");
                  itype = INFO_NORM;
               }
            }
            break;

         case '*':
            if ((info[itype].ch_flags & SUBSCRIPT) != 0) {
                if (!center_active) {
                   insertbuffer(wp_center_justify, sizeof(wp_center_justify), 0);
                   DBPS("<Ctr>");
                }
                center_active = 1;
            } else {
               if (center_active != 0) {
                   center_active = 0;
                   insertbuffer(wp_current_justify, wp_current_justify_len,0);
                   DBPS("<ctr>");
                }
            }
            if ((info[itype].ch_flags & UNDERLINE) != 0)
            {
//printf("Hyphen dropped\n");
               /*
                * Hyphenation - Let's do it!
                */
               tabflag = 1;
               addbuffer(wp_hyphen, sizeof(wp_hyphen));
               DBPS("<->");
            }
            else
            {
               if (soft != 0)
               {
                  while ((ch = popbuffer()) == ' ');
                  if (ch != 0)
                     pushbuffer(ch,1);
                  if (info[itype].in_right_tab) {
                     //printf("SRT->HRT\n");
                     pushbuffer(10,1);
                     srtflag = 0;
                     DBPS("<Srt->Hrt>\n");
                  } else {
                     pushbuffer(13,1);
                     srtflag = 1;
                     DBPS("<Srt>\n");
                  }
                  tabflag = 1;
                  /* dumpbuffer(); */
               }
               else
               {
                  tabflag=0;
                  /*
                   * <CarMkr> - Hard return
                   */
                  //wp_setflags(0, 'Z');
                  // Sometimes it has superscript around return, don't
                  // actually send code
                  if ((info[itype].ch_flags ^ info[itype].cur_flags)
                       & UNDERLINE) {
                     wp_setflags((info[itype].cur_flags & ~UNDERLINE) |
                          (info[itype].ch_flags & UNDERLINE), 0);
                  }
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
                     pushbuffer(ch,1);
                  pushbuffer(10,1);
                  dumpbuffer();
                  if (altbufflag) {
                     altlastret = altwpbuffcount - 1;
                  }
                  DBPS("<Rtn>\n");
/*
Put back in
*/
                  tabflag = 1;
                  srtflag = 0;
               }
            }
            info[itype].in_right_tab = 0;
            break;
         case '.':
#if 0
             /*
              * <CtrMkr> - Center line
              */
             tabflag=0;
             while ((wpbuffer[0] == ' ') && (wpbuffcount >0))
             {
                deletebuffer(0,1);
             }
             insertbuffer(wp_center, sizeof(wp_center), 0);
             DBPS("<ctr>");
             break;
#else
            /*
             * Subscript on - next single character
             */
            wp_setflag(SUBSCRIPT);
            DBPS("<SubScr>");
            break;
#endif

         case '!':
            /*
             * <BldMkr> - Bold on
             */
            wp_setflag(BOLD);
            DBPS("<Bold>");
            break;
         case '"':
            /*
             * <BldMkr> - Bold off - next single character
             */
            wp_resetflag(BOLD);
            DBPS("<Unbold>");
            break;
         case '1':
            wp_outattr(ch);
            if (xcase) {
               pushbuffer('[',1);
               DBPC('[');
            } else {
               pushbuffer('{',1);
               DBPC('{');
            }
            break;
         case '2':
            /*
             * Vertical line
             */
            wp_outattr(ch);
            if (xcase) {
               pushbuffer('\\',1);
               DBPC('\\');
            } else {
               pushbuffer('|',1);
               DBPC('|');
            }
            break;
         case '3':
            wp_outattr(ch);
            if (xcase) {
               pushbuffer(']',1);
               DBPC(']');
            } else {
               pushbuffer('}',1);
               DBPC('}');
            }
            break;
         case '4':
            wp_outattr(ch);
            if (xcase) {
               pushbuffer('^',1);
               DBPC('^');
            } else {
               pushbuffer('~',1);
               DBPC('~');
            }
            break;
/*   133         / PRINTABLE CHARACTER "{", "["  (CMD 1)
     134         / PRINTABLE CHARACTER "|", "\"  (CMD 2)
     135         / PRINTABLE CHARACTER "}", "]"  (CMD 3)
     136         / PRINTABLE CHARACTER "~", "^"  (CMD 4)
*/
         case '5':
            /*
             * Underline character?
             */
            tabflag=0;
            wp_outattr(ch);
            pushbuffer('_',1);
            DBPC('_');
            break;
            /* Ignore line modified code */
         case '6':
            break;
         case ',':
#if 0
            /*
             * Subscript on - next single character
             */
            wp_setflag(SUBSCRIPT);
            DBPS("<SubScr>");
            break;
#else
            /*
             * Superscipt on - next single character
             */
            wp_setflag(SUPERSCRIPT);
            DBPS("<SupScr>");
            break;
#endif
         case '-':
#if 0
            /*
             * Subscript off - next single character
             */
            wp_resetflag(SUBSCRIPT);
            DBPS("<SubOff>");
            break;
#else
            /*
             * Superscript off - next single character
             */
            wp_resetflag(SUPERSCRIPT);
            DBPS("<SupOff>");
            break;
#endif
         case '#':
            /*
             * <UdlMkr> - Underline marker - next single character
             */
            wp_setflag(UNDERLINE);
            DBPS("<UndLne>");
            break;
         case '$':
            /*
             * <UdlMkr> - Underline marker - next single character
             */
            wp_resetflag(UNDERLINE);
            DBPS("<UndOff>");
            break;
         case '%':
            /*
             * <Tab> - Tab over
             */
/*
            while ((ch = popbuffer()) == ' ');
            if (ch != 0)
               pushbuffer(ch,1);
*/
            if (srtflag != 0) {
               if (tabflag != 1)
                  printf("Non tabflag soft return tab\n");
               else {
                  /* WPS will put a soft return and tab to next tab field
                     if tab hit at end of line.  Starts after word wrap tab */
                  //printf("Tabbed Srt->Hrt\n");
                     /* Get rid of spaces and soft return */
                  while (((ch = popbuffer()) == ' ') || (ch == 13));
                  if (ch != 13 && ch != ' ')
                     pushbuffer(ch,1);
// order next 2 switched
                  pushbuffer(10,1);
               }
               wp_outattr(0);
               dumpbuffer();
               srtflag = 0;
            }
            {
               int tab_added = 0;

               wp_outattr(0);
                /* Tab in WPS takes a space sometime, add it */
               for (i = info[itype].char_in_line + 1; i <= TAB_LEN; i++) {
                  if (info[itype].ts[i] != 0) {
                     if (info[itype].ts[i] == TAB_RIGHT) {
                        info[itype].ignore_count = i - info[itype].char_in_line;
                        /*
                        printf("Adding tab space\n");
                        */
                        if (info[itype].last_tab == TAB_LEFT) {
                           pushbuffer(' ',0);
                           addbuffer(wp_tab,sizeof(wp_tab));
                        } else {
                           addbuffer(wp_tab,sizeof(wp_tab));
                           pushbuffer(' ',0);
                        }
                        tab_added = 1;
                     }
                     info[itype].char_in_line = i;
                     info[itype].last_tab = info[itype].ts[i];
                     if (info[itype].last_tab == TAB_RIGHT)
                        info[itype].in_right_tab = 1;
                     else
                        info[itype].in_right_tab = 0;
                     break;
                  }
               }
               if (!tab_added)
                  addbuffer(wp_tab,sizeof(wp_tab));
            }
            tabflag=1;
            DBPS("<Tab>");
            break;
         case '7':
            /*
             * <RulMkr> - Ruler Marker
             */
            DBPS("<Ruler::");
            if (srtflag) {
                 /* Get rid of soft return */
               ch = popbuffer();
               if (ch != 13)
                  pushbuffer(ch,1);
               pushbuffer(10,1);
               dumpbuffer();
            }
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

            info[itype].lm = 0;  /* Left Margin */
            info[itype].sp = 1;  /* Spacing */
            info[itype].wm = -1; /* Wrap Margin? */
            info[itype].rm = 0;  /* Right Margin */
            info[itype].jf = 0;  /* Justify */
            for (i=0; i<=TAB_LEN; i++)
               info[itype].ts[i] = TAB_NONE;
            i = 0;

            while ((ch = read_next()) != '^')
            {
               if (ch < 65)
               {
                  i = (i * 16) + (ch & 15);
               }
               else
               {
#ifdef DEBUG
                   printf("(T-%d%c)",i,ch);
#endif
                   switch (ch)
                   {
                   case '[':
                      xcase = 1;
                      break;
                   case ']':
                      xcase = 0;
                      break;
                      /* Word wrap indent */
                   case 'H':
                      info[itype].wm = i;
                      if (i <= TAB_LEN)
                         info[itype].ts[i] = TAB_LEFT;
                      else
printf("tab overflow %d\n",i);
                      break;
                      /* Paragraph indent */
                   case 'I':
printf("para indent tab\n");
                      if (i <= TAB_LEN)
                         info[itype].ts[i] = TAB_LEFT;
                      else
printf("tab overflow %d\n",i);
                      break;
                      /* Decimal tab */
                   case 'A':
                      if (i <= TAB_LEN)
                         info[itype].ts[i] = TAB_DEC;
                      else
printf("tab overflow %d\n",i);
                      break;
                      /* Right justified tab */
                   case 'B':
                      if (i <= TAB_LEN)
                         info[itype].ts[i+1] = TAB_RIGHT;
                      else
printf("tab overflow %d\n",i);
                  break;
/* Not possible with wordperfect */
                      /* Center point */
                   case 'J':
#if 0
                      if (i <= TAB_LEN)
                         info[itype].ts[i] = TAB_CENTER;
                      else
printf("tab overflow %d\n",i);
#else
                      printf("Center tab ignored\n");
#endif
                  break;
                  /* Left justified tab */
                   case 'C':
                      if (i <= TAB_LEN)
                         info[itype].ts[i] = TAB_LEFT;
                      else
printf("tab overflow %d\n",i);
                      break;
                      /* Left margin single space*/
                   case 'D':
                      info[itype].lm = i;
                      info[itype].sp = 1;
                      break;
                      /* Left margin double space*/
                   case 'F':
                      info[itype].lm = i;
                      info[itype].sp = 2;
                      break;
                      /* Right margin ragged */
                   case 'E':
                      wp_current_justify = (unsigned char *) wp_left_justify;
                      wp_current_justify_len = sizeof(wp_left_justify);
                      info[itype].rm = i;
                      break;
                      /* Right margin justified */
                   case 'G':/*W*/
                      wp_current_justify = (unsigned char *) wp_full_justify;
                      wp_current_justify_len = sizeof(wp_full_justify);
                      info[itype].rm = i;
//printf("right justify\n");
                      break;
                   /* Hyphenation zone, not meaningful in WP */
                   case 'L':
                      break;
                   case '?':/*J*/
printf("justify? ruler\n");
                      info[itype].rm = i;
                      info[itype].jf=1;
                      break;
/* Missing
/   J 53 13    0  CENTERING POINT                 C
/   K 54 14    0  LEFT MARGIN, SPACE-AND-A-HALF   N
/   L 55 15    0  HYPHENATION ZONE                H
/   M 56 16    0  LEFT MARGIN, HALF-SPACE         F
*/

                   default:
                      printf("\n<Ruler error %c-%d>\n",ch,i);
                      break;
                   }
                   i=0;
               }
            }
            if (info[itype].lm < 0)
               info[itype].lm = 0;
            info[itype].char_in_line = info[itype].lm;
            info[itype].ignore_count = 0;
            info[itype].last_tab = TAB_LEFT;
            if (info[itype].wm < 0)
               info[itype].wm = info[itype].lm;

#if 0
            if (lm <= TAB_LEN)
               info[itype].ts[lm] = TAB_LEFT;
            else
printf("lm overflow %d\n",i);
#endif

            if (info[itype].rm <= TAB_LEN)
               info[itype].ts[info[itype].rm+1] = TAB_RIGHT;
            else
printf("rm overflow %d\n",i);
            if (info[itype].wm > info[itype].lm) {
               if (info[itype].wm <= TAB_LEN)
                  info[itype].ts[info[itype].wm] = TAB_LEFT;
               else
printf("wm overflow %d\n",i);
            }
            wp_setruler();
            dumpbuffer();
            /*
             * Eat '8' (hopefully) 1
             */
            ch = read_next();
#ifdef DEBUG
            printf(">\n");
#endif
            tabflag = 1;
            srtflag = 0;
            break;

         default:
printf("Unknown command %c (%d)\n",ch,ch);
#ifdef DEBUG
            printf("<94-%c>",ch);
#endif
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
/*
         if (tabflag == 0 && soft == 0)
*/
         wp_outattr(0);
         if (soft == 0)
            pushbuffer(' ',1);
         DBPC(' ');
         break;
      /*
       * Must be normal text
       */
      default:
         if (soft) {
           // printf("Soft character %c\n",ch);
         }

         tabflag=0;
         if (xcase == 0)
         {
            if (ch == '@')
               ch = '`';
            else
               ch = tolower(ch);
         }
         wp_outattr(ch);
         DBPC(ch);
         if (altbufflag == -1) {
            altlastret = -1;
         }
         if (altbufflag == -1 && altfirstchar == -1) {
            altfirstchar = wpbuffcount + altwpbuffcount;
         }
         pushbuffer(ch,1);
      }
   }

 doneread:
   return(0);
}

unsigned char preread[1024];
int preread_loc = 0;
int num_preread = 0;

int search_ahead()
{
   int mark_char = 0;
   int ignore_char = 0;
   unsigned char chars[7];
   int ch;
   int char_cntr = 0;
   int retc;
   int done = 0;

   retc = INFO_NORM;
   preread_loc = 0;
   while (!done) {
      ch = read_next();
      if (ch == -1) {
         done = 1;
         printf("Early end of file\n");
      }
      preread[preread_loc++] = ch;
      if (preread_loc >= ARRAYSIZE(preread)) {
         printf("Preread overflow\n");
         done = 1;
      }
      if (ch == '^')
         mark_char = 1;
      else
      if (mark_char) {
         switch(ch) {
           case '+':
              done = 1;
              break;
           case '7':
              ignore_char = 1;
              break;
           case '8':
              ignore_char = 0;
              break;
         }
         mark_char = 0;
      } else {
        if (!ignore_char) {
           if (strchr("\[] ",ch) == NULL)
              chars[char_cntr++] = ch;
           if (char_cntr == 3) {
              if (strncasecmp(chars, "TOP", 3) == 0) {
                 retc = INFO_HDR;
                 done = 1;
              }
           }
           if (char_cntr >= 6) {
              if (strncasecmp(chars, "BOTTOM", 6) == 0) {
                 retc = INFO_FOOT;
              } else {
                chars[6] = 0;
                printf("Unable to match %s\n",chars);
              }
              done = 1;
           }
         }
      }
   }
   num_preread = preread_loc;
   preread_loc = 0;
   return retc;
}
/*
 * Pull one character from the file
 */
int read_next()
{
   int ch;

   if (num_preread != 0) {
      num_preread--;
      return preread[preread_loc++];
   }
   do {
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
#ifdef DUMP
      fprintf(dumpfile,"%c",ch);
      if (ch == '*')
         fprintf(dumpfile,"\n");
#endif
      /* Discard null characters, they can be stuck anywhere */
   } while (ch == 31);
   return(ch);
}

/*
 * Output regular text character to wp file
 */
void wp_outattr(int ch)
{
   /*
    * Check for any attributes that need to be changed
    */
   if (info[itype].ch_flags != info[itype].cur_flags)
   {
      wp_setflags(info[itype].ch_flags, ch);
   }
}


void wp_setflags(int new, int ch)
{

   /*
    * Should we turn on bold?
    * Doesn't change for spaces.
    */
   if (((new & BOLD) == BOLD) && ((info[itype].cur_flags & BOLD) != BOLD))
   {
      addbuffer(wp_boldon, sizeof(wp_boldon));
   }

   /*
    * Should we turn on bold?
    * Doesn't change for spaces.
    */
   if (((new & ITALICS) == ITALICS) && ((info[itype].cur_flags & ITALICS) != ITALICS))
   {
      addbuffer(wp_italicson, sizeof(wp_italicson));
   }

   /*
    * Should we turn off bold?
    * Doesn't change for spaces.
    */
   if (((new & BOLD) != BOLD) && ((info[itype].cur_flags & BOLD) == BOLD))
   {
      addbuffer(wp_boldoff, sizeof(wp_boldoff));
   }

   /*
    * Should we turn off bold?
    */
   if (((new & ITALICS) != ITALICS) && ((info[itype].cur_flags & ITALICS) == ITALICS))
   {
      addbuffer(wp_italicsoff, sizeof(wp_italicsoff));
   }

   /*
    * Should we turn on superscript?
    * Doesn't change for spaces.
    */
   if (((new & SUPERSCRIPT) == SUPERSCRIPT) && ((info[itype].cur_flags & SUPERSCRIPT) != SUPERSCRIPT))
   {
#ifdef DEBUG
printf("Adding superscript\n");
#endif
      addbuffer(wp_superscripton, sizeof(wp_superscripton));
   }

   /*
    * Should we turn off superscript?
    * Doesn't change for spaces.
    */
   if (((new & SUPERSCRIPT) != SUPERSCRIPT) && ((info[itype].cur_flags & SUPERSCRIPT) == SUPERSCRIPT))
   {
#ifdef DEBUG
printf("removing superscript\n");
#endif
      addbuffer(wp_superscriptoff, sizeof(wp_superscriptoff));
   }

   /*
    * Should we turn on subscript?
    * Doesn't change for spaces.
    */
   if (((new & SUBSCRIPT) == SUBSCRIPT) && ((info[itype].cur_flags & SUBSCRIPT) != SUBSCRIPT))
   {
#ifdef DEBUG
printf("Adding subscript\n");
#endif
      addbuffer(wp_subscripton, sizeof(wp_subscripton));
   }

   /*
    * Should we turn off subscript?
    * Doesn't change for spaces.
    */
   if (((new & SUBSCRIPT) != SUBSCRIPT) && ((info[itype].cur_flags & SUBSCRIPT) == SUBSCRIPT))
   {
#ifdef DEBUG
printf("removing subscript\n");
#endif
      addbuffer(wp_subscriptoff, sizeof(wp_subscriptoff));
   }

   /*
    * Should we turn on underline?
    */
   if (((new & UNDERLINE) == UNDERLINE) && ((info[itype].cur_flags & UNDERLINE) != UNDERLINE))
   {
      addbuffer(wp_underlineon, sizeof(wp_underlineon));
   }

   /*
    * Should we turn off underline?
    */
   if (((new & UNDERLINE) != UNDERLINE) && ((info[itype].cur_flags & UNDERLINE) == UNDERLINE))
   {
      addbuffer(wp_underlineoff, sizeof(wp_underlineoff));
   }

   info[itype].cur_flags = new;

}

void wp_setflag(int fl)
{
   info[itype].ch_flags |= fl;
}

void wp_resetflag(int fl)
{
   info[itype].ch_flags = info[itype].ch_flags & ~fl;
}

/*
 * DumpBuffer
 */
void dumpbuffer()
{
   int tabadj, i;
   int wpu;

#if 0
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
#else
   if ((printable() != 0) && (altbufflag == 0))
   {
      if (info[itype].wm != info[itype].lm) {
         wpu = floor((info[itype].lm - info[itype].wm) * font_toinch + .5);
         wp_first_line_indent[7] = wpu & 255;
         wp_first_line_indent[8] = wpu >> 8;
         insertbuffer(wp_first_line_indent,sizeof(wp_first_line_indent),0);
      }
   }
#endif
   if (altbufflag == 0)
   {
      //fwrite(wpbuffer, wpbuffcount, 1, destfd);
      if (pgbuffcount + wpbuffcount >= ARRAYSIZE(pgbuff)) {
         printf("Page buffer overflow\n");
         dump_pgbuff(0);
      }
      memcpy(&pgbuff[pgbuffcount], wpbuffer, wpbuffcount);
      pgbuffcount += wpbuffcount;
   }
   else
   {
      altaddbuffer(wpbuffer, wpbuffcount);
   }
   wpbuffcount = 0;
}

void dump_pgbuff(int insert) {
   int pgstart;

     if (insert) {
        pgstart = pgbuffcount;
        dumpbuffer();
        fwrite(&pgbuff[pgstart], pgbuffcount - pgstart, 1, destfd);
        fwrite(pgbuff, pgstart, 1, destfd);
     } else {
        fwrite(pgbuff, pgbuffcount, 1, destfd);
     }
     pgbuffcount = 0;
}

/*
 * AddBuffer
 */
void addbuffer(const unsigned char *text, int len)
{
   while (len-- && wpbuffcount < sizeof(wpbuffer))
   {
      wpbuffer[wpbuffcount++] = *text++;
   }
   if (wpbuffcount >= sizeof(wpbuffer))
      printf("wpbuffer overflow\n");
}

/*
 * AltAddBuffer
 */
void altaddbuffer(const unsigned char *text, int len)
{
   while (len-- && altwpbuffcount < sizeof(altwpbuffer))
   {
      altwpbuffer[altwpbuffcount++] = *text++;
   }
   if (altwpbuffcount >= sizeof(altwpbuffer))
      printf("altwpbuffer overflow\n");
}

/*
 * AltDeleteBuffer
 */
void altdeletebuffer(int start, int count)
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
void insertbuffer(const unsigned char *text, int len, int offset)
{
   int tmp;

   if (altfirstchar != -1 && altfirstchar >= (offset + altwpbuffcount))
      altfirstchar += len;

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
 * altInsertBuffer
 */
void altinsertbuffer(const unsigned char *text, int len, int offset)
{
   int tmp;
   for (tmp = altwpbuffcount; tmp >= offset; tmp--)
   {
      altwpbuffer[tmp+len] = altwpbuffer[tmp];
   }

   altwpbuffcount+=len;

   while (len--)
   {
      altwpbuffer[offset++] = *text++;
   }
}

/*
 * PopBuffer
 */
int popbuffer()
{
   if (wpbuffcount != 0)
   {
      info[itype].char_in_line--;
      return(wpbuffer[--wpbuffcount]);
   }
   else
   {
      return(0);
   }
}

void filename_check(int ch)
{
  static int last_char = 0;
  static int fileid;
  static int in_filename = 0;
  static int in_fileid = 0;
  static char filename_hold[MAX_FILENAME];
  static char filename_cntr;
  int loop;

  if (filename_file) {
     if (in_filename) {
        if (ch == '<') {
           in_filename = 0;
           filename_hold[filename_cntr] = 0;
        } else {
           if (filename_cntr < MAX_FILENAME - 1) {
              filename_hold[filename_cntr++] = ch;
           } else
              printf("Filename to long, truncated\n");
        }
     }
     if (in_fileid) {
        if (ch == '<') {
           in_fileid = 0;
           for (loop = 0; loop < dir_count; loop++) {
              if (fileid == dir[loop].doc_num) {
                 strcpy(dir[loop].filename,filename_hold);
                 break;
              }
           }
        } else
           fileid = fileid * 10 + ch - '0';
     }

     if (ch == '>') {
        if (last_char == 'n') {
           in_filename = 1;
           filename_cntr = 0;
        } else if (last_char == '#') {
           in_fileid = 1;
           fileid = 0;
        } else
           if (last_char != '<')
              printf("Unknown filename character %c\n",last_char);
     }
     last_char = ch;
  }
}


/*
 * Pushbuffer
 */
void pushbuffer(int ch,int count)
{
   int ch1,ch2;

   if (count) {
      if (ch >= 10 && ch <= 13) {
         info[itype].char_in_line = info[itype].lm;
         info[itype].ignore_count = 0;
         info[itype].last_tab = TAB_LEFT;
      } else {
         if (info[itype].ignore_count-- <= 0)
            info[itype].char_in_line++;
      }
   }
   if (wpbuffcount >= sizeof(wpbuffer)) {
      printf("**********wpbuffer overflow\n");
      return;  // Give up
   }

   wpbuffer[wpbuffcount++] = ch;
   switch (info[itype].overstrikeflag)
   {
   case 0:
      break;
   case 1:
      info[itype].overstrikeflag = 2;
      break;
   case 2:
      ch1 = popbuffer();
      ch2 = popbuffer();
      info[itype].overstrikeflag = 0;
      overstrike(ch1,ch2);
      info[itype].overstrikeflag = 2;
      break;
   }
}

/*
 * DeleteBuffer
 */
void deletebuffer(int start, int count)
{
   int i;

   for (i = start + count; i < wpbuffcount; i++)
   {
      wpbuffer[i-count] = wpbuffer[i];
   }
   wpbuffcount -= count;
}

/*
 * strncasestr
 */
int strncasestr(const char *source, const char *match, int count)
{
   int i;
   int len;

   len = strlen(match);

   for (i = 0; i <= count - len; i++)
   {
      if (strncasecmp(source+i, match, len) == 0)
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
         if (wpbuffer[1] == 0 || wpbuffer[1] == wp_tab[1])
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
   {'A','\'',1, 26},
   {'a','\'',1, 27},
   {'A','^', 1, 28},
   {'a','^', 1, 29},
   {'A','"', 1, 30},
   {'a','"', 1, 31},
   {'A','`', 1, 32},
   {'a','`', 1, 33},
   {'E','\'', 1, 40},
   {'e','\'', 1, 41},
   {'E','^', 1, 42},
   {'e','^', 1, 43},
   {'E','"', 1, 44},
   {'e','"', 1, 45},
   {'E','`', 1, 46},
   {'e','`', 1, 47},
   {'I','\'',1, 48},
   {'i','\'',1, 49},
   {'I','^', 1, 50},
   {'i','^', 1, 51},
   {'I','"', 1, 52},
   {'i','"', 1, 53},
   {'I','`', 1, 54},
   {'i','`', 1, 55},
   {'N','~', 1, 56},
   {'n','~', 1, 57},
   {'O','\'',1, 58},
   {'o','\'',1, 59},
   {'O','^', 1, 60},
   {'o','^', 1, 61},
   {'O','"', 1, 62},
   {'o','"', 1, 63},
   {'O','`', 1, 64},
   {'o','`', 1, 65},
   {'U','`', 1, 66},
   {'u','`', 1, 67},
   {'U','^', 1, 68},
   {'u','^', 1, 69},
   {'U','"', 1, 70},
   {'u','"', 1, 71},
   {'U','`', 1, 72},
   {'u','`', 1, 73},
   {'Y','\"',1, 74},
   {'D','-', 1, 78},
   {'O','/', 1, 80},
   {'o','/', 1, 81},
   {'O','~', 1, 82},
   {'o','~', 1, 83},
   {'Y','\'',1, 84},
   {'y','\'',1, 85},
   {'C','\'',1, 96},
   {'c','\'',1, 97},
   {'C','^', 1,100},
   {'c','^', 1,101},
   {'G','\'',1,114},
   {'g','\'',1,115},
   {'G','^', 1,122},
   {'g','^', 1,123},
   {'H','^', 1,126},
   {'h','^', 1,127},
   {'I','~', 1,136},
   {'i','~', 1,137},
   {'J','^', 1,140},
   {'j','^', 1,141},
   {'L','\'',1,144},
   {'l','\'',1,145},
   {'N','\'',1,154},
   {'n','\'',1,155},
   {'R','\'',1,168},
   {'r','\'',1,169},
   {'S','\'',1,174},
   {'s','\'',1,175},
   {'S','^', 1,180},
   {'s','^', 1,181},
   {'U','~', 1,198},
   {'u','~', 1,199},
   {'W','^', 1,200},
   {'w','^', 1,201},
   {'Y','^', 1,202},
   {'y','^', 1,203},
   {'Z','\'',1,204},
   {'z','\'',1,205},
   {'R','`', 1,218},
   {'r','`', 1,219},
   {'Y','`', 1,226},
   {'y','`', 1,227},
   {0,0,0,0}
};

int overstrike(char first, char second)
{
   int loop;

printf("Using overstrike\n");
   if ((first == ' ') && (second == ' '))
   {
      pushbuffer(' ',1);
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
         pushbuffer(192,1);
         pushbuffer(overlist[loop].item,0);
         pushbuffer(overlist[loop].table,0);
         pushbuffer(192,0);
         goto goodfinish;
      }
   }
   /*
    * If we can't find it, just slap in the two characters
    * and hope for the best.
    */
   pushbuffer(first,1);
   pushbuffer(second,1);
goodfinish:
   return(0);
}

void change_font(double font_cpi)
{
   int wpu;
   int point;

   if (info[itype].cur_cpi != font_cpi) {
      info[itype].cur_cpi = font_cpi;
      if (font_cpi != 10)
         printf("Switching cpi to %f\n",font_cpi);
      for (point = ARRAYSIZE(courier_new_cpi) - 1; point > 0; point--) {
         if (courier_new_cpi[point] == font_cpi) {
            break;
         }
      }

      wpu = point * 50 + .5;
      wp_font_courier_new[5] = wpu & 255;
      wp_font_courier_new[6] = wpu >> 8;
      wpu = point * 10 + .5;
      wp_font_courier_new[7] = wpu & 255;
      wp_font_courier_new[8] = wpu >> 8;
      /*
      wp_font_courier_new[23] = (ARRAYSIZE(courier_new_cpi) - point);
      */
      addbuffer(wp_font_courier_new, sizeof(wp_font_courier_new));
   }
}

/*
 * Let's see what we can do about the rulers
 */
void wp_setruler()
{
   int wpu;
   int flag;
   int i;
   int tmp;
   int tbuff[40],tbuff_type[40];
   int tcount = 0;
   int type,last_type;
   int len;
   char ch;

   if (altbufflag) {
      ch = wp_current_justify[HDR_FOOT_JUST_LOC] + 128;
      addbuffer(&ch, 1);
   } else
      addbuffer(wp_current_justify, wp_current_justify_len);
   if (info[itype].cur_sp != info[itype].sp) {
      wp_line_spacing[WP_LINE_SPACING_LOC] = info[itype].sp;
      addbuffer(wp_line_spacing, sizeof(wp_line_spacing));
   }
   /*
    * Left and right margins
    */
   if ((info[itype].cur_lm != info[itype].lm) ||
        (info[itype].cur_rm != info[itype].rm) ||
        (info[itype].cur_wm != info[itype].wm))
   {
      if (info[itype].lm > info[itype].wm)
         printf("lm is greater than wm\n");
#if 0
      if (info[itype].lm / info[itype].font_cpi < min_left_margin) {
         left_margin_shift = min_left_margin - info[itype].lm /
               info[itype].font_cpi;
      } else {
         left_margin_shift = 0;
      }
#endif
     left_margin_shift = min_left_margin;
if (info[itype].rm - info[itype].lm > 110) {
//if (0) {
      if (info[itype].cur_rm != info[itype].rm) {
         if (info[itype].rm / info[itype].font_cpi + left_margin_shift >
             (pagewidth_inch - (min_left_margin + min_right_margin))) {
printf("Switching to lineprinter page\n");
            pagewidth_inch = 17;
            addbuffer(wp_page_lineprinter,sizeof(wp_page_lineprinter));
         }
      }
} else {
      if (info[itype].cur_rm != info[itype].rm) {
         for (i = ARRAYSIZE(courier_new_cpi) - 1; i > 0; i--) {
            if (info[itype].rm / courier_new_cpi[i] + left_margin_shift <=
                 (pagewidth_inch - (min_left_margin + min_right_margin))) {
               info[itype].font_cpi = courier_new_cpi[i];
               font_toinch = 120.0 * 10.0 / info[itype].font_cpi;
               break;
            }
         }
         change_font(info[itype].font_cpi);
#if 0
         if (info[itype].lm / info[itype].font_cpi < min_left_margin) {
            left_margin_shift = min_left_margin - info[itype].lm /
              info[itype].font_cpi;
         } else {
            left_margin_shift = 0;
         }
#endif
      }
}

#ifndef UNKNOWN
          /* Wrap margin */
      wpu = (info[itype].wm + left_margin_shift * info[itype].font_cpi) *
             font_toinch + .5;
      wp_margin[8] = wpu & 255;
      wp_margin[9] = wpu >> 8;
         /* Right Margin */
      wpu = ((pagewidth_inch - left_margin_shift) * info[itype].font_cpi -
          (info[itype].rm + 1)) * font_toinch + .5;
      wp_margin[10] = wpu & 255;
      wp_margin[11] = wpu >> 8;
      addbuffer(wp_margin, sizeof(wp_margin));
#else
// This doesn't seem to work, don't know what I was trying
          /* left margin */
      wpu = (info[itype].lm + left_margin_shift * info[itype].font_cpi) *
            font_toinch + .5;
      wp_margin[8] = wpu & 255;
      wp_margin[9] = wpu >> 8;
         /* Right Margin */
      wpu = ((pagewidth_inch - left_margin_shift) * info[itype].font_cpi -
          (info[itype].rm + 1)) * font_toinch + .5;
      wp_margin[10] = wpu & 255;
      wp_margin[11] = wpu >> 8;
      addbuffer(wp_margin, sizeof(wp_margin));

          /* wrap margin */
      wpu = floor((info[itype].wm - info[itype].lm + left_margin_shift *
         info[itype].font_cpi) * font_toinch + .5);
      wp_para_left_marg[7] = wpu & 255;
      wp_para_left_marg[8] = wpu >> 8;
      addbuffer(wp_para_left_marg, sizeof(wp_para_left_marg));

#endif
      info[itype].cur_lm = info[itype].lm;
      info[itype].cur_rm = info[itype].rm;
      info[itype].cur_wm = info[itype].wm;
      info[itype].cur_sp = info[itype].sp;
   }

   /*
    * See if the rulers changed
    */
   for (i = 0; i<40; i++) {
      tbuff[i] = 0;
      tbuff_type[i] = TAB_NONE;
   }

   flag = 0;
   for (i=1; i<=TAB_LEN; i++)
   {
      if (info[itype].ts[i] != info[itype].cur_ts[i])
         flag = 1;
      if (info[itype].ts[i] != TAB_NONE)
      {
         tbuff_type[tcount] = info[itype].ts[i];
         if (make_abs_tabs) {
            tbuff[tcount] = font_toinch * (i +
                  min_left_margin * info[itype].font_cpi) + .5;
            tcount++;
         } else {
               /* 1125 is wp offset on relative tabs */
            tbuff[tcount++] = font_toinch * (i - info[itype].wm) + 1125.5;
         }
      }
      info[itype].cur_ts[i] = info[itype].ts[i];
   }
   if (flag)
   {
      if (make_abs_tabs)
         len = 204;
      else
         len = 208;
      pushbuffer(208,0);
      pushbuffer(4,0);
      pushbuffer(len & 255,0);     /* Length */
      pushbuffer(len >> 8,0);
      for (i=0; i<=39; i++)      /* Old Tab Positions */
      {
         pushbuffer(0,0);
         pushbuffer(0,0);
      }
      for (i=0; i<=39; i += 2)   /* Old tab types */
         pushbuffer(0,0);
      for (i=0; i<=39; i++)      /* New tab positions */
      {
         if (tbuff[i] == 0)
            tmp = -1;
         else
            tmp = tbuff[i];
         pushbuffer(tmp & 255,0);
         pushbuffer(tmp >> 8,0);
      }
      for (i=0; i<=39; i++)      /* New tab types */
      {
         switch(tbuff_type[i]) {
#if 0
            case TAB_CENTER:
          type = 1;
          break;
#endif
            case TAB_RIGHT:
          type = 2;
          break;
            case TAB_DEC:
          type = 3;
          break;
            default:
          type = 0;
          break;
         }
         if (i & 1)
            pushbuffer(last_type | type,0);
         else
            last_type = type << 4;
      }
     if (!make_abs_tabs) {
         pushbuffer(176,0);
         pushbuffer(4,0);
         pushbuffer(101,0);
         pushbuffer(4,0);
      }
      pushbuffer(len & 255,0);     /* Length */
      pushbuffer(len >> 8,0);
      pushbuffer(4,0);
      pushbuffer(208,0);
      dumpbuffer();

      /*
       * Calculate forced indent
       */
      forceindent = 0;
      if (info[itype].lm < info[itype].wm)
      {
         for (i = info[itype].lm+1; i<= info[itype].wm; i++)
         {
            if (info[itype].ts[i] != TAB_NONE)
               forceindent--;
         }
      }
      if (info[itype].lm > info[itype].wm)
      {
         for (i = info[itype].wm+1; i<= info[itype].lm; i++)
         {
            if (info[itype].ts[i] != TAB_NONE)
               forceindent++;
         }
      }
   }
}

void dump_buffer()
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
void pcehandler()
{
   int i;
   int found = 0;

   if (altfirstchar == -1) {
      printf("Altfirstchar = -1\n");
      altfirstchar = 0;
   }

   /*
    * Reset Page Number
    */
   if (strncasecmp(altwpbuffer + altfirstchar, "RESET", 5) == 0)
   {
      found = 1;
      addbuffer(wp_resetpage, sizeof(wp_resetpage));
      dumpbuffer();
   }

   /*
    * Top of page format
    */
   if (strncasecmp(altwpbuffer + altfirstchar, "TOP", 3) == 0)
   {
      found = 1;
         // Remove top
      altdeletebuffer(altfirstchar,4);

      altlastret -=4;
      //WPS last hard return doesn't space but WP does
      if (altlastret >= 0)
         altdeletebuffer(altlastret,1);
      while ((i = strncasestr(altwpbuffer, "\\p", altwpbuffcount)) != 0)
      {
         altwpbuffer[i] = 2;  /* Page # */
         altdeletebuffer(i+1,1); /* Lose one character */
      }

      while ((i = strncasestr(altwpbuffer, "\\d", altwpbuffcount)) != 0)
      {
         altdeletebuffer(i,2); /* Lose two character */
         altinsertbuffer(wp_date,sizeof(wp_date),i);
      }
      altinsertbuffer(wp_underline_tab,sizeof(wp_underline_tab),0);

      /*
       * Create information
       */
      pushbuffer(213,0);
      pushbuffer(0,0);
      pushbuffer((altwpbuffcount+22) & 255,0);  /* Size in bytes */
      pushbuffer((altwpbuffcount+22) / 256,0);
      pushbuffer(0,0);       /* Old Occurence flag */
      pushbuffer(0,0);       /* Old formatter lines */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* Old position */
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(1,0);       /* New occurence flag */
      pushbuffer(0,0);       /* New formatter lines */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* New position */
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);       /* # boxes */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* Hash value */
      pushbuffer(0,0);
      for (i = 0; i < altwpbuffcount; i++)
      {
         pushbuffer(altwpbuffer[i],0);
      }
      pushbuffer((altwpbuffcount+22) & 255,0);  /* Size in bytes */
      pushbuffer((altwpbuffcount+22) / 256,0);
      pushbuffer(0,0);
      pushbuffer(213,0);
      dumpbuffer();
      wp_bottopmargin[TOP_LOC] = 104;
      wp_bottopmargin[TOP_LOC+1] = 1;
      addbuffer(wp_bottopmargin, sizeof(wp_bottopmargin));
      dump_pgbuff(1);

   }

   /*
    * Bottom of page format
    */
   if (strncasecmp(altwpbuffer + altfirstchar, "BOTTOM", 6) == 0)
   {
      found = 1;
         // Remove bottom
      altdeletebuffer(altfirstchar,7);

      altlastret -= 7;
      //WPS last hard return doesn't space but WP does
      if (altlastret >= 0)
         altdeletebuffer(altlastret,1);

      while ((i = strncasestr(altwpbuffer, "\\p", altwpbuffcount)) != 0)
      {
         altwpbuffer[i] = 2;  /* Page # */
         altdeletebuffer(i+1,1); /* Lose one character */
      }

      while ((i = strncasestr(altwpbuffer, "\\d", altwpbuffcount)) != 0)
      {
         altdeletebuffer(i,2); /* Lose two character */
         altinsertbuffer(wp_date,sizeof(wp_date),i);
      }
      altinsertbuffer(wp_underline_tab,sizeof(wp_underline_tab),0);

      /*
       * Create information
       */
//printf("\n***LEN %d:%d %d,%d\n",altwpbuffcount+22,wpbuffcount,
//(altwpbuffcount+22) & 255,(altwpbuffcount+22) / 256);
      pushbuffer(213,0);
      pushbuffer(2,0);
      pushbuffer((altwpbuffcount+22) & 255,0);  /* Size in bytes */
      pushbuffer((altwpbuffcount+22) / 256,0);
      pushbuffer(0,0);       /* Old Occurence flag */
      pushbuffer(0,0);       /* Old formatter lines */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* Old position */
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(1,0);       /* New occurence flag */
      pushbuffer(0,0);       /* New formatter lines */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* New position */
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);
      pushbuffer(0,0);       /* # boxes */
      pushbuffer(0,0);
      pushbuffer(0,0);       /* Hash value */
      pushbuffer(0,0);
      for (i = 0; i < altwpbuffcount; i++)
      {
         pushbuffer(altwpbuffer[i],0);
      }
      pushbuffer((altwpbuffcount+22) & 255,0);  /* Size in bytes */
      pushbuffer((altwpbuffcount+22) / 256,0);
      pushbuffer(2,0);
      pushbuffer(213,0);
      //dumpbuffer();
      wp_bottopmargin[BOT_LOC] = 104;
      wp_bottopmargin[BOT_LOC+1] = 1;
      insertbuffer(wp_bottopmargin, sizeof(wp_bottopmargin), 0);
      dump_pgbuff(1);
   }
   if (!found) {
      int offset;

      found = (strncasecmp(altwpbuffer + altfirstchar, "COMMENT", 7) == 0);
      if (found)
         altfirstchar += 8;
      //found = 1;
      for (i = 0; i < MAX_DOC; i++) {
         if (dir[i].doc_num == actdocnum) {
            dir[i].comment = malloc(altwpbuffcount + 1 - altfirstchar);
            memcpy(dir[i].comment,&altwpbuffer[altfirstchar],altwpbuffcount - 
               altfirstchar);
            dir[i].comment[altwpbuffcount-altfirstchar] = 0;
            break;
         }
      }
   }
   if (!found) {
      altwpbuffer[altwpbuffcount] = 0;
      printf("Didn't match PCE %s\n",altwpbuffer);
   }
   altwpbuffcount = 0;
}

