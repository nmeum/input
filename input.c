#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <linenoise.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/types.h>

#define DEFHSIZ 256
#define MAXCOMPLEN 512

static char *histfp;
static char *compfp;

static void
usage(char *prog)
{
	char *usage = "USAGE: %s [-t] [-c COMPLETION] [-p PROMPT] "
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
	savehist();
}

static void
comp(const char *buf, linenoiseCompletions *lc)
{
	FILE *pipe;
	size_t clen;
	char *cmd, *p;
	static char line[LINE_MAX + 1];

	if (strchr(buf, '\'')) return; /* TODO */

	clen = 1 + strlen("grep '^' ''") + strlen(buf) + strlen(compfp);
	if (!(cmd = malloc(clen)))
		err(EXIT_FAILURE, "malloc failed");
	if (snprintf(cmd, clen, "grep '^%s' '%s'", buf, compfp) < 0)
		err(EXIT_FAILURE, "snprintf failed");

	pipe = popen(cmd, "r");
	if (!pipe)
		err(EXIT_FAILURE, "popen failed");

	while (fgets(line, sizeof(line), pipe)) {
		if ((p = strchr(line, '\n')))
			*p = '\0';
		linenoiseAddCompletion(lc, line);
	}
	if (ferror(pipe))
		errx(EXIT_FAILURE, "ferror failed");

	if (pclose(pipe))
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
	int opt, hsiz, ftty;
	struct sigaction act;
	char *prompt;

	ftty = hsiz = 0;
	prompt = "> ";

	while ((opt = getopt(argc, argv, "c:p:h:s:t")) != -1) {
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
		case 't':
			ftty = 1;
			break;
		default:
			usage(*argv);
		}
	}

	if (ftty && !isatty(STDIN_FILENO)) {
		close(STDIN_FILENO);
		if (open("/dev/tty", O_RDWR) == -1)
			err(EXIT_FAILURE, "open failed");
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

	if (compfp)
		linenoiseSetCompletionCallback(comp);
	iloop(prompt);

	savehist();
	return EXIT_SUCCESS;
}
