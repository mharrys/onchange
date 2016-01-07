#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ALIGNED(X) __attribute__ ((aligned(__alignof__(struct X))));
#define STEP (sizeof(struct inotify_event) + event->len)

static int is_dir(const char *path)
{
	struct stat stat_buffer;
	stat(path, &stat_buffer);
	return S_ISDIR(stat_buffer.st_mode);
}

static int is_file(const char *path)
{
	struct stat stat_buffer;
	stat(path, &stat_buffer);
	return S_ISREG(stat_buffer.st_mode);
}

static void empty_stdin()
{
	char buffer;
	while (read(STDIN_FILENO, &buffer, 1) > 0 && buffer != '\n');
}

static int what_changed(int fd)
{
	char buffer[4096] ALIGNED(inotify_event);
	const struct inotify_event *event;
	int changed = 0;
	while (1) {
		const ssize_t len = read(fd, buffer, sizeof(buffer));
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		if (len < 1)
			break;
		for (char *it = buffer; it < buffer + len; it += STEP) {
			event = (const struct inotify_event *) it;
			if (event->mask & IN_MODIFY)
				changed |= IN_MODIFY;
			if (event->mask & IN_CLOSE_WRITE)
				changed |= IN_CLOSE_WRITE;
			if (event->mask & IN_MOVED_TO)
				changed |= IN_MOVED_TO;
			if (event->mask & IN_CREATE)
				changed |= IN_CREATE;
		}
	}
	return changed;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: %s PATH\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int inotify_fd = inotify_init1(IN_NONBLOCK);
	if (inotify_fd == -1) {
		perror("inotify_init1");
		exit(EXIT_FAILURE);
	}

	const int mask = IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE;
	int inotify_wd = inotify_add_watch(inotify_fd, argv[1], mask);
	if (inotify_wd == -1) {
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}

	struct pollfd fds[2];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds[1].fd = inotify_fd;
	fds[1].events = POLLIN;

	int prev_changed = 0, changed = 0;
	while (1) {
		int ready = poll(fds, 2, -1);
		if (ready == -1) {
			perror("poll");
			exit(EXIT_FAILURE);
		} else if (ready > 0) {
			if (fds[0].revents & POLLIN) {
				/* exit when user press the "return key" */
				empty_stdin();
				break;
			}
			if (fds[1].revents & POLLIN)
				changed = what_changed(inotify_fd);
		}

		if (prev_changed & IN_MODIFY && changed & IN_CLOSE_WRITE)
			/* save operation can trigger IN_MODIFY followed by
			 * IN_CLOSE_WRITE, only handle one */
			prev_changed = changed = 0;
		else if (changed > 0) {
			prev_changed = changed;
			changed = 0;
			printf("changed\n");
		}
	}

	close(inotify_fd);

	exit(EXIT_SUCCESS);
}
