#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "linenoise.h"
#include "utf8.h"

#define GREPCMD "grep -F -f "
#define DEFHSIZ 256

static char *histfp;
static char *cmdbuf;

static int fdtemp = -1;
static char fntemp[] = "/tmp/inputXXXXXX";

static void
usage(char *prog)
{
	char *usage = "[-c COMMAND] [-p PROMPT] "
	              "[-h HISTORY] [-s HISTSIZE]";

	fprintf(stderr, "USAGE: %s %s\n", basename(prog), usage);
	exit(EXIT_FAILURE);
}

static void
cleanup(void)
{
	if (histfp && linenoiseHistorySave(histfp) == -1)
		warnx("couldn't save history to '%s'", histfp);

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
}

static char *
safegrep(const char *pattern)
{
	if (ftruncate(fdtemp, 0) == -1)
		err(EXIT_FAILURE, "ftruncate failed");
	if (write(fdtemp, pattern, strlen(pattern)) == -1 ||
	    write(fdtemp, "\n", 1) == -1)
		err(EXIT_FAILURE, "write failed");

	return cmdbuf;
}

static void
comp(const char *buf, linenoiseCompletions *lc)
{
	FILE *pipe;
	size_t buflen;
	char *p, *cmd;
	static char line[LINE_MAX + 1];

	cmd = safegrep(buf);
	if (!(pipe = popen(cmd, "r")))
		err(EXIT_FAILURE, "popen failed");

	buflen = strlen(buf);
	while (fgets(line, sizeof(line), pipe)) {
		if (strncmp(buf, line, buflen))
			continue;
		if ((p = strchr(line, '\n')))
			*p = '\0';
		linenoiseAddCompletion(lc, line);
	}
	if (ferror(pipe))
		errx(EXIT_FAILURE, "ferror failed");

	if (pclose(pipe) == -1)
		errx(EXIT_FAILURE, "pclose failed");
}

static void
iloop(char *prompt)
{
	char *line;

	while ((line = linenoise(prompt)) != NULL) {
		/* We output empty lines intentionally. */
		puts(line);
		fflush(stdout);

		if (*line != '\0' && histfp)
			linenoiseHistoryAdd(line);
		free(line);
	}
}

static void
confhist(char *fp, int size)
{
	struct sigaction act;

	if (linenoiseHistoryLoad(fp) == -1 && errno != ENOENT)
		err(EXIT_FAILURE, "couldn't load '%s'", fp);
	if (!linenoiseHistorySetMaxLen(size ? size : DEFHSIZ))
		err(EXIT_FAILURE, "couldn't set history size");

	act.sa_handler = sighandler;
	if (sigemptyset(&act.sa_mask) == -1)
		err(EXIT_FAILURE, "sigemptyset failed");
	if (sigaction(SIGINT, &act, NULL))
		err(EXIT_FAILURE, "sigaction failed");
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

	linenoiseSetCompletionCallback(comp);
}

int
main(int argc, char **argv)
{
	int opt, hsiz;
	char *prompt, *compcmd;

	hsiz = 0;
	prompt = "> ";
	compcmd = NULL;

	while ((opt = getopt(argc, argv, "c:p:h:s:")) != -1) {
		switch (opt) {
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
			if (!(hsiz = (int)strtol(optarg, (char **)NULL, 10)))
				err(EXIT_FAILURE, "strtol failed");
			break;
		default:
			usage(*argv);
		}
	}

	if (atexit(cleanup))
		err(EXIT_FAILURE, "atexit failed");

	if (histfp)
		confhist(histfp, hsiz);
	if (compcmd)
		confcomp(compcmd);

	linenoiseSetEncodingFunctions(
	    linenoiseUtf8PrevCharLen,
	    linenoiseUtf8NextCharLen,
	    linenoiseUtf8ReadCode);

	iloop(prompt);
	return EXIT_SUCCESS;
}
