/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FULL	0
#define NAMES	1
#define NUMS	2

enum type { USER, GROUP };

static void print_id(const char *prefix, enum type type, id_t id, int mode)
{
	char *name = NULL;

	if (type == GROUP) {
		struct group *grp = getgrgid(id);
		if (grp) {
			name = grp->gr_name;
		}
	} else {
		struct passwd *pwd = getpwuid(id);
		if (pwd) {
			name = pwd->pw_name;
		}
	}

	printf("%s", prefix);
	if (mode == NAMES) {
		if (name) {
			printf("%s", name);
		} else {
			printf("%ju", (uintmax_t)id);
		}
	} else if (mode == NUMS) {
		printf("%ju", (uintmax_t)id);
	} else {
		if (name) {
			printf("%ju(%s)", (uintmax_t)id, name);
		} else {
			printf("%ju", (uintmax_t)id);
		}
	}
}

static void print_groups(uid_t uid, int mode)
{
	struct passwd *pwd = getpwuid(uid);
	if (pwd == NULL) {
		return;
	}

	struct group *grp = NULL;
	char *prefix = " groups=";

	setgrent();
	while ((grp = getgrent()) != NULL) {
		for (int i = 0; grp->gr_mem[i] != NULL; i++) {
			if (!strcmp(pwd->pw_name, grp->gr_mem[i])) {
				print_id(prefix, GROUP, grp->gr_gid, mode);
				prefix = ",";
			}
		}
	}
	endgrent();
}

int main(int argc, char *argv[])
{
	bool names = false;
	char mode = 0;
	int c;
	uid_t uid = geteuid();
	gid_t gid = getegid();

	while ((c = getopt(argc, argv, "Ggunr")) != -1) {
		switch (c) {
		case 'G':
		case 'g':
		case 'u':
			mode = c;
			break;

		case 'n':
			names = true;
			break;

		case 'r':
			uid = getuid();
			gid = getgid();
			break;

		default:
			return 1;
		}
	}

	if ((mode == 0 && (names)) || (mode == 'G')) {
		return 1;
	}

	if (optind < argc - 1) {
		fprintf(stderr, "id: too many operands\n");
		return 1;
	}

	if (optind == argc - 1) {
		struct passwd *pwd = getpwnam(argv[optind]);
		if (pwd == NULL) {
			fprintf(stderr, "id: user '%s' not found\n",
				argv[optind]);
			return 1;
		}
		uid = pwd->pw_uid;
		gid = pwd->pw_gid;
	}

	switch (mode) {
	case 'G':
		print_groups(uid, names ? NAMES : NUMS);
		break;

	case 'g':
		print_id("", GROUP, gid, names);
		break;

	case 'u':
		print_id("", USER, uid, names);
		break;

	default:
		print_id("uid=", USER, uid, 0);
		print_id(" gid=", GROUP, gid, 0);

		if (optind >= argc && uid != geteuid()) {
			print_id(" euid=", USER, geteuid(), 0);
		}

		if (optind >= argc && gid != getegid()) {
			print_id(" egid=", GROUP, getegid(), 0);
		}

		print_groups(uid, FULL);
	}

	printf("\n");
	return 0;
}
