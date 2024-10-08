/* 
   Copyright (C) by Ronnie Sahlberg <ronniesahlberg@gmail.com> 2015
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/*
 * This example setts up stats collection, reads the specific file
 * and finally prints the stats on a per procedure when finished.
 */

#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef AROS
#include "aros_compat.h"
#endif


#ifdef WIN32
#include <win32/win32_compat.h>
#pragma comment(lib, "ws2_32.lib")
WSADATA wsaData;
#else
#include <sys/stat.h>
#include <string.h>
#endif
 
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-raw-mount.h"
#include "libnfs-raw-nfs.h"

struct file_context {
	int fd;
	struct nfs_context *nfs;
	struct nfsfh *nfsfh;
	struct nfs_url *url;
};

void usage(void)
{
	fprintf(stderr, "Usage: nfs-nfsstat <file>\n");
	fprintf(stderr, "<file> read a file and print the stats callback data.\n");
	exit(0);
}

static void
free_file_context(struct file_context *file_context)
{
	if (file_context->fd != -1) {
		close(file_context->fd);
	}
	if (file_context->nfsfh != NULL) {
		nfs_close(file_context->nfs, file_context->nfsfh);
	}
	if (file_context->nfs != NULL) {
		nfs_destroy_context(file_context->nfs);
	}
	nfs_destroy_url(file_context->url);
	free(file_context);
}

struct nfsstat {
	uint32_t count;
	uint32_t errors;
	uint64_t max_response_time;
	uint64_t aggregated_response_time;
};

struct nfsstat nfs3stats[22];

void stats_cb(struct rpc_context *rpc,
	      struct rpc_pdu_stats *data, void *private_data)
{
	if (data->prog != NFS_PROGRAM) {
		return;
	}
	if (data->vers == NFS_V3) {
		if (data->proc > 21) {
			return;
		}
		nfs3stats[data->proc].count++;
		if (data->status) {
			nfs3stats[data->proc].errors++;
		}
		if (nfs3stats[data->proc].max_response_time < data->response_time) {
			nfs3stats[data->proc].max_response_time = data->response_time;
		}
		nfs3stats[data->proc].aggregated_response_time += data->response_time;
	}
}

char *nfs3names[22] = {
		"NULL",
		"GETATTR",
		"SETATTR",
		"LOOKUP",
		"ACCESS",
		"READLINK",
		"READ",
		"WRITE",
		"CREATE",
		"MKDIR",
		"SYMLINK",
		"MKNOD",
		"REMOVE",
		"RMDIR",
		"RENAME",
		"LINK",
		"READDIR",
		"READDIRPLUS",
		"FSSTAT",
		"FSINFO",
		"PATHCONF",
		"COMMIT"
};

void print_final_stats(void)
{
	int i;
	uint64_t avg;

	printf("%-12s %5s %6s %21s %25s\n", "PROCEDURE", "COUNT", "ERRORS", "MAX RESPONSE TIME(us)", "AVERAGE RESPONSE TIME(us)");
	for (i = 0; i < 22; i++) {
		avg = 0;
		if (nfs3stats[i].count) {
			avg = nfs3stats[i].aggregated_response_time / nfs3stats[i].count;
		}
		printf("%-12s %5d %6d %21d %25d\n", nfs3names[i], nfs3stats[i].count, nfs3stats[i].errors, nfs3stats[i].max_response_time, avg);
	}
}

static struct file_context *
open_file(const char *url, int flags)
{
	struct file_context *file_context;

	file_context = malloc(sizeof(struct file_context));
	if (file_context == NULL) {
		fprintf(stderr, "Failed to malloc file_context\n");
		return NULL;
	}
	file_context->fd     = -1;
	file_context->nfs    = NULL;
	file_context->nfsfh  = NULL;
	file_context->url    = NULL;
	
	file_context->nfs = nfs_init_context();
	if (file_context->nfs == NULL) {
		fprintf(stderr, "failed to init context\n");
		free_file_context(file_context);
		return NULL;
	}

	rpc_set_stats_cb(nfs_get_rpc_context(file_context->nfs), stats_cb, NULL);

	file_context->url = nfs_parse_url_full(file_context->nfs, url);
	if (file_context->url == NULL) {
		fprintf(stderr, "%s\n", nfs_get_error(file_context->nfs));
		free_file_context(file_context);
		return NULL;
	}

	if (nfs_mount(file_context->nfs, file_context->url->server,
				file_context->url->path) != 0) {
		fprintf(stderr, "Failed to mount nfs share : %s\n",
			       nfs_get_error(file_context->nfs));
		free_file_context(file_context);
		return NULL;
	}

	if (flags == O_RDONLY) {
		if (nfs_open(file_context->nfs, file_context->url->file, flags,
				&file_context->nfsfh) != 0) {
 			fprintf(stderr, "Failed to open file %s: %s\n",
				       file_context->url->file,
				       nfs_get_error(file_context->nfs));
			free_file_context(file_context);
			return NULL;
		}
	} else {
		if (nfs_creat(file_context->nfs, file_context->url->file, 0660,
				&file_context->nfsfh) != 0) {
 			fprintf(stderr, "Failed to creat file %s: %s\n",
				       file_context->url->file,
				       nfs_get_error(file_context->nfs));
			free_file_context(file_context);
			return NULL;
		}
	}
	return file_context;
}


#define BUFSIZE 1024*1024
static char buf[BUFSIZE];

int main(int argc, char *argv[])
{
	struct file_context *nf;
	struct nfs_stat_64 st;
	uint64_t off;
	int64_t count;
	
#ifdef WIN32
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		printf("Failed to start Winsock2\n");
		return 10;
	}
#endif

#ifdef AROS
	aros_init_socket();
#endif

	if (argc < 2) {
		usage();
	}

	nf = open_file(argv[1], O_RDONLY);
	if (nf == NULL) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		exit(10);
	}
	if (nfs_fstat64(nf->nfs, nf->nfsfh, &st) < 0) {
		fprintf(stderr, "Failed to stat %s\n", argv[1]);
		exit(10);
	}
	
	off = 0;
	while (off < st.nfs_size) {
		count = st.nfs_size - off;
		if (count > BUFSIZE) {
			count = BUFSIZE;
		}
		count = nfs_pread(nf->nfs, nf->nfsfh, buf, count, off);
		if (count < 0) {
			fprintf(stderr, "Failed to read from file\n");
			free_file_context(nf);
			return 10;
		}
		off += count;
	}

	free_file_context(nf);
	print_final_stats();
	return 0;
}
