#include <assert.h>
#include <err.h>
#include <histedit.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#include "fns.h"

typedef struct _comp comp;

struct _comp {
	char *val;
	comp *nxt;
};

static int wcomp;
static compfn cfn;
static comp *comps;

#define cursoridx(LINEINFO) \
	((LINEINFO->cursor) - (LINEINFO)->buffer)

static size_t
getin(const char **dest, const LineInfo *linfo)
{
	size_t len;
	const char* ws;

	len = linfo->lastchar - linfo->buffer;
	if (!wcomp) {
		*dest = linfo->buffer;
		return len;
	}

	if ((ws = memrchr(linfo->buffer, ' ', len)))
		ws++;
	else
		ws = linfo->buffer;

	*dest = ws;
	return linfo->lastchar - ws;
}

static comp *
nxtcomp(comp *lcomp)
{
	comp *nxt;

	assert(lcomp);

	nxt = lcomp->nxt;
	if (!nxt && comps != lcomp)
		nxt = comps;

	return nxt; /* NULL if there is no new completion */
}

static void
freecomps(void)
{
	comp *c, *n;

	c = comps;
	while (c) {
		n = c->nxt;
		free(c->val);
		free(c);
		c = n;
	}

	comps = NULL;
}

void
addcomp(char *str)
{
	comp *c;

	if (!(c = malloc(sizeof(*c))))
		err(EXIT_FAILURE, "malloc failed");
	if (!(c->val = strdup(str)))
		err(EXIT_FAILURE, "strdup failed");

	if (!comps) {
		comps = c;
		comps->nxt = NULL;
	} else {
		c->nxt = comps->nxt;
		comps->nxt = c;
	}
}

void
initcomp(compfn fn, int wordcomp)
{
	cfn = fn;
	wcomp = wordcomp;
}

unsigned char
complete(EditLine *el, int ch)
{
	comp *c;
	size_t inlen, cidx, cccur;
	const LineInfoW *winfo;
	const LineInfo *linfo;
	const char *input;
	static char *linput;
	static comp *lcomp;
	static size_t bbcur, bccur;

	(void)ch;

	winfo = el_wline(el);
	linfo = el_line(el);
	inlen = getin(&input, linfo);

	if (!linput || inlen != strlen(linput) || strncmp(input, linput, inlen)) {
		freecomps();
		lcomp = NULL;

		cfn(input, inlen);
		if (!comps)
			return CC_ERROR;
		c = comps;

		bbcur = cursoridx(linfo); /* base byte cursor */
		bccur = cursoridx(winfo); /* base char cursor */
	} else {
		cccur = cursoridx(winfo); /* current char cursor */
		assert(cccur >= bccur);
		el_deletestr(el, cccur - bccur);

		if (!(c = nxtcomp(lcomp)))
			return CC_REFRESH; /* deletestr changed line */
	}

	cidx = bbcur;
	if (wcomp) {
		assert(input >= linfo->buffer);
		cidx -= input - linfo->buffer;
	}

	if (cidx >= strlen(c->val))
		goto next;
	if (el_insertstr(el, &c->val[cidx]) == -1)
		errx(EXIT_FAILURE, "el_insertstr failed");

next:
	inlen = getin(&input, el_line(el));

	free(linput);
	if (!(linput = malloc(inlen + 1)))
		err(EXIT_FAILURE, "malloc failed");
	memcpy(linput, input, inlen);
	linput[inlen] = '\0';
	lcomp = c;

	return CC_REFRESH;
}
