#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <histedit.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "fns.h"

#define GREPCMD "grep -F -f "
#define TTYDEVP "/dev/tty"
#define DEFHSIZ 128

static char *cmdbuf;
static char *histfp;
static char *prompt;

static int wflag = 0;
static int fdtemp = -1;
static char fntemp[] = "/tmp/inputXXXXXX";
static int signals[] = {SIGINT, SIGTERM, SIGQUIT, SIGHUP};

static EditLine *el;
static History *hist;
static HistEvent ev;

static void
usage(char *prog)
{
	char *usage = "[-w] [-c COMMAND] [-p PROMPT] "
	              "[-h HISTORY] [-s HISTSIZE]";

	fprintf(stderr, "USAGE: %s %s\n", basename(prog), usage);
	exit(EXIT_FAILURE);
}

static void
cleanup(void)
{
	if (!el)
		return;

	el_end(el);
	el = NULL;

	if (hist) {
		if (history(hist, &ev, H_SAVE, histfp) == -1)
			warnx("couldn't save history to '%s'", histfp);
		history_end(hist);
	}

	if (fdtemp > 0) {
		if (close(fdtemp) == -1)
			warn("close failed for '%s'", fntemp);
		if (remove(fntemp) == -1)
			warn("couldn't remove file '%s'", fntemp);
	}
}

static void
sighandler(int num)
{
	(void)num;

	cleanup();
	exit(EXIT_FAILURE);
}

static void
sethandler(void)
{
	size_t i;
	struct sigaction act;

	act.sa_flags = 0;
	act.sa_handler = sighandler;
	if (sigemptyset(&act.sa_mask) == -1)
		err(EXIT_FAILURE, "sigemptyset failed");

	for (i = 0; i < (sizeof(signals) / sizeof(signals[0])); i++) {
		if (sigaction(signals[i], &act, NULL))
			err(EXIT_FAILURE, "sigaction failed");
	}
}

static FILE *
fout(void)
{
	FILE *out;

	out = stdout;
	if (isatty(fileno(out)))
		return out;

	if (!(out = fopen(TTYDEVP, "w")))
		err(EXIT_FAILURE, "fopen failed");
	return out;
}

static char *
safegrep(const char *pattern, size_t len)
{
	if (ftruncate(fdtemp, 0) == -1)
		err(EXIT_FAILURE, "ftruncate failed");
	if (lseek(fdtemp, SEEK_SET, 0) == -1)
		err(EXIT_FAILURE, "lseek failed");

	if (write(fdtemp, pattern, len) == -1 ||
	    write(fdtemp, "\n", 1) == -1)
		err(EXIT_FAILURE, "write failed");

	return cmdbuf;
}

static void
comp(const char *input, size_t inlen)
{
	FILE *pipe;
	char *p, *cmd;
	static char *line;
	static size_t llen;

	cmd = safegrep(input, inlen);
	if (!(pipe = popen(cmd, "r")))
		err(EXIT_FAILURE, "popen failed");

	while (getline(&line, &llen, pipe) != -1) {
		if (strncmp(input, line, inlen))
			continue;
		if ((p = strchr(line, '\n')))
			*p = '\0';
		addcomp(line);
	}
	if (ferror(pipe))
		errx(EXIT_FAILURE, "ferror failed");

	if (pclose(pipe) == -1)
		errx(EXIT_FAILURE, "pclose failed");
}

static void
iloop(void)
{
	int num;
	const char *line;

	while ((line = el_gets(el, &num)) && num >= 0) {
		/* We output empty lines intentionally. */
		printf("%s", line);
		fflush(stdout);

		if (!hist || !strcmp(line, "\n"))
			continue;

		if (history(hist, &ev, H_ENTER, line) == -1)
			warnx("couldn't add input to history");
	}

	if (num == -1)
		err(EXIT_FAILURE, "el_gets failed");
}

static void
confhist(char *fp, int size)
{
	int hsiz;
	char *errmsg;

	hist = history_init();

	hsiz = size ? size : DEFHSIZ;
	if (history(hist, &ev, H_SETSIZE, hsiz) == -1) {
		errmsg = "couldn't set history size";
		goto err;
	}

	el_set(el, EL_HIST, history, hist);
	if (!access(fp, F_OK) && history(hist, &ev, H_LOAD, fp) == -1) {
		errmsg = "couldn't load history file";
		goto err;
	}

	return;
err:
	/* don't overwrite history file in cleanup() */
	history_end(hist);
	hist = NULL;

	errx(EXIT_FAILURE, "%s", errmsg);
}

static void
confcomp(char *compcmd)
{
	int ret;
	size_t cmdlen;

	if (!(fdtemp = mkstemp(fntemp)))
		err(EXIT_FAILURE, "mkstemp failed");
	if (fchmod(fdtemp, 0600) == -1) /* not manadated by POSIX */
		err(EXIT_FAILURE, "fchmod failed");

	/* + 2 for the null byte and the pipe character. */
	cmdlen = 2 + strlen(GREPCMD) + strlen(fntemp) + strlen(compcmd);
	if (!(cmdbuf = malloc(cmdlen)))
		err(EXIT_FAILURE, "malloc failed");

	ret = snprintf(cmdbuf, cmdlen, "%s|" GREPCMD "%s", compcmd, fntemp);
	if (ret < 0)
		err(EXIT_FAILURE, "snprintf failed");
	else if ((size_t)ret >= cmdlen)
		errx(EXIT_FAILURE, "buffer for snprintf is too short");

	initcomp(comp, wflag);
	el_set(el, EL_ADDFN, "complete", "Complete input", complete);
	el_set(el, EL_BIND, "^I", "complete", NULL);
}

static char *
promptfn(EditLine *e)
{
	(void)e;
	return prompt;
}

int
main(int argc, char **argv)
{
	int opt, hsiz;
	char *compcmd;

	hsiz = 0;
	prompt = "> ";
	compcmd = NULL;

	while ((opt = getopt(argc, argv, "wc:p:h:s:")) != -1) {
		switch (opt) {
		case 'w':
			wflag = 1;
			break;
		case 'c':
			compcmd = optarg;
			break;
		case 'p':
			prompt = optarg;
			break;
		case 'h':
			histfp = optarg;
			break;
		case 's':
			if (!(hsiz = strtol(optarg, (char **)NULL, 10)))
				err(EXIT_FAILURE, "strtol failed");
			break;
		default:
			usage(*argv);
		}
	}

	setlocale(LC_CTYPE, "");
	if (!(el = el_init(*argv, stdin, fout(), stderr)))
		errx(EXIT_FAILURE, "el_init failed");

	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_PROMPT, promptfn);
	el_set(el, EL_SIGNAL, 1);
	el_source(el, NULL); /* source user defaults */

	sethandler();
	if (atexit(cleanup))
		err(EXIT_FAILURE, "atexit failed");

	if (histfp)
		confhist(histfp, hsiz);
	if (compcmd)
		confcomp(compcmd);

	iloop();
	return EXIT_SUCCESS;
}
