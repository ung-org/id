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

static void print_id(const char *prefix, const char *name, id_t id, enum display mode)
{
	printf("%s", prefix);
	if (mode == NAMES) {
		if (name) {
			printf("%s", name);
		} else {
			printf("%ju", (uintmax_t)id);
		}
	} else if (mode == NUMBERS) {
		printf("%ju", (uintmax_t)id);
	} else {
		if (name) {
			printf("%ju(%s)", (uintmax_t)id, name);
		} else {
			printf("%ju", (uintmax_t)id);
		}
	}
}

static void print_groups(uid_t uid, enum display mode)
{
	struct passwd *pwd = getpwuid(uid);
	if (pwd == NULL) {
		return;
	}

	struct group *grp = NULL;
	char *prefix = "";
	if (mode == FULL) {
		prefix = " groups=";
	}

	setgrent();
	while ((grp = getgrent()) != NULL) {
		for (int i = 0; grp->gr_mem[i] != NULL; i++) {
			if (!strcmp(pwd->pw_name, grp->gr_mem[i])) {
				print_id(prefix, grp->gr_name, grp->gr_gid, mode);
				if (mode == FULL) {
					prefix = ",";
				} else {
					prefix = " ";
				}
			}
		}
	}
	endgrent();
}

int main(int argc, char *argv[])
{
	enum display dmode = NUMBERS;
	enum mode mode = DEFAULT;
	int c;
	uid_t ruid = getuid();
	uid_t euid = geteuid();
	gid_t rgid = getgid();
	gid_t egid = getegid();
	uid_t uid = euid;
	gid_t gid = egid;

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
		/* TODO: output real and/or effective gids if necessary */
		print_groups(uid, dmode);
	} else if (mode == GID) {
		print_id("", get_name(GROUP, gid), gid, dmode);
	} else if (mode == UID) {
		print_id("", get_name(USER, uid), uid, dmode);
	} else {
		print_id("uid=", get_name(USER, ruid), ruid, 0);
		print_id(" gid=", get_name(GROUP, rgid), rgid, 0);

		if (ruid != euid) {
			print_id(" euid=", get_name(USER, euid), euid, 0);
		}

		if (rgid != egid) {
			print_id(" egid=", get_name(GROUP, egid), egid, 0);
		}

		print_groups(ruid, FULL);
	}

	printf("\n");
	return 0;
}
