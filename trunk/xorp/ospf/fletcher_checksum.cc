/*
** return number modulo 255 most importantly convert negative numbers
** to a ones complement representation.
*/
onecomp(a)
int a;
{
	int res;

	res = a % 255;

	if(res < 0)
		res = 255 + res;

	return res;
}

/*
** generate iso checksums.
*/
isocheck(bufp, len, off, x, y)
register unsigned char bufp[];
register int len;
int off;
int *x;
int *y;
{
        register int c0 = 0, c1 = 0;
	register int i;

	for(i = 0; i < len; i++) {
		c0 = bufp[i] + c0;
		c1 = c1 + c0;
	}
	c0 %= 255;
	c1 %= 255;

	*x = onecomp(-c1 + (len - off) * c0);
	*y = onecomp(c1 - (len - off + 1) * c0);
}
