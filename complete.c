#include <assert.h>
#include <err.h>
#include <histedit.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <sys/types.h>

#include "fns.h"

typedef struct _comp comp;

struct _comp {
	wchar_t *val;
	comp *nxt;
};

static int wcomp;
static compfn cfn;
static comp *comps;

#define cursoridx(LINEINFO) \
	((LINEINFO->cursor) - (LINEINFO)->buffer)

static size_t
getin(const wchar_t **dest, const LineInfoW *linfo)
{
	ssize_t n;
	size_t len;
	const wchar_t *ws;

	len = linfo->lastchar - linfo->buffer;
	if (!wcomp) {
		*dest = linfo->buffer;
		return len;
	}

	n = cursoridx(linfo);
	while (n-- && !iswspace(linfo->buffer[n]))
		;
	ws = (n >= 0) ? &linfo->buffer[++n] : linfo->buffer;

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

static void
callcomp(const wchar_t *input, size_t len)
{
	char *mbs;
	size_t mbslen;

	if ((mbslen = wcsnrtombs(NULL, &input, len, 0, NULL)) == (size_t)-1)
		err(EXIT_FAILURE, "wcsnrtombs failed");

	if (!(mbs = calloc(mbslen + 1, sizeof(wchar_t))))
		err(EXIT_FAILURE, "calloc failed");
	if (wcsnrtombs(mbs, &input, len, mbslen + 1, NULL) == (size_t)-1)
		err(EXIT_FAILURE, "wcsnrtombs failed");

	cfn(mbs, mbslen);
}

void
addcomp(char *str)
{
	comp *c;
	size_t mbslen;

	if ((mbslen = mbstowcs(NULL, str, 0)) == (size_t)-1)
		err(EXIT_FAILURE, "mbstowcs failed");

	if (!(c = malloc(sizeof(*c))))
		err(EXIT_FAILURE, "malloc failed");
	if (!(c->val = calloc(mbslen + 1, sizeof(wchar_t))))
		err(EXIT_FAILURE, "calloc failed");

	if (mbstowcs(c->val, str, mbslen + 1) == (size_t)-1)
		err(EXIT_FAILURE, "mbstowcs failed");

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
	size_t inlen, cidx, ccur;
	const LineInfoW *linfo;
	const wchar_t *input;
	static wchar_t *linput;
	static comp *lcomp;
	static size_t bcur;

	(void)ch;

	linfo = el_wline(el);
	inlen = getin(&input, linfo);

	if (!linput || inlen != wcslen(linput) || wcsncmp(input, linput, inlen)) {
		freecomps();
		lcomp = NULL;

		callcomp(input, inlen);
		if (!comps)
			return CC_ERROR;
		c = comps;

		bcur = cursoridx(linfo); /* base char cursor */
	} else {
		ccur = cursoridx(linfo); /* current char cursor */
		assert(ccur >= bcur);
		el_deletestr(el, ccur - bcur);

		if (!(c = nxtcomp(lcomp)))
			return CC_REFRESH; /* deletestr changed line */
	}

	cidx = bcur;
	if (wcomp) {
		assert(input >= linfo->buffer);
		cidx -= input - linfo->buffer;
	}

	if (cidx >= wcslen(c->val))
		goto next;
	if (el_winsertstr(el, &c->val[cidx]) == -1)
		errx(EXIT_FAILURE, "el_winsertstr failed");

next:
	inlen = getin(&input, el_wline(el));

	free(linput);
	if (!(linput = calloc(inlen + 1, sizeof(wchar_t))))
		err(EXIT_FAILURE, "calloc failed");
	memcpy(linput, input, inlen * sizeof(wchar_t));
	lcomp = c;

	return CC_REFRESH;
}
