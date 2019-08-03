/*
 * UNG's Not GNU
 * 
 * Copyright (c) 2011-2014, Jakob Kaivo <jakob@kaivo.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char *id_desc = "return user identity";
const char *id_inv =
    "id [user]\nid -G [-n] [user]\nid -g [-nr] user\nid -u [-nr] [user]";

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
		if (i > 0)
			putchar(sep);
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

int main(int argc, char **argv)
{
	int n = 0, r = 0;
	char mode = 0;
	int c;
	struct passwd pw;
	struct group gr;

	while ((c = getopt(argc, argv, ":Ggunr")) != -1) {
		switch (c) {
		case 'G':
		case 'g':
		case 'u':
			if (mode)
				return 1;
			mode = c;
			break;
		case 'n':
			n = 1;
			break;
		case 'r':
			r = 1;
			break;
		default:
			return 1;
		}
	}

	if ((mode == 0 && (n == 1 || r == 1)) || (mode == 'G' && r == 1))
		return 1;

	if (optind >= argc)
		pw = *getpwuid(mode == 'u' && r == 0 ? geteuid() : getuid());
	else if (optind == argc - 1)
		pw = *getpwnam(argv[optind]);
	else
		return 1;

	switch (mode) {
	case 'G':
		id_printgids(pw.pw_name, n ? NAMES : NUMS);
		break;
	case 'g':
		gr = *getgrgid(r ? getgid() : getegid());
		if (n)
			printf("%s", gr.gr_name);
		else
			printf("%u", gr.gr_gid);
		break;
	case 'u':
		if (n)
			printf("%s", pw.pw_name);
		else
			printf("%u", pw.pw_uid);
		break;
	default:
		gr = *getgrgid(pw.pw_uid == getuid()? getgid() : pw.pw_gid);
		printf("uid=%u(%s) gid=%u(%s)", pw.pw_uid, pw.pw_name,
		       gr.gr_gid, gr.gr_name);
		if (pw.pw_uid != geteuid()) {
			pw = *getpwuid(geteuid());
			printf(" euid=%u(%s)", pw.pw_uid, pw.pw_name);
		}
		if (gr.gr_gid != getegid()) {
			gr = *getgrgid(getegid());
			printf(" egid=%u(%s)", gr.gr_gid, gr.gr_name);
		}
		id_printgids(pw.pw_name, FULL);
	}
	printf("\n");
	return 0;
}
