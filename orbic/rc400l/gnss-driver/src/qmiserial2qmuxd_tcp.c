/*
 * qmiserial2qmuxd_tcp - TCP/Unix socket variant of qmiserial2qmuxd
 *
 * Copyright (C) 2017 Joey Hewitt <joey@joeyhewitt.com>
 * Copyright (C) 2026 Luke Jenkins
 *
 * Based on qmiserial2qmuxd by Joey Hewitt.
 * Bridges QMI serial frames to/from qmuxd via Unix socket.
 * Usage: qmiserial2qmuxd_tcp [sockpath] [qmux_dir]
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/uio.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#define LOG(fmt, ...) fprintf(stderr, "[qmuxd-tcp] %s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

struct serial_hdr {
	uint16_t len;
	uint8_t flags;
	uint8_t service;
	uint8_t client;
} __packed;

struct qmuxd_hdr {
	uint16_t len;
	uint8_t _unused4[2];
	uint32_t qmuxd_client;
	uint32_t message;
	uint32_t qmuxd_client_again;
	uint16_t transaction;
	uint8_t _unused1[2];
	int32_t sys_err;
	uint32_t qmi_err;
	uint32_t channel;
	uint8_t service;
	uint8_t _unused2[3];
	uint8_t qmi_client;
	uint8_t flags;
	uint8_t _unused3[2];
} __packed;

struct qmi_ctl {
	uint8_t flags;
	uint8_t transaction;
	uint16_t message;
} __packed;

struct qmi_svc {
	uint8_t flags;
	uint16_t transaction;
	uint16_t message;
} __packed;

union qmi_ctl_or_svc {
	struct qmi_ctl ctl;
	struct qmi_svc svc;
} __packed;

enum { QMUXD_MSG_RAW_QMI_CTL = 11, QMUXD_MSG_WRITE_QMI_SDU = 0 };
#define MAX_QMI_MSG_SIZE 0x4100

static void hexdump(const char *label, const uint8_t *data, size_t len) {
	char hex[256];
	size_t n = len > 80 ? 80 : len;
	for (size_t i = 0; i < n; i++)
		sprintf(hex + i*3, "%02x ", data[i]);
	hex[n*3] = '\0';
	LOG("%s (%zu bytes): %s%s", label, len, hex, len > 80 ? "..." : "");
}

static int g_qmuxd_socket;
static uint32_t g_qmuxd_client_id;
static int g_tcpfd;
static char g_sockpath[256] = "/var/";

static int readall(int fd, void *buf, size_t len) {
	size_t total = 0;
	while (total < len) {
		ssize_t ret = read(fd, (uint8_t *)buf + total, len - total);
		if (ret <= 0) return -1;
		total += (size_t)ret;
	}
	return 0;
}

static int writeall(int fd, const void *buf, size_t len) {
	size_t total = 0;
	while (total < len) {
		ssize_t ret = write(fd, (const uint8_t *)buf + total, len - total);
		if (ret <= 0) return -1;
		total += (size_t)ret;
	}
	return 0;
}

static int read_msg(int fd, int expect_frame, void *msg, size_t msg_buf_size) {
	if (expect_frame) {
		uint8_t frame;
		if (readall(fd, &frame, 1) < 0) return -1;
		if (frame != 1) {
			LOG("invalid frame byte: %u", frame);
			return -1;
		}
	}
	uint16_t *hdr = msg;
	if (readall(fd, hdr, 2) < 0) return -1;
	uint16_t len = *hdr;
	if (len > msg_buf_size) {
		LOG("message too big: %u > %zu", len, msg_buf_size);
		return -1;
	}
	if (len > 2) {
		if (readall(fd, (uint8_t *)msg + 2, len - 2) < 0) return -1;
	}
	return 0;
}

static int send_to_qmuxd(const uint8_t *frame_data, size_t frame_len) {
	/*
	 * 40-byte qmuxd header format (Android qmuxd):
	 * The QMUX frame starts with [0x01][len:2][flags:1][service:1][client:1]
	 * We extract service and client from the frame to populate the header.
	 * The QMI SDU (after the QMUX header) is sent as the payload.
	 */
	if (frame_len < 6) { LOG("frame too short"); return -1; }

	/* Parse the QMUX frame header */
	/* frame_data[0] = 0x01 (IF type) */
	/* frame_data[1..2] = len */
	uint8_t ctrl_flags = frame_data[3];
	uint8_t service = frame_data[4];
	uint8_t client = frame_data[5];
	const uint8_t *qmi_sdu = frame_data + 6;
	size_t qmi_sdu_len = frame_len - 6;

	uint32_t qmuxd_msg;
	uint16_t transaction = 0;
	uint16_t message = 0;

	if (service == 0) {
		/* CTL service: 1-byte transaction */
		if (qmi_sdu_len >= 4) {
			transaction = qmi_sdu[1];
			memcpy(&message, &qmi_sdu[2], 2);
		}
		qmuxd_msg = QMUXD_MSG_RAW_QMI_CTL;
	} else {
		/* Other services: 2-byte transaction */
		if (qmi_sdu_len >= 5) {
			memcpy(&transaction, &qmi_sdu[1], 2);
			memcpy(&message, &qmi_sdu[3], 2);
		}
		qmuxd_msg = QMUXD_MSG_WRITE_QMI_SDU;
	}

	hexdump("TX frame", frame_data, frame_len);
	LOG("TX svc=%u client=%u msg=0x%04x txn=%u qmuxd_msg=%u",
		service, client, message, transaction, qmuxd_msg);

	struct qmuxd_hdr qhdr;
	memset(&qhdr, 0, sizeof(qhdr));
	qhdr.len = sizeof(qhdr) + qmi_sdu_len;
	qhdr.message = qmuxd_msg;
	qhdr.transaction = transaction;
	qhdr.qmuxd_client = g_qmuxd_client_id;
	qhdr.qmuxd_client_again = g_qmuxd_client_id;
	qhdr.service = service;
	qhdr.qmi_client = client;
	qhdr.flags = ctrl_flags;

	hexdump("TX qmuxd_hdr", (const uint8_t *)&qhdr, sizeof(qhdr));

	struct iovec iov[2] = {
		{ .iov_base = &qhdr, .iov_len = sizeof(qhdr) },
		{ .iov_base = (void *)qmi_sdu, .iov_len = qmi_sdu_len },
	};

	size_t total = sizeof(qhdr) + qmi_sdu_len;
	ssize_t ret = writev(g_qmuxd_socket, iov, 2);
	if (ret < 0 || (size_t)ret != total) {
		LOG("writev error: %s (wrote %zd/%zu)", strerror(errno), ret, total);
		return -1;
	}
	return 0;
}

static int send_to_tcp(const struct qmuxd_hdr *qhdr, const void *msg) {
	LOG("RX svc=%u msg=%u syserr=%d qmierr=%u len=%u",
		qhdr->service, qhdr->message, qhdr->sys_err, qhdr->qmi_err,
		(unsigned)(qhdr->len - sizeof(*qhdr)));

	if (qhdr->sys_err != 0) {
		LOG("qmuxd syserr=%d, dropping", qhdr->sys_err);
		return 0;
	}

	struct serial_hdr shdr;
	shdr.len = qhdr->len - sizeof(*qhdr) + sizeof(shdr);
	shdr.service = qhdr->service;
	shdr.client = qhdr->qmi_client;
	shdr.flags = 0x80;

	uint8_t frame = 1;
	if (writeall(g_tcpfd, &frame, 1) < 0) return -1;
	if (writeall(g_tcpfd, &shdr, sizeof(shdr)) < 0) return -1;
	if (shdr.len > sizeof(shdr)) {
		if (writeall(g_tcpfd, msg, shdr.len - sizeof(shdr)) < 0) return -1;
	}
	return 0;
}

static void *qmuxd_read_thread(void *arg) {
	(void)arg;
	for (;;) {
		uint8_t buf[MAX_QMI_MSG_SIZE];

		/* Read the first 4 bytes to get total_msg_size */
		if (readall(g_qmuxd_socket, buf, 4) < 0) {
			LOG("qmuxd read hdr error: %s", strerror(errno));
			break;
		}
		uint32_t total_size;
		memcpy(&total_size, buf, 4);
		LOG("qmuxd response: first 4 bytes (total_size?) = %u (0x%08x)", total_size, total_size);

		/* For 16-bit interpretation */
		uint16_t size16;
		memcpy(&size16, buf, 2);
		LOG("qmuxd response: first 2 bytes (len16?) = %u", size16);

		/* Read the rest based on the 32-bit size */
		size_t remaining;
		if (total_size > 4 && total_size < MAX_QMI_MSG_SIZE) {
			remaining = total_size - 4;
		} else if (size16 > 2 && size16 < MAX_QMI_MSG_SIZE) {
			/* Maybe it's a 16-bit length field */
			remaining = size16 - 2;
		} else {
			LOG("can't determine msg size, reading 256 bytes");
			remaining = 256;
		}

		if (readall(g_qmuxd_socket, buf + 4, remaining) < 0) {
			LOG("qmuxd read body error (wanted %zu): %s", remaining, strerror(errno));
			break;
		}

		hexdump("qmuxd_response", buf, 4 + remaining);

		/* Try to forward as QMUX frame to TCP client */
		/* Look for 0x01 (QMUX IF type marker) in the first 16 bytes */
		int qmux_offset = -1;
		for (int i = 0; i < 16 && i < (int)(4 + remaining); i++) {
			if (buf[i] == 0x01) {
				/* Check if this looks like a QMUX frame */
				if (i + 6 < (int)(4 + remaining)) {
					uint16_t flen;
					memcpy(&flen, &buf[i+1], 2);
					if (flen > 4 && flen < 4096) {
						LOG("possible QMUX frame at offset %d, len=%u", i, flen);
						qmux_offset = i;
						break;
					}
				}
			}
		}

		if (qmux_offset >= 0) {
			size_t frame_start = qmux_offset;
			uint16_t frame_len;
			memcpy(&frame_len, &buf[frame_start + 1], 2);
			size_t total_frame = 1 + frame_len; /* 0x01 + len field value */
			LOG("forwarding QMUX frame: offset=%d size=%zu", qmux_offset, total_frame);
			if (writeall(g_tcpfd, &buf[frame_start], total_frame) < 0) {
				LOG("tcp write error: %s", strerror(errno));
				break;
			}
		} else {
			LOG("no QMUX frame found in response, not forwarding");
		}
	}
	return NULL;
}

static int open_qmuxd(void) {
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) { LOG("socket: %s", strerror(errno)); return -1; }

	struct sockaddr_un addr = { .sun_family = AF_UNIX };

	snprintf(addr.sun_path, sizeof(addr.sun_path),
		"%sqmux_client_socket%7lu", g_sockpath, (unsigned long)getpid());
	unlink(addr.sun_path);
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG("bind %s: %s", addr.sun_path, strerror(errno));
		close(sockfd);
		return -1;
	}

	snprintf(addr.sun_path, sizeof(addr.sun_path),
		"%sqmux_connect_socket", g_sockpath);
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG("connect %s: %s", addr.sun_path, strerror(errno));
		close(sockfd);
		return -1;
	}

	ssize_t ret = recv(sockfd, &g_qmuxd_client_id, sizeof(g_qmuxd_client_id), 0);
	if (ret != sizeof(g_qmuxd_client_id)) {
		LOG("failed to receive client id");
		close(sockfd);
		return -1;
	}

	LOG("connected to qmuxd, client_id=%u", g_qmuxd_client_id);
	g_qmuxd_socket = sockfd;
	return 0;
}

static char g_listen_path[256] = "/tmp/qmi_bridge.sock";

static int unix_listen(const char *path) {
	LOG("unix_listen: creating AF_UNIX socket at %s", path);
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		LOG("unix socket: %s", strerror(errno));
		return -1;
	}

	unlink(path);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG("unix bind %s: %s", path, strerror(errno));
		close(sockfd);
		return -1;
	}

	/* Make it world-accessible so adb forward can connect */
	chmod(path, 0666);

	if (listen(sockfd, 1) < 0) {
		LOG("unix listen: %s", strerror(errno));
		close(sockfd);
		return -1;
	}

	LOG("listening on unix socket %s (fd=%d)", path, sockfd);
	return sockfd;
}

int main(int argc, char *argv[]) {
	if (argc > 1) snprintf(g_listen_path, sizeof(g_listen_path), "%s", argv[1]);
	if (argc > 2) snprintf(g_sockpath, sizeof(g_sockpath), "%s", argv[2]);

	signal(SIGPIPE, SIG_IGN);

	LOG("main: qmux_sockpath=%s listen_path=%s", g_sockpath, g_listen_path);
	if (open_qmuxd() < 0) { LOG("main: open_qmuxd failed"); return 1; }

	int listenfd = unix_listen(g_listen_path);
	if (listenfd < 0) { LOG("main: unix_listen failed"); return 1; }
	LOG("main: ready, entering accept loop");

	for (;;) {
		LOG("waiting for connection on %s...", g_listen_path);
		struct sockaddr_un client_addr;
		socklen_t addrlen = sizeof(client_addr);
		g_tcpfd = accept(listenfd, (struct sockaddr *)&client_addr, &addrlen);
		if (g_tcpfd < 0) {
			LOG("accept: %s", strerror(errno));
			continue;
		}
		LOG("client connected");

		pthread_t reader;
		pthread_create(&reader, NULL, qmuxd_read_thread, NULL);

		/* TCP read loop (main thread) */
		for (;;) {
			uint8_t frame_buf[MAX_QMI_MSG_SIZE + 1];

			/* Read frame byte (0x01) */
			if (readall(g_tcpfd, frame_buf, 1) < 0) {
				LOG("tcp read frame byte error");
				break;
			}
			if (frame_buf[0] != 0x01) {
				LOG("bad frame byte: 0x%02x", frame_buf[0]);
				break;
			}

			/* Read length (2 bytes) */
			if (readall(g_tcpfd, frame_buf + 1, 2) < 0) {
				LOG("tcp read len error");
				break;
			}
			uint16_t qmux_len;
			memcpy(&qmux_len, frame_buf + 1, 2);
			LOG("QMUX frame: len=%u", qmux_len);

			if (qmux_len < 3 || qmux_len > MAX_QMI_MSG_SIZE) {
				LOG("bad QMUX len: %u", qmux_len);
				break;
			}

			/* Read remaining data (len - 2 bytes since len includes itself) */
			size_t remaining = qmux_len - 2;
			if (readall(g_tcpfd, frame_buf + 3, remaining) < 0) {
				LOG("tcp read payload error");
				break;
			}

			/* Total QMUX frame: [0x01][len:2][data:len-2] = 1 + qmux_len */
			size_t total_frame = 1 + qmux_len;
			if (send_to_qmuxd(frame_buf, total_frame) < 0) {
				LOG("qmuxd write error");
				break;
			}
		}

		close(g_tcpfd);
		pthread_cancel(reader);
		pthread_join(reader, NULL);
		LOG("client disconnected, waiting for next...");
	}

	return 0;
}
