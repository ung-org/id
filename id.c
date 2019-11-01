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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FULL	0
#define NAMES	1
#define NUMS	2

void id_printgids(char *user, int mode)
{
	struct group *gr = NULL;
	struct group *current = NULL;
	int num_groups = 0;
	char sep = ' ';
	int i = 0;

	setgrent();
	while ((current = getgrent()) != NULL) {
		for (i = 0; current->gr_mem[i] != NULL; i++) {
			if (!strcmp(user, current->gr_mem[i])) {
				num_groups++;
				gr = realloc(gr,
					     num_groups * sizeof(struct group));
				gr[num_groups - 1].gr_gid = current->gr_gid;
				gr[num_groups - 1].gr_name =
				    strdup(current->gr_name);
			}
		}
	}
	endgrent();

	if (mode == FULL && num_groups > 0) {
		printf(" groups=");
		sep = ',';
	}

	while (i < num_groups) {
		if (i > 0) {
			putchar(sep);
		}

		switch (mode) {
		case NAMES:
			printf("%s", gr[i].gr_name);
			break;

		case NUMS:
			printf("%u", gr[i].gr_gid);
			break;

		default:
			printf("%u(%s)", gr[i].gr_gid, gr[i].gr_name);
			break;

		}

		free(gr[i].gr_name);
		i++;
	}

	free(gr);
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
			fprintf(stderr, "id: user '%s' not found\n", argv[optind]);
			return 1;
		}
		grp = getgrgid(pwd->pw_gid);
	}

	switch (mode) {
	case 'G':
		id_printgids(pwd->pw_name, names ? NAMES : NUMS);
		break;

	case 'g':
		if (names) {
			printf("%s", grp->gr_name);
		} else {
			printf("%u", grp->gr_gid);
		}
		break;

	case 'u':
		if (names) {
			printf("%s", pwd->pw_name);
		} else {
			printf("%u", pwd->pw_uid);
		}
		break;

	default:
		printf("uid=%u(%s) gid=%u(%s)", pwd->pw_uid, pwd->pw_name,
		       grp->gr_gid, grp->gr_name);

		if (pwd->pw_uid != geteuid()) {
			pwd = getpwuid(geteuid());
			printf(" euid=%u(%s)", pwd->pw_uid, pwd->pw_name);
		}

		if (grp->gr_gid != getegid()) {
			grp = getgrgid(getegid());
			printf(" egid=%u(%s)", grp->gr_gid, grp->gr_name);
		}

		id_printgids(pwd->pw_name, FULL);
	}

	printf("\n");
	return 0;
}
