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
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

enum type { USER, GROUP };
enum mode { DEFAULT, UID, GID, ALL_GID };
enum display { FULL, NAMES, NUMBERS };

static char *get_name(enum type type, id_t id)
{
	if (type == GROUP) {
		struct group *grp = getgrgid(id);
		if (grp) {
			return grp->gr_name;
		}
	} else {
		struct passwd *pwd = getpwuid(id);
		if (pwd) {
			return pwd->pw_name;
		}
	}

	return NULL;
}

static void print_id(const char *name, id_t id, enum display mode)
{
	if (mode == FULL || mode == NUMBERS || name == NULL) {
		printf("%ju", (uintmax_t)id);
	}

	if (name != NULL) {
		if (mode == FULL) {
			printf("(%s)", name);
		} else if (mode == NAMES) {
			printf("%s", name);
		}
	}
}

static void print_groups(uid_t uid, gid_t rgid, gid_t egid, enum display mode)
{
	if (mode == FULL) {
		printf(" groups=");
	}

	print_id(get_name(GROUP, rgid), rgid, mode);

	char separator = (mode == FULL ? ',' : ' ');
	
	if (rgid != egid) {
		putchar(separator);
		print_id(get_name(GROUP, egid), egid, mode);
	}

	char *name = get_name(USER, uid);
	if (name == NULL) {
		return;
	}

	struct group *grp = NULL;
	setgrent();
	while ((grp = getgrent()) != NULL) {
		if (grp->gr_gid == rgid || grp->gr_gid == egid) {
			continue;
		}

		for (int i = 0; grp->gr_mem[i] != NULL; i++) {
			if (!strcmp(name, grp->gr_mem[i])) {
				putchar(separator);
				print_id(grp->gr_name, grp->gr_gid, mode);
			}
		}
	}
	endgrent();
}

int main(int argc, char *argv[])
{
	enum display dmode = NUMBERS;
	enum mode mode = DEFAULT;
	uid_t ruid = getuid();
	uid_t euid = geteuid();
	gid_t rgid = getgid();
	gid_t egid = getegid();
	uid_t uid = euid;
	gid_t gid = egid;

	setlocale(LC_ALL, "");

	int c;
	while ((c = getopt(argc, argv, "Ggunr")) != -1) {
		switch (c) {
		case 'G':
			mode = ALL_GID;
			break;

		case 'g':
			mode = GID;
			break;

		case 'u':
			mode = UID;
			break;

		case 'n':
			dmode = NAMES;
			break;

		case 'r':
			uid = ruid;
			gid = rgid;
			break;

		default:
			return 1;
		}
	}

	/* TODO: validate arguments together */

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
		uid = ruid = euid = pwd->pw_uid;
		gid = rgid = egid = pwd->pw_gid;
	}

	if (mode == ALL_GID) {
		print_groups(uid, rgid, egid, dmode);
	} else if (mode == GID) {
		print_id(get_name(GROUP, gid), gid, dmode);
	} else if (mode == UID) {
		print_id(get_name(USER, uid), uid, dmode);
	} else {
		printf("uid=");
		print_id(get_name(USER, ruid), ruid, 0);

		printf(" gid=");
		print_id(get_name(GROUP, rgid), rgid, 0);

		if (ruid != euid) {
			printf(" euid=");
			print_id(get_name(USER, euid), euid, 0);
		}

		if (rgid != egid) {
			printf(" egid=");
			print_id(get_name(GROUP, egid), egid, 0);
		}

		print_groups(ruid, rgid, egid, FULL);
	}

	putchar('\n');
	return 0;
}
