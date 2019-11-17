#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <sys/stat.h>
#include <sys/types.h>

#define GREPCMD "grep -F -f "
#define TTYDEVP "/dev/tty"

static char *cmdbuf;
static char *histfp;

static int fdtemp = -1;
static char fntemp[] = "/tmp/inputXXXXXX";
static int signals[] = {SIGINT, SIGTERM, SIGQUIT, SIGHUP};

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
	static short clean;

	if (clean)
		return; /* don't cleanup twice */

	if (histfp) {
		if (write_history(histfp))
			warn("saving history to '%s' failed", histfp);
	}

	if (fdtemp > 0) {
		if (close(fdtemp) == -1)
			warn("close failed for '%s'", fntemp);
		if (remove(fntemp) == -1)
			warn("couldn't remove file '%s'", fntemp);
	}

	clean = 1;
}

static void
onexit(void)
{
	sigset_t blockset;

	/* cleanup is called atexit(3) and from a signal handler, block
	 * all signals here before invoking cleanup to ensure we are not
	 * interrupted by the signal handler during cleanup. */

	if (sigfillset(&blockset) == -1)
		err(EXIT_FAILURE, "sigemptyset failed");
	if (sigprocmask(SIG_BLOCK, &blockset, NULL))
		err(EXIT_FAILURE, "sigprocmask failed");

	cleanup();
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

static char *
gencomp(const char *input, int state)
{
	size_t inlen;
	char *r, *p, *cmd;
	static FILE *pipe;
	static char *line;
	static size_t llen;

	inlen = strlen(input);

	if (!state) {
		cmd = safegrep(input, inlen);
		if (!(pipe = popen(cmd, "r")))
			err(EXIT_FAILURE, "popen failed");
	}

	while (getline(&line, &llen, pipe) != -1) {
		if (strncmp(input, line, inlen))
			continue;
		if ((p = strchr(line, '\n')))
			*p = '\0';

		if (!(r = strdup(line)))
			err(EXIT_FAILURE, "strdup failed");
		return r;
	}
	if (ferror(pipe))
		errx(EXIT_FAILURE, "ferror failed");

	if (pclose(pipe) == -1)
		errx(EXIT_FAILURE, "pclose failed");

	return NULL;
}

static char **
comp(const char *text, int start, int end)
{
	(void)start;
	(void)end;

	/* Prevent readline from performing file completions */
	rl_attempted_completion_over = 1;

	/* Don't append any character to completions */
	rl_completion_append_character = '\0';

	return rl_completion_matches(text, gencomp);
}

static void
iloop(char *prompt)
{
	const char *line;

	while ((line = readline(prompt))) {
		/* We output empty lines intentionally. */
		printf("%s\n", line);
		fflush(stdout);

		if (histfp && *line != '\0')
			add_history(line);
	}
}

static void
confhist(char *fp, int size)
{
	using_history();
	stifle_history(size);

	if (!access(fp, F_OK) && read_history(fp))
		err(EXIT_FAILURE, "read_history failed");
}

static void
confcomp(char *compcmd, int wflag)
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

	rl_basic_word_break_characters = (wflag) ? " " : "";
	rl_attempted_completion_function = comp;
}

int
main(int argc, char **argv)
{
	int opt, hsiz, wflag;
	char *prompt, *compcmd;

	wflag = 0;
	hsiz = 128;
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

	rl_outstream = fout();
	if (histfp)
		confhist(histfp, hsiz);
	if (compcmd)
		confcomp(compcmd, wflag);
	else
		rl_bind_key('\t', rl_insert); /* disable completion */

	/* setup after initialization to prevent history truncation */
	sethandler();
	if (atexit(onexit))
		err(EXIT_FAILURE, "atexit failed");

	iloop(prompt);
	return EXIT_SUCCESS;
}
