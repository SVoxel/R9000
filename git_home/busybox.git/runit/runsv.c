/* Busyboxed by Denis Vlasenko <vda.linux@googlemail.com> */
/* TODO: depends on runit_lib.c - review and reduce/eliminate */

#include <sys/poll.h>
#include <sys/file.h>
#include "busybox.h"
#include "runit_lib.h"

static int selfpipe[2];

/* state */
#define S_DOWN 0
#define S_RUN 1
#define S_FINISH 2
/* ctrl */
#define C_NOOP 0
#define C_TERM 1
#define C_PAUSE 2
/* want */
#define W_UP 0
#define W_DOWN 1
#define W_EXIT 2

struct svdir {
	int pid;
	int state;
	int ctrl;
	int want;
	struct taia start;
	int fdlock;
	int fdcontrol;
	int fdcontrolwrite;
	int islog;
};
static struct svdir svd[2];

static int sigterm = 0;
static int haslog = 0;
static int pidchanged = 1;
static int logpipe[2];
static char *dir;

#define usage() bb_show_usage()

static void fatal2_cannot(char *m1, char *m2)
{
	bb_perror_msg_and_die("%s: fatal: cannot %s%s", dir, m1, m2);
	/* was exiting 111 */
}
static void fatal_cannot(char *m)
{
	fatal2_cannot(m, "");
	/* was exiting 111 */
}
static void fatal2x_cannot(char *m1, char *m2)
{
	bb_error_msg_and_die("%s: fatal: cannot %s%s", dir, m1, m2);
	/* was exiting 111 */
}
static void warn_cannot(char *m)
{
	bb_perror_msg("%s: warning: cannot %s", dir, m);
}
static void warnx_cannot(char *m)
{
	bb_error_msg("%s: warning: cannot %s", dir, m);
}

static void stopservice(struct svdir *);

static void s_child(int sig_no)
{
	write(selfpipe[1], "", 1);
}

static void s_term(int sig_no)
{
	sigterm = 1;
	write(selfpipe[1], "", 1); /* XXX */
}

static char *add_str(char *p, const char *to_add)
{
	while ((*p = *to_add) != '\0') {
		p++;
		to_add++;
	}
	return p;
}

static int open_trunc_or_warn(const char *name)
{
	int fd = open_trunc(name);
	if (fd < 0)
		bb_perror_msg("%s: warning: cannot open %s",
				dir, name);
	return fd;
}

static int rename_or_warn(const char *old, const char *new)
{
	if (rename(old, new) == -1) {
		bb_perror_msg("%s: warning: cannot rename %s to %s",
				dir, old, new);
		return -1;
	}
	return 0;
}

static void update_status(struct svdir *s)
{
	unsigned long l;
	int fd;
	char status[20];

	/* pid */
	if (pidchanged) {
		fd = open_trunc_or_warn("supervise/pid.new");
		if (fd < 0)
			return;
		if (s->pid) {
			char spid[sizeof(s->pid)*3 + 2];
			int size = sprintf(spid, "%d\n", s->pid);
			write(fd, spid, size);
		}
		close(fd);
		if (s->islog) {
			if (rename_or_warn("supervise/pid.new", "log/supervise/pid"))
				return;
		} else if (rename_or_warn("supervise/pid.new", "supervise/pid")) {
			return;
		}
		pidchanged = 0;
	}

	/* stat */
	fd = open_trunc_or_warn("supervise/stat.new");
	if (fd < -1)
		return;

	{
		char stat_buf[sizeof("finish, paused, got TERM, want down\n")];
		char *p = stat_buf;
		switch (s->state) {
		case S_DOWN:
			p = add_str(p, "down");
			break;
		case S_RUN:
			p = add_str(p, "run");
			break;
		case S_FINISH:
			p = add_str(p, "finish");
			break;
		}
		if (s->ctrl & C_PAUSE) p = add_str(p, ", paused");
		if (s->ctrl & C_TERM) p = add_str(p, ", got TERM");
		if (s->state != S_DOWN)
			switch (s->want) {
			case W_DOWN:
				p = add_str(p, ", want down");
				break;
			case W_EXIT:
				p = add_str(p, ", want exit");
				break;
			}
		*p++ = '\n';
		write(fd, stat_buf, p - stat_buf);
		close(fd);
	}

	if (s->islog) {
		rename_or_warn("supervise/stat.new", "log/supervise/stat");
	} else {
		rename_or_warn("supervise/stat.new", "log/supervise/stat"+4);
	}

	/* supervise compatibility */
	taia_pack(status, &s->start);
	l = (unsigned long)s->pid;
	status[12] = l; l >>=8;
	status[13] = l; l >>=8;
	status[14] = l; l >>=8;
	status[15] = l;
	if (s->ctrl & C_PAUSE)
		status[16] = 1;
	else
		status[16] = 0;
	if (s->want == W_UP)
		status[17] = 'u';
	else
		status[17] = 'd';
	if (s->ctrl & C_TERM)
		status[18] = 1;
	else
		status[18] = 0;
	status[19] = s->state;
	fd = open_trunc_or_warn("supervise/status.new");
	if (fd < 0)
		return;
	l = write(fd, status, sizeof status);
	if (l < 0) {
		warn_cannot("write supervise/status.new");
		close(fd);
		unlink("supervise/status.new");
		return;
	}
	close(fd);
	if (l < sizeof status) {
		warnx_cannot("write supervise/status.new: partial write");
		return;
	}
	if (s->islog) {
		rename_or_warn("supervise/status.new", "log/supervise/status");
	} else {
		rename_or_warn("supervise/status.new", "log/supervise/status"+4);
	}
}

static unsigned custom(struct svdir *s, char c)
{
	int pid;
	int w;
	char a[10];
	struct stat st;
	char *prog[2];

	if (s->islog) return 0;
	memcpy(a, "control/?", 10);
	a[8] = c;
	if (stat(a, &st) == 0) {
		if (st.st_mode & S_IXUSR) {
			pid = fork();
			if (pid == -1) {
				warn_cannot("fork for control/?");
				return 0;
			}
			if (!pid) {
				if (haslog && fd_copy(1, logpipe[1]) == -1)
					warn_cannot("setup stdout for control/?");
				prog[0] = a;
				prog[1] = 0;
				execve(a, prog, environ);
				fatal_cannot("run control/?");
			}
			while (wait_pid(&w, pid) == -1) {
				if (errno == EINTR) continue;
				warn_cannot("wait for child control/?");
				return 0;
			}
			return !wait_exitcode(w);
		}
	}
	else {
		if (errno == ENOENT) return 0;
		warn_cannot("stat control/?");
	}
	return 0;
}

static void stopservice(struct svdir *s)
{
	if (s->pid && ! custom(s, 't')) {
		kill(s->pid, SIGTERM);
		s->ctrl |=C_TERM;
		update_status(s);
	}
	if (s->want == W_DOWN) {
		kill(s->pid, SIGCONT);
		custom(s, 'd'); return;
	}
	if (s->want == W_EXIT) {
		kill(s->pid, SIGCONT);
		custom(s, 'x');
	}
}

static void startservice(struct svdir *s)
{
	int p;
	char *run[2];

	if (s->state == S_FINISH)
		run[0] = "./finish";
	else {
		run[0] = "./run";
		custom(s, 'u');
	}
	run[1] = 0;

	if (s->pid != 0) stopservice(s); /* should never happen */
	while ((p = fork()) == -1) {
		warn_cannot("fork, sleeping");
		sleep(5);
	}
	if (p == 0) {
		/* child */
		if (haslog) {
			if (s->islog) {
				if (fd_copy(0, logpipe[0]) == -1)
					fatal_cannot("setup filedescriptor for ./log/run");
				close(logpipe[1]);
				if (chdir("./log") == -1)
					fatal_cannot("change directory to ./log");
			} else {
				if (fd_copy(1, logpipe[1]) == -1)
					fatal_cannot("setup filedescriptor for ./run");
				close(logpipe[0]);
			}
		}
		sig_uncatch(sig_child);
		sig_unblock(sig_child);
		sig_uncatch(sig_term);
		sig_unblock(sig_term);
		execve(*run, run, environ);
		if (s->islog)
			fatal2_cannot("start log/", *run);
		else
			fatal2_cannot("start ", *run);
	}
	if (s->state != S_FINISH) {
		taia_now(&s->start);
		s->state = S_RUN;
	}
	s->pid = p;
	pidchanged = 1;
	s->ctrl = C_NOOP;
	update_status(s);
}

static int ctrl(struct svdir *s, char c)
{
	switch (c) {
	case 'd': /* down */
		s->want = W_DOWN;
		update_status(s);
		if (s->pid && s->state != S_FINISH) stopservice(s);
		break;
	case 'u': /* up */
		s->want = W_UP;
		update_status(s);
		if (s->pid == 0) startservice(s);
		break;
	case 'x': /* exit */
		if (s->islog) break;
		s->want = W_EXIT;
		update_status(s);
		if (s->pid && s->state != S_FINISH) stopservice(s);
		break;
	case 't': /* sig term */
		if (s->pid && s->state != S_FINISH) stopservice(s);
		break;
	case 'k': /* sig kill */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGKILL);
		s->state = S_DOWN;
		break;
	case 'p': /* sig pause */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGSTOP);
		s->ctrl |=C_PAUSE;
		update_status(s);
		break;
	case 'c': /* sig cont */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGCONT);
		if (s->ctrl & C_PAUSE) s->ctrl &=~C_PAUSE;
		update_status(s);
		break;
	case 'o': /* once */
		s->want = W_DOWN;
		update_status(s);
		if (!s->pid) startservice(s);
		break;
	case 'a': /* sig alarm */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGALRM);
		break;
	case 'h': /* sig hup */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGHUP);
		break;
	case 'i': /* sig int */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGINT);
		break;
	case 'q': /* sig quit */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGQUIT);
		break;
	case '1': /* sig usr1 */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGUSR1);
		break;
	case '2': /* sig usr2 */
		if (s->pid && ! custom(s, c)) kill(s->pid, SIGUSR2);
		break;
	}
	return 1;
}

int runsv_main(int argc, char **argv)
{
	struct stat s;
	int fd;
	int r;
	char buf[256];

	if (!argv[1] || argv[2]) usage();
	dir = argv[1];

	if (pipe(selfpipe) == -1) fatal_cannot("create selfpipe");
	coe(selfpipe[0]);
	coe(selfpipe[1]);
	ndelay_on(selfpipe[0]);
	ndelay_on(selfpipe[1]);

	sig_block(sig_child);
	sig_catch(sig_child, s_child);
	sig_block(sig_term);
	sig_catch(sig_term, s_term);

	xchdir(dir);
	svd[0].pid = 0;
	svd[0].state = S_DOWN;
	svd[0].ctrl = C_NOOP;
	svd[0].want = W_UP;
	svd[0].islog = 0;
	svd[1].pid = 0;
	taia_now(&svd[0].start);
	if (stat("down", &s) != -1) svd[0].want = W_DOWN;

	if (stat("log", &s) == -1) {
		if (errno != ENOENT)
			warn_cannot("stat ./log");
	} else {
		if (!S_ISDIR(s.st_mode))
			warnx_cannot("stat log/down: log is not a directory");
		else {
			haslog = 1;
			svd[1].state = S_DOWN;
			svd[1].ctrl = C_NOOP;
			svd[1].want = W_UP;
			svd[1].islog = 1;
			taia_now(&svd[1].start);
			if (stat("log/down", &s) != -1)
				svd[1].want = W_DOWN;
			if (pipe(logpipe) == -1)
				fatal_cannot("create log pipe");
			coe(logpipe[0]);
			coe(logpipe[1]);
		}
	}

	if (mkdir("supervise", 0700) == -1) {
		r = readlink("supervise", buf, 256);
		if (r != -1) {
			if (r == 256)
				fatal2x_cannot("readlink ./supervise: ", "name too long");
			buf[r] = 0;
			mkdir(buf, 0700);
		} else {
			if ((errno != ENOENT) && (errno != EINVAL))
				fatal_cannot("readlink ./supervise");
		}
	}
	svd[0].fdlock = xopen3("log/supervise/lock"+4,
			O_WRONLY|O_NDELAY|O_APPEND|O_CREAT, 0600);
	if (lock_exnb(svd[0].fdlock) == -1)
		fatal_cannot("lock supervise/lock");
	coe(svd[0].fdlock);
	if (haslog) {
		if (mkdir("log/supervise", 0700) == -1) {
			r = readlink("log/supervise", buf, 256);
			if (r != -1) {
				if (r == 256)
					fatal2x_cannot("readlink ./log/supervise: ", "name too long");
				buf[r] = 0;
				fd = xopen(".", O_RDONLY|O_NDELAY);
				xchdir("./log");
				mkdir(buf, 0700);
				if (fchdir(fd) == -1)
					fatal_cannot("change back to service directory");
				close(fd);
			}
			else {
				if ((errno != ENOENT) && (errno != EINVAL))
					fatal_cannot("readlink ./log/supervise");
			}
		}
		svd[1].fdlock = xopen3("log/supervise/lock",
				O_WRONLY|O_NDELAY|O_APPEND|O_CREAT, 0600);
		if (lock_ex(svd[1].fdlock) == -1)
			fatal_cannot("lock log/supervise/lock");
		coe(svd[1].fdlock);
	}

	fifo_make("log/supervise/control"+4, 0600);
	svd[0].fdcontrol = xopen("log/supervise/control"+4, O_RDONLY|O_NDELAY);
	coe(svd[0].fdcontrol);
	svd[0].fdcontrolwrite = xopen("log/supervise/control"+4, O_WRONLY|O_NDELAY);
	coe(svd[0].fdcontrolwrite);
	update_status(&svd[0]);
	if (haslog) {
		fifo_make("log/supervise/control", 0600);
		svd[1].fdcontrol = xopen("log/supervise/control", O_RDONLY|O_NDELAY);
		coe(svd[1].fdcontrol);
		svd[1].fdcontrolwrite = xopen("log/supervise/control", O_WRONLY|O_NDELAY);
		coe(svd[1].fdcontrolwrite);
		update_status(&svd[1]);
	}
	fifo_make("log/supervise/ok"+4, 0600);
	fd = xopen("log/supervise/ok"+4, O_RDONLY|O_NDELAY);
	coe(fd);
	if (haslog) {
		fifo_make("log/supervise/ok", 0600);
		fd = xopen("log/supervise/ok", O_RDONLY|O_NDELAY);
		coe(fd);
	}
	for (;;) {
		iopause_fd x[3];
		struct taia deadline;
		struct taia now;
		char ch;

		if (haslog)
			if (!svd[1].pid && svd[1].want == W_UP)
				startservice(&svd[1]);
		if (!svd[0].pid)
			if (svd[0].want == W_UP || svd[0].state == S_FINISH)
				startservice(&svd[0]);

		x[0].fd = selfpipe[0];
		x[0].events = IOPAUSE_READ;
		x[1].fd = svd[0].fdcontrol;
		x[1].events = IOPAUSE_READ;
		if (haslog) {
			x[2].fd = svd[1].fdcontrol;
			x[2].events = IOPAUSE_READ;
		}
		taia_now(&now);
		taia_uint(&deadline, 3600);
		taia_add(&deadline, &now, &deadline);

		sig_unblock(sig_term);
		sig_unblock(sig_child);
		iopause(x, 2+haslog, &deadline, &now);
		sig_block(sig_term);
		sig_block(sig_child);

		while (read(selfpipe[0], &ch, 1) == 1)
			;
		for (;;) {
			int child;
			int wstat;

			child = wait_nohang(&wstat);
			if (!child) break;
			if ((child == -1) && (errno != EINTR)) break;
			if (child == svd[0].pid) {
				svd[0].pid = 0;
				pidchanged = 1;
				svd[0].ctrl &=~C_TERM;
				if (svd[0].state != S_FINISH)
					fd = open_read("finish");
					if (fd != -1) {
						close(fd);
						svd[0].state = S_FINISH;
						update_status(&svd[0]);
						continue;
					}
				svd[0].state = S_DOWN;
				taia_uint(&deadline, 1);
				taia_add(&deadline, &svd[0].start, &deadline);
				taia_now(&svd[0].start);
				update_status(&svd[0]);
				if (taia_less(&svd[0].start, &deadline)) sleep(1);
			}
			if (haslog) {
				if (child == svd[1].pid) {
					svd[1].pid = 0;
					pidchanged = 1;
					svd[1].state = S_DOWN;
					svd[1].ctrl &=~C_TERM;
					taia_uint(&deadline, 1);
					taia_add(&deadline, &svd[1].start, &deadline);
					taia_now(&svd[1].start);
					update_status(&svd[1]);
					if (taia_less(&svd[1].start, &deadline)) sleep(1);
				}
			}
		}
		if (read(svd[0].fdcontrol, &ch, 1) == 1)
			ctrl(&svd[0], ch);
		if (haslog)
			if (read(svd[1].fdcontrol, &ch, 1) == 1)
				ctrl(&svd[1], ch);

		if (sigterm) {
			ctrl(&svd[0], 'x');
			sigterm = 0;
		}

		if (svd[0].want == W_EXIT && svd[0].state == S_DOWN) {
			if (svd[1].pid == 0)
				_exit(0);
			if (svd[1].want != W_EXIT) {
				svd[1].want = W_EXIT;
				/* stopservice(&svd[1]); */
				update_status(&svd[1]);
				close(logpipe[1]);
				close(logpipe[0]);
				//if (close(logpipe[1]) == -1)
				//	warn_cannot("close logpipe[1]");
				//if (close(logpipe[0]) == -1)
				//	warn_cannot("close logpipe[0]");
			}
		}
	}
	/* not reached */
	return 0;
}
