#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <linenoise.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#define DEFHSIZ 256

static char *histfp;
static char *compfp;

static int fdtemp = -1;
static char fntemp[] = "/tmp/inputXXXXXX";

static void
usage(char *prog)
{
	char *usage = "USAGE: %s [-c COMPLETION] [-p PROMPT] "
	              "[-h HISTORY] [-s HISTSIZE]\n";

	fprintf(stderr, usage, basename(prog));
	exit(EXIT_FAILURE);
}

static void
savehist(void)
{
	if (histfp && linenoiseHistorySave(histfp) == -1)
		err(EXIT_FAILURE, "couldn't save history to '%s'", histfp);
}

static void
sighandler(int num)
{
	(void)num;

	if (fdtemp > 0)
		remove(fntemp);
	savehist();
}

static char *
safegrep(const char *pattern)
{
	char *cmd;
	size_t len;

	/* TODO: grep -F */

	if (ftruncate(fdtemp, 0) == -1)
		err(EXIT_FAILURE, "ftruncate failed");
	if (write(fdtemp, pattern, strlen(pattern)) == -1 ||
	    write(fdtemp, "\n", 1) == -1)
		err(EXIT_FAILURE, "write failed");

	len = 1 + strlen("grep -f '' ''") + strlen(fntemp) + strlen(compfp);
	if (!(cmd = malloc(len)))
		err(EXIT_FAILURE, "malloc failed");
	if (snprintf(cmd, len, "grep -f '%s' '%s'", fntemp, compfp) < 0)
		err(EXIT_FAILURE, "snprintf failed");

	return cmd;
}

static void
comp(const char *buf, linenoiseCompletions *lc)
{
	char *p, *cmd;
	FILE *pipe;
	static char line[LINE_MAX + 1];

	cmd = safegrep(buf);
	if (!(pipe = popen(cmd, "r")))
		err(EXIT_FAILURE, "popen failed");

	while (fgets(line, sizeof(line), pipe)) {
		if ((p = strchr(line, '\n')))
			*p = '\0';
		linenoiseAddCompletion(lc, line);
	}
	if (ferror(pipe))
		errx(EXIT_FAILURE, "ferror failed");

	if (pclose(pipe) == -1)
		errx(EXIT_FAILURE, "pclose failed");
	free(cmd);
}

static void
iloop(char *prompt)
{
	char *line;

	while ((line = linenoise(prompt)) != NULL) {
		if (*line != '\0')
			puts(line);
		free(line);
	}
}

int
main(int argc, char **argv)
{
	int opt, hsiz;
	struct sigaction act;
	char *prompt;

	hsiz = 0;
	prompt = "> ";

	while ((opt = getopt(argc, argv, "c:p:h:s:")) != -1) {
		switch (opt) {
		case 'c':
			compfp = optarg;
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

	if (histfp) {
		if (linenoiseHistoryLoad(histfp) == -1)
			err(EXIT_FAILURE, "couldn't load '%s'", histfp);
		if (!linenoiseHistorySetMaxLen(hsiz ? hsiz : DEFHSIZ))
			err(EXIT_FAILURE, "couldn't set history size");

		memset(&act, '\0', sizeof(act));
		act.sa_handler = sighandler;
		if (sigaction(SIGINT, &act, NULL))
			err(EXIT_FAILURE, "sigaction failed");
	}

	if (compfp) {
		if (!(fdtemp = mkstemp(fntemp)))
			err(EXIT_FAILURE, "mkstemp failed");
		linenoiseSetCompletionCallback(comp);
	}

	iloop(prompt);
	savehist();

	/* TODO: always save hist and remove fntemp on exit. */
	if (fdtemp > 0) {
		close(fdtemp);
		remove(fntemp);
	}

	return EXIT_SUCCESS;
}
