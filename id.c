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

static void print_id(const char *prefix, const char *name, id_t id, int mode)
{
	printf("%s", prefix);
	if (mode == NAMES) {
		printf("%s", name);
	} else if (mode == NUMS) {
		printf("%ju", (uintmax_t)id);
	} else {
		printf("%ju(%s)", (uintmax_t)id, name);
	}
}

static void id_printgids(char *user, int mode)
{
	struct group *grp = NULL;
	char *prefix = " groups=";

	setgrent();
	while ((grp = getgrent()) != NULL) {
		for (int i = 0; grp->gr_mem[i] != NULL; i++) {
			if (!strcmp(user, grp->gr_mem[i])) {
				print_id(prefix, grp->gr_name, grp->gr_gid, mode);
				prefix = ",";
			}
		}
	}
	endgrent();
}

int main(int argc, char *argv[])
{
	bool names = false;
	bool real_id = false;
	char mode = 0;
	int c;
	struct passwd *pwd;
	struct group *grp;

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
			real_id = true;
			break;

		default:
			return 1;
		}
	}

	if ((mode == 0 && (names || real_id)) || (mode == 'G' && real_id)) {
		return 1;
	}

	if (optind < argc - 1) {
		fprintf(stderr, "id: too many operands\n");
		return 1;
	}

	if (optind >= argc) {
		pwd = getpwuid(mode == 'u' && !real_id ? geteuid() : getuid());
		grp = getgrgid(real_id ? getgid() : getegid());
	} else {
		pwd = getpwnam(argv[optind]);
		if (pwd == NULL) {
			fprintf(stderr, "id: user '%s' not found\n",
				argv[optind]);
			return 1;
		}
		grp = getgrgid(pwd->pw_gid);
	}

	switch (mode) {
	case 'G':
		id_printgids(pwd->pw_name, names ? NAMES : NUMS);
		break;

	case 'g':
		print_id("", grp->gr_name, grp->gr_gid, names);
		break;

	case 'u':
		print_id("", pwd->pw_name, pwd->pw_uid, names);
		break;

	default:
		print_id("uid=", pwd->pw_name, pwd->pw_uid, 0);

		if (optind >= argc && pwd->pw_uid != geteuid()) {
			pwd = getpwuid(geteuid());
			print_id(" euid=", pwd->pw_name, pwd->pw_uid, 0);
		}

		if (optind >= argc && grp->gr_gid != getegid()) {
			grp = getgrgid(getegid());
			print_id(" egid=", grp->gr_name, grp->gr_gid, 0);
		}

		id_printgids(pwd->pw_name, FULL);
	}

	printf("\n");
	return 0;
}
