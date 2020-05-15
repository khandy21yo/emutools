
/******************************  derad50  ************************************/


static void derad50(	/* decode a word in which 3 characters are coded by */
			/* the RAD50 scheme. */
	unsigned short	x,
	char 	*s)

{
	s[2] = returnchar(x % 40);
	x /= 40;
	s[1] = returnchar(x % 40);
	x /= 40;
	s[0] = returnchar(x % 40);
}

/******************************  returnchar  *******************************/


static int returnchar(   	/* return a character according to RAD50 coding */
			/* scheme, called by derad50 */
	unsigned short	k)
{
	if (k >= 1 && k <= 26)
		/* k represents a letter */
		return('a' + k - 1);

	else if (k >= 30 && k <= 39)
		/* k represents a digit */
		return('0' + k - 30);

	else switch (k)
	{
		case 0:
			return(' ');

		case 27:
			return('$');

		case 28:
			return('.');

		case 29:
			return ('_');

		default:
			lerror("RAD50 non-character");
			return 0; /* NOTREACHED */
	}

}


