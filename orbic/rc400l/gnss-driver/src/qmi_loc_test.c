/*
 * qmi_loc_test.c -- Minimal QMI LOC GNSS client for MDM9207
 *
 * Copyright (C) 2026 Luke Jenkins
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
 *
 * ---
 *
 * Activates the GNSS engine on Qualcomm MDM9207-based cellular modules
 * where the OEM has stripped all GNSS/GPS software from the application
 * processor. Constructs a fake QMI IDL service object to register with
 * the LOC service on the modem DSP, then communicates via raw TLV messages.
 *
 * QMI IDL struct layouts are interoperability interface definitions derived
 * from Qualcomm's qmi_idl_lib_internal.h. QMI LOC message IDs and TLV
 * formats were cross-referenced from:
 *   - Qualcomm LOC v02 IDL (BSD, Copyright (c) 2011-2014 Code Aurora Forum)
 *   - libqmi (LGPL-2.1+, freedesktop.org)
 *
 * Links dynamically against the modem's libqmi_cci.so.1 and
 * libqmi_client_qmux.so.1 to communicate through qmuxd.
 *
 * Target: Orbic RC400L (MDM9207), cross-compiled with
 *   arm-linux-gnueabihf-gcc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

/*
 * Position seed for GNSS cold-start assistance.
 * Replace with your approximate latitude and longitude (WGS84 degrees).
 * A 10 km uncertainty radius is used, so rough values are fine.
 */
#ifndef YOUR_LAT
#error "Define YOUR_LAT and YOUR_LON with your approximate coordinates, e.g. -DYOUR_LAT=40.0 -DYOUR_LON=-111.0"
#endif

/*
 * QMI IDL service object structure - matches Qualcomm's
 * qmi_idl_lib_internal.h struct qmi_idl_service_object.
 */
struct qmi_idl_service_message_table_entry {
	uint16_t qmi_message_id;
	uint16_t message_table_message_id;
	uint16_t max_msg_len;
};

struct qmi_idl_type_table_entry {
	uint32_t c_struct_sz;
	const uint8_t *p_encoded_type_data;
};

struct qmi_idl_message_table_entry {
	uint32_t c_struct_sz;
	const uint8_t *p_encoded_tlv_data;
};

struct qmi_idl_range_table_entry {
	uint32_t range_offset;
	uint32_t range_low;
	uint32_t range_high;
};

struct qmi_idl_type_table_object {
	uint16_t n_types;
	uint16_t n_messages;
	uint8_t  n_referenced_tables;
	const struct qmi_idl_type_table_entry *p_types;
	const struct qmi_idl_message_table_entry *p_messages;
	const struct qmi_idl_type_table_object **p_referenced_tables;
	const struct qmi_idl_range_table_entry *p_ranges;
};

struct qmi_idl_service_object {
	uint32_t library_version;
	uint32_t idl_version;
	uint32_t service_id;
	uint32_t max_msg_len;
	uint16_t n_msgs[3];
	const struct qmi_idl_service_message_table_entry *msgid_to_msg[3];
	const struct qmi_idl_type_table_object *p_type_table;
	uint32_t idl_minor_version;
	struct qmi_idl_service_object *parent_service_obj;
};

typedef struct qmi_idl_service_object *qmi_idl_service_object_type;

/* QMI CCI types */
typedef int32_t qmi_client_error_type;
typedef void *qmi_client_type;
typedef uint32_t qmi_service_instance;

#define QMI_NO_ERR 0
#define QMI_TIMEOUT_ERR -7

/* QMI LOC service constants */
#define QMI_LOC_SERVICE_ID 0x10
#define QMI_LOC_V02_IDL_MAJOR_VERS 0x02
#define QMI_LOC_V02_IDL_MINOR_VERS 0x23

/* LOC message IDs (from libqmi qmi-service-loc.json) */
#define QMI_LOC_REG_EVENTS             0x0021
#define QMI_LOC_START                  0x0022
#define QMI_LOC_STOP                   0x0023
#define QMI_LOC_EVENT_POSITION_IND     0x0024
#define QMI_LOC_EVENT_GNSS_SV_IND     0x0025
#define QMI_LOC_EVENT_NMEA_IND         0x0026
#define QMI_LOC_INJECT_PREDICTED_ORBITS 0x0035  /* XTRA orbit data */
#define QMI_LOC_INJECT_UTC_TIME        0x0038  /* timeUtc(u64,TLV01) + timeUnc(u32,TLV02) */
#define QMI_LOC_INJECT_POSITION        0x0039
#define QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE 0x0034  /* query maxPartSize */
#define QMI_LOC_SET_ENGINE_LOCK        0x003C  /* TLV 0x01: uint32 lock_type */
#define QMI_LOC_GET_ENGINE_LOCK        0x003D  /* resp TLV 0x10: uint32 lock_type */
#define QMI_LOC_SET_NMEA_TYPES         0x003E  /* nmea_types bitmask (u32, TLV01) */
#define QMI_LOC_SET_OPERATION_MODE     0x004A  /* TLV 0x01: uint32 mode */
#define QMI_LOC_GET_OPERATION_MODE     0x004B  /* resp TLV 0x10: uint32 mode */

/* Engine lock types */
#define QMI_LOC_LOCK_NONE  0  /* no lock - all sessions allowed */
#define QMI_LOC_LOCK_MI    1  /* lock mobile-initiated sessions */
#define QMI_LOC_LOCK_MT    2  /* lock mobile-terminated sessions */
#define QMI_LOC_LOCK_ALL   3  /* lock all sessions */

/* Operation modes */
#define QMI_LOC_OPER_MODE_DEFAULT    1
#define QMI_LOC_OPER_MODE_MSB        2  /* MS-Based (A-GPS) */
#define QMI_LOC_OPER_MODE_MSA        3  /* MS-Assisted */
#define QMI_LOC_OPER_MODE_STANDALONE 4
#define QMI_LOC_OPER_MODE_CELL_ID    5

/* NMEA type bitmask for QMI_LOC_SET_NMEA_TYPES (QmiLocNmeaType) */
#define QMI_LOC_NMEA_TYPE_GGA   (1u << 0)
#define QMI_LOC_NMEA_TYPE_RMC   (1u << 1)
#define QMI_LOC_NMEA_TYPE_GSV   (1u << 2)
#define QMI_LOC_NMEA_TYPE_GSA   (1u << 3)
#define QMI_LOC_NMEA_TYPE_VTG   (1u << 4)
#define QMI_LOC_NMEA_TYPE_PQXFI (1u << 5)
#define QMI_LOC_NMEA_TYPE_PSTIS (1u << 6)
#define QMI_LOC_NMEA_TYPE_ALL   0x0000FFFFu

/* LOC event masks for REG_EVENTS */
#define QMI_LOC_EVENT_MASK_POSITION        (1ULL << 0)
#define QMI_LOC_EVENT_MASK_GNSS_SV_INFO    (1ULL << 1)
#define QMI_LOC_EVENT_MASK_NMEA            (1ULL << 2)
#define QMI_LOC_EVENT_MASK_ENGINE_STATE    (1ULL << 7)
#define QMI_LOC_EVENT_MASK_FIX_SESSION     (1ULL << 8)

/* OS params for CCI */
typedef struct {
	int ext_signal;
	int sig;
	int timer_sig;
} qmi_client_os_params;

/* Indication callback */
typedef void (*qmi_client_ind_cb)(
	qmi_client_type client,
	unsigned int msg_id,
	void *ind_buf,
	unsigned int ind_buf_len,
	void *ind_cb_data
);

/* CCI API declarations - linked from libqmi_cci.so.1 */
extern qmi_client_error_type qmi_client_init_instance(
	qmi_idl_service_object_type service_obj,
	qmi_service_instance instance_id,
	qmi_client_ind_cb ind_cb,
	void *ind_cb_data,
	qmi_client_os_params *os_params,
	uint32_t timeout_msecs,
	qmi_client_type *user_handle
);

extern qmi_client_error_type qmi_client_send_raw_msg_sync(
	qmi_client_type user_handle,
	unsigned int msg_id,
	void *req_buf,
	unsigned int req_buf_len,
	void *resp_buf,
	unsigned int resp_buf_len,
	unsigned int *resp_msg_len,
	unsigned int timeout_msecs
);

extern qmi_client_error_type qmi_client_release(
	qmi_client_type user_handle
);

/*
 * DMS service object - reference the exported data symbol directly
 * instead of calling the _internal getter (avoids version mismatch).
 */
extern struct qmi_idl_service_object dms_qmi_idl_service_object_v01;

/*
 * Fake LOC service object.
 * qmi_client_init_instance reads: library_version, service_id, idl_version, max_msg_len
 * qmi_client_send_raw_msg_sync does NOT use message tables.
 * We'll set library_version to match whatever DMS uses at runtime.
 */
static struct qmi_idl_type_table_object loc_type_table = {
	.n_types = 0,
	.n_messages = 0,
	.n_referenced_tables = 0,
	.p_types = NULL,
	.p_messages = NULL,
	.p_referenced_tables = NULL,
	.p_ranges = NULL,
};

static struct qmi_idl_service_object fake_loc_svc_obj = {
	.library_version = 0,  /* filled at runtime from DMS */
	.idl_version = QMI_LOC_V02_IDL_MAJOR_VERS,
	.service_id = QMI_LOC_SERVICE_ID,
	.max_msg_len = 0x4000,
	.n_msgs = {0, 0, 0},
	.msgid_to_msg = {NULL, NULL, NULL},
	.p_type_table = &loc_type_table,
	.idl_minor_version = QMI_LOC_V02_IDL_MINOR_VERS,
	.parent_service_obj = NULL,
};

static volatile int g_running = 1;

static void sighandler(int sig) {
	(void)sig;
	g_running = 0;
}

static void hexdump(const char *label, const void *data, size_t len) {
	const uint8_t *p = data;
	printf("%s (%zu bytes):", label, len);
	for (size_t i = 0; i < len && i < 256; i++) {
		if (i % 16 == 0) printf("\n ");
		printf(" %02x", p[i]);
	}
	if (len > 256) printf("\n  ...");
	printf("\n");
}

static void dump_service_object(const char *name, struct qmi_idl_service_object *obj) {
	printf("%s service object at %p:\n", name, (void *)obj);
	printf("  library_version:  %u\n", obj->library_version);
	printf("  idl_version:      %u\n", obj->idl_version);
	printf("  service_id:       0x%02x\n", obj->service_id);
	printf("  max_msg_len:      %u\n", obj->max_msg_len);
	printf("  n_msgs:           req=%u resp=%u ind=%u\n",
		obj->n_msgs[0], obj->n_msgs[1], obj->n_msgs[2]);
	printf("  p_type_table:     %p\n", (void *)obj->p_type_table);
	printf("  idl_minor_version:%u\n", obj->idl_minor_version);
	printf("  parent:           %p\n", (void *)obj->parent_service_obj);
}

static void loc_ind_cb(
	qmi_client_type client,
	unsigned int msg_id,
	void *ind_buf,
	unsigned int ind_buf_len,
	void *ind_cb_data
) {
	(void)client;
	(void)ind_cb_data;
	printf("[LOC IND] msg_id=0x%04x len=%u\n", msg_id, ind_buf_len);
	hexdump("  payload", ind_buf, ind_buf_len);

	if (msg_id == QMI_LOC_EVENT_NMEA_IND && ind_buf_len > 4) {
		const uint8_t *p = ind_buf;
		if (p[0] == 0x01 && ind_buf_len > 3) {
			uint16_t slen;
			memcpy(&slen, &p[1], 2);
			if (slen > 0 && slen + 3 <= ind_buf_len) {
				printf("  NMEA: %.*s\n", slen, &p[3]);
			}
		}
	}

	if (msg_id == QMI_LOC_EVENT_POSITION_IND) {
		/*
		 * Parse key TLVs from QMI_LOC_EVENT_POSITION_IND_V02 (0x0024):
		 *   TLV 0x01: sessionStatus (uint32)
		 *     0 = SUCCESS (confirmed position fix)
		 *     1 = IN_PROGRESS (engine still acquiring, intermediate update)
		 *     2 = GENERAL_FAILURE
		 *   TLV 0x02: sessionId (uint8)
		 *   TLV 0x10: latitude (double, decimal degrees WGS84)
		 *   TLV 0x11: longitude (double, decimal degrees WGS84)
		 *   TLV 0x12: horUncCircular (float, meters)
		 *   TLV 0x1A: altitudeWrtEllipsoid (float, meters)
		 */
		const uint8_t *p  = ind_buf;
		size_t         sz = ind_buf_len;
		size_t         i  = 0;

		uint32_t sess_status = 0xFF;
		double   lat = 0.0, lon = 0.0;
		float    hacc = 0.0f;
		int      has_lat = 0, has_lon = 0, has_hacc = 0;

		while (i + 3 <= sz) {
			uint8_t  type = p[i];
			uint16_t len;
			memcpy(&len, &p[i+1], 2);
			i += 3;
			if (i + len > sz) break;

			if (type == 0x01 && len >= 4)
				memcpy(&sess_status, &p[i], 4);
			else if (type == 0x10 && len >= 8) {
				memcpy(&lat, &p[i], 8); has_lat = 1;
			} else if (type == 0x11 && len >= 8) {
				memcpy(&lon, &p[i], 8); has_lon = 1;
			} else if (type == 0x12 && len >= 4) {
				memcpy(&hacc, &p[i], 4); has_hacc = 1;
			}
			i += len;
		}

		if (sess_status == 0) {
			printf("  ** POSITION FIX (SUCCESS) **\n");
		} else if (sess_status == 1) {
			printf("  ** POSITION UPDATE (IN_PROGRESS) **\n");
		} else {
			printf("  ** POSITION IND status=%u **\n", sess_status);
		}
		if (has_lat && has_lon) {
			printf("    lat=%.6f  lon=%.6f", lat, lon);
			if (has_hacc)
				printf("  hacc=%.0f m", (double)hacc);
			printf("\n");
		}
	}

	/* Engine state indication (0x002B) */
	if (msg_id == 0x002B) {
		printf("  ** ENGINE STATE CHANGE **\n");
	}

	/* Fix session state indication (0x002C) */
	if (msg_id == 0x002C) {
		printf("  ** FIX SESSION STATE CHANGE **\n");
	}

	if (msg_id == QMI_LOC_EVENT_GNSS_SV_IND) {
		printf("  ** GNSS SV INFO **\n");
	}

	if (msg_id == QMI_LOC_INJECT_UTC_TIME) {
		printf("  ** INJECT_UTC_TIME IND (engine acceptance) **\n");
	}

	if (msg_id == QMI_LOC_INJECT_POSITION) {
		printf("  ** INJECT_POSITION IND (engine acceptance) **\n");
	}

	if (msg_id == QMI_LOC_SET_NMEA_TYPES) {
		printf("  ** SET_NMEA_TYPES IND **\n");
	}

	if (msg_id == QMI_LOC_INJECT_PREDICTED_ORBITS) {
		/* Parse status (TLV 0x01, uint32) and optional maxPartSize (TLV 0x11, uint32) */
		printf("  ** INJECT_PREDICTED_ORBITS IND (XTRA) **\n");
		const uint8_t *p = ind_buf;
		size_t i = 0;
		while (i + 3 <= ind_buf_len) {
			uint8_t  tlv_type = p[i];
			uint16_t tlv_len;
			memcpy(&tlv_len, &p[i + 1], 2);
			i += 3;
			if (i + tlv_len > ind_buf_len)
				break;
			if (tlv_type == 0x01 && tlv_len >= 4) {
				uint32_t status;
				memcpy(&status, &p[i], 4);
				printf("  status=%u (%s)\n", status,
				       status == 0 ? "SUCCESS" : "FAILURE");
			} else if (tlv_type == 0x11 && tlv_len >= 4) {
				uint32_t max_part;
				memcpy(&max_part, &p[i], 4);
				printf("  maxPartSize=%u bytes\n", max_part);
			}
			i += tlv_len;
		}
	}
}

/*
 * Inject current UTC time into the GNSS engine to reduce cold-start TTFF.
 * The engine uses this to predict satellite positions rather than doing
 * a blind search across all possible Doppler/code-phase combinations.
 *
 * Message 0x0038: QMI_LOC_INJECT_UTC_TIME_REQ_V02
 *   TLV 0x01: timeUtc  (uint64) - ms since Unix epoch (mandatory)
 *   TLV 0x02: timeUnc  (uint32) - time uncertainty in ms (mandatory)
 *
 * The synchronous response is a transport-level ACK. The engine's actual
 * acceptance/rejection arrives asynchronously as a 0x0038 indication.
 */
static int inject_utc_time(qmi_client_type client)
{
	/* Use time() - maps to 32-bit syscall on ARM, no glibc64 issues */
	time_t now = time(NULL);

	/* Sanity check: reject obviously wrong clocks (before 2020-01-01) */
	if (now < 1577836800L) {
		printf("WARN: modem clock looks wrong (epoch=%ld), skipping UTC inject\n",
		       (long)now);
		return -1;
	}

	uint64_t time_ms = (uint64_t)now * 1000;
	uint32_t unc_ms  = 5000; /* 5-second uncertainty */

	uint8_t req[20];
	int pos = 0;
	req[pos++] = 0x01;
	req[pos++] = 0x08; req[pos++] = 0x00;
	memcpy(&req[pos], &time_ms, 8); pos += 8;
	req[pos++] = 0x02;
	req[pos++] = 0x04; req[pos++] = 0x00;
	memcpy(&req[pos], &unc_ms, 4); pos += 4;

	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_INJECT_UTC_TIME,
		req, pos, resp, sizeof(resp), &resp_len, 5000);

	printf("LOC_INJECT_UTC_TIME: rc=%d time_ms=%llu unc_ms=%u\n",
	       rc, (unsigned long long)time_ms, unc_ms);
	if (rc == 0 && resp_len > 0)
		hexdump("  response", resp, resp_len);
	return rc;
}

/*
 * Configure which NMEA sentence types the engine generates.
 * Message 0x003E: QMI_LOC_SET_NMEA_TYPES
 *   TLV 0x01: nmea_types (uint32 bitmask, mandatory)
 *
 * Enabling ALL (0xFFFF) adds PQXFI/PSTIS extended Qualcomm sentences
 * which may carry time information before a position fix.
 */
/*
 * Query and configure the GNSS engine operation mode and lock state.
 * These must be called BEFORE LOC_START to ensure the engine is properly
 * configured for standalone GPS acquisition.
 */

static void query_operation_mode(qmi_client_type client)
{
	uint8_t resp[64];
	unsigned int resp_len = 0;
	/* GET_OPERATION_MODE has no mandatory TLVs */
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_GET_OPERATION_MODE,
		NULL, 0, resp, sizeof(resp), &resp_len, 5000);
	printf("GET_OPERATION_MODE: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == 0 && resp_len > 0) {
		hexdump("  response", resp, resp_len);
		/* Parse response TLVs looking for 0x10 (operationMode, uint32) */
		unsigned int off = 0;
		while (off + 3 < resp_len) {
			uint8_t  tlv_type = resp[off];
			uint16_t tlv_len;
			memcpy(&tlv_len, &resp[off+1], 2);
			if (tlv_type == 0x10 && tlv_len == 4 && off + 3 + 4 <= resp_len) {
				uint32_t mode;
				memcpy(&mode, &resp[off+3], 4);
				const char *mstr = "UNKNOWN";
				switch (mode) {
					case 1: mstr = "DEFAULT"; break;
					case 2: mstr = "MSB"; break;
					case 3: mstr = "MSA"; break;
					case 4: mstr = "STANDALONE"; break;
					case 5: mstr = "CELL_ID"; break;
				}
				printf("  ** Current operation mode: %u (%s) **\n", mode, mstr);
			}
			off += 3 + tlv_len;
		}
	}
}

static int set_operation_mode(qmi_client_type client, uint32_t mode)
{
	uint8_t req[8];
	int pos = 0;
	/* TLV 0x01: operationMode (uint32) */
	req[pos++] = 0x01;
	req[pos++] = 0x04; req[pos++] = 0x00;
	memcpy(&req[pos], &mode, 4); pos += 4;

	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_SET_OPERATION_MODE,
		req, pos, resp, sizeof(resp), &resp_len, 5000);
	const char *mstr = "UNKNOWN";
	switch (mode) {
		case 1: mstr = "DEFAULT"; break;
		case 2: mstr = "MSB"; break;
		case 3: mstr = "MSA"; break;
		case 4: mstr = "STANDALONE"; break;
		case 5: mstr = "CELL_ID"; break;
	}
	printf("SET_OPERATION_MODE(%s): rc=%d\n", mstr, rc);
	if (rc == 0 && resp_len > 0)
		hexdump("  response", resp, resp_len);
	return rc;
}

static void query_engine_lock(qmi_client_type client)
{
	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_GET_ENGINE_LOCK,
		NULL, 0, resp, sizeof(resp), &resp_len, 5000);
	printf("GET_ENGINE_LOCK: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == 0 && resp_len > 0) {
		hexdump("  response", resp, resp_len);
		unsigned int off = 0;
		while (off + 3 < resp_len) {
			uint8_t  tlv_type = resp[off];
			uint16_t tlv_len;
			memcpy(&tlv_len, &resp[off+1], 2);
			if (tlv_type == 0x10 && tlv_len == 4 && off + 3 + 4 <= resp_len) {
				uint32_t lock;
				memcpy(&lock, &resp[off+3], 4);
				const char *lstr = "UNKNOWN";
				switch (lock) {
					case 0: lstr = "NONE (all allowed)"; break;
					case 1: lstr = "MI (mobile-initiated locked)"; break;
					case 2: lstr = "MT (mobile-terminated locked)"; break;
					case 3: lstr = "ALL (fully locked)"; break;
				}
				printf("  ** Current engine lock: %u (%s) **\n", lock, lstr);
			}
			off += 3 + tlv_len;
		}
	}
}

static int set_engine_lock(qmi_client_type client, uint32_t lock_type)
{
	uint8_t req[8];
	int pos = 0;
	/* TLV 0x01: lockType (uint32) */
	req[pos++] = 0x01;
	req[pos++] = 0x04; req[pos++] = 0x00;
	memcpy(&req[pos], &lock_type, 4); pos += 4;

	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_SET_ENGINE_LOCK,
		req, pos, resp, sizeof(resp), &resp_len, 5000);
	printf("SET_ENGINE_LOCK(%u): rc=%d\n", lock_type, rc);
	if (rc == 0 && resp_len > 0)
		hexdump("  response", resp, resp_len);
	return rc;
}

static int set_nmea_types(qmi_client_type client, uint32_t mask)
{
	uint8_t req[8];
	int pos = 0;
	req[pos++] = 0x01;
	req[pos++] = 0x04; req[pos++] = 0x00;
	memcpy(&req[pos], &mask, 4); pos += 4;

	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_SET_NMEA_TYPES,
		req, pos, resp, sizeof(resp), &resp_len, 5000);
	printf("SET_NMEA_TYPES: rc=%d mask=0x%08x\n", rc, mask);
	if (rc == 0 && resp_len > 0)
		hexdump("  response", resp, resp_len);
	return rc;
}

/*
 * Inject a coarse known position to seed the GNSS engine.
 *
 * Message 0x0039: QMI_LOC_INJECT_POSITION_REQ_V02
 *   TLV 0x01: latitude  (double, WGS84 degrees, mandatory)
 *   TLV 0x02: longitude (double, WGS84 degrees, mandatory)
 *   TLV 0x03: horUncCircular (float, meters, optional)
 *   TLV 0x04: horConfidence  (uint8, 1-99%, optional)
 *
 * Seeding the engine with the approximate location (e.g. within 10 km)
 * dramatically narrows the satellite search space and reduces TTFF.
 */
static int inject_position(qmi_client_type client,
                           double lat, double lon, float hacc_m)
{
	uint8_t  req[64];
	int      pos = 0;

	/*
	 * QMI LOC INJECT_POSITION uses optional TLV numbers matching the
	 * position event indication (0x0024) -- not mandatory TLV 0x01/02/03.
	 * TLV 0x10: latitude (double)
	 * TLV 0x11: longitude (double)
	 * TLV 0x12: horUncCircular (float, meters)
	 */
	req[pos++] = 0x10;
	req[pos++] = 0x08; req[pos++] = 0x00;
	memcpy(&req[pos], &lat, 8); pos += 8;

	req[pos++] = 0x11;
	req[pos++] = 0x08; req[pos++] = 0x00;
	memcpy(&req[pos], &lon, 8); pos += 8;

	req[pos++] = 0x12;
	req[pos++] = 0x04; req[pos++] = 0x00;
	memcpy(&req[pos], &hacc_m, 4); pos += 4;

	uint8_t resp[64];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_INJECT_POSITION,
		req, pos, resp, sizeof(resp), &resp_len, 5000);

	printf("LOC_INJECT_POSITION: rc=%d lat=%.4f lon=%.4f hacc=%.0fm\n",
	       rc, lat, lon, (double)hacc_m);
	if (rc == 0 && resp_len > 0)
		hexdump("  response", resp, resp_len);
	return rc;
}

/*
 * Parse the mandatory QMI result TLV (type 0x02) from a synchronous response.
 * Every QMI response contains this TLV: [result:u16][error:u16]
 *   result 0 = SUCCESS, result 1 = FAILURE (see error code for reason)
 * Common error codes:
 *   0x0013 = QMI_ERR_ARG_TOO_LONG
 *   0x003A = QMI_ERR_ENCODING
 *
 * Returns 0 on SUCCESS, -1 on FAILURE or parse error.
 * Writes the error code to *out_error if non-NULL.
 */
static int check_qmi_result(const uint8_t *resp, unsigned int resp_len,
                             const char *label, uint16_t *out_error)
{
	if (out_error) *out_error = 0;
	for (size_t i = 0; i + 6 <= resp_len; ) {
		uint8_t  type = resp[i];
		uint16_t len;
		memcpy(&len, &resp[i + 1], 2);
		i += 3;
		if (i + len > resp_len) break;
		if (type == 0x02 && len >= 4) {
			uint16_t result, error;
			memcpy(&result, &resp[i], 2);
			memcpy(&error,  &resp[i + 2], 2);
			if (out_error) *out_error = error;
			if (result != 0) {
				printf("  %s: QMI FAILURE result=%u error=0x%04x\n",
				       label, result, error);
				return -1;
			}
			return 0;
		}
		i += len;
	}
	return 0; /* no result TLV found, assume OK */
}

/*
 * Query the modem's preferred XTRA part size via
 * QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE (0x0034).
 *
 * Response TLV 0x10: qmiLocPredictedOrbitsAllowedSizesStructT_v02
 *   maxFileSizeInBytes (uint32) + maxPartSize (uint32)  -- 8 bytes total
 *
 * Returns maxPartSize in bytes, or 0 if the query fails/is unsupported.
 */
static uint32_t query_xtra_max_part_size(qmi_client_type client)
{
	uint8_t resp[256];
	unsigned int resp_len = 0;
	qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
		client, QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE,
		NULL, 0, resp, sizeof(resp), &resp_len, 5000);

	printf("GET_PREDICTED_ORBITS_DATA_SOURCE: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc != 0 || resp_len == 0)
		return 0;
	hexdump("  response", resp, resp_len);

	for (size_t i = 0; i + 3 <= resp_len; ) {
		uint8_t  type = resp[i];
		uint16_t len;
		memcpy(&len, &resp[i + 1], 2);
		i += 3;
		if (i + len > resp_len) break;
		if (type == 0x10 && len >= 8) {
			uint32_t max_file, max_part;
			memcpy(&max_file, &resp[i],     4);
			memcpy(&max_part, &resp[i + 4], 4);
			printf("  maxFileSizeInBytes=%u  maxPartSize=%u\n",
			       max_file, max_part);
			return max_part;
		}
		i += len;
	}
	return 0;
}

/*
 * XTRA predicted orbit data injection.
 *
 * Message 0x0035: QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_REQ_V02
 *   TLV 0x01: totalSize   (uint32)  - total file size in bytes (mandatory)
 *   TLV 0x02: totalParts  (uint16)  - number of chunks to be sent (mandatory)
 *   TLV 0x03: partNum     (uint16)  - 1-based index of this chunk (mandatory)
 *   TLV 0x04: partData    (uint8[]) - chunk payload, encoded as [count:2][data]
 *   TLV 0x05: formatType  (uint32)  - 0=XTRA1, 1=XTRA2, 2=XTRA3 (optional)
 *
 * The engine returns a 0x0035 IND for each chunk accepted:
 *   TLV 0x01: status       (uint32) - 0=SUCCESS
 *   TLV 0x11: maxPartSize  (uint32) - optimal chunk size in bytes
 *
 * Download XTRA2 file on host before running:
 *   wget http://xtrapath4.izatcloud.net/xtra2.bin -O /tmp/xtra2.bin
 *   adb push /tmp/xtra2.bin /tmp/xtra2.bin
 */
/*
 * MDM9207 firmware limit: QMI_LOC_MAX_PREDICTED_ORBITS_PARTS_V02 = 218.
 * Part 219+ returns QMI_ERR_MALFORMED_MSG (0x0001).
 * xtra2.bin = 60787 bytes → ceil(60787/218) = 279 bytes/chunk → 218 parts.
 * Use 280 to give a clean boundary with the last part at 27 bytes.
 */
#define XTRA_MAX_PARTS          218   /* MDM9207 firmware IDL limit */
#define XTRA_CHUNK_SIZE_DEFAULT 280   /* ceil(60787/218) rounded up */
#define XTRA_MAX_CHUNK_SIZE     2048  /* static buffer upper bound */

static int inject_xtra_data(qmi_client_type client, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		printf("ERROR: Cannot open XTRA file: %s\n", path);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (file_size <= 0 || file_size > 1024 * 1024) {
		printf("ERROR: Unexpected XTRA file size: %ld\n", file_size);
		fclose(f);
		return -1;
	}

	/* Ask the modem how large each chunk can be */
	uint32_t chunk_size = query_xtra_max_part_size(client);
	if (chunk_size == 0 || chunk_size > XTRA_MAX_CHUNK_SIZE) {
		printf("INJECT_XTRA: modem query returned %u, using default %d\n",
		       chunk_size, XTRA_CHUNK_SIZE_DEFAULT);
		chunk_size = XTRA_CHUNK_SIZE_DEFAULT;
	}

	/* Detect format type from filename: xtra.bin→0 (XTRA1), xtra2.bin→1 (XTRA2), xtra3→2 (XTRA3) */
	uint32_t format_type = 0;
	if (strstr(path, "xtra3") != NULL)
		format_type = 2;
	else if (strstr(path, "xtra2") != NULL)
		format_type = 1;

	uint32_t total_size  = (uint32_t)file_size;
	uint16_t total_parts = (uint16_t)((file_size + chunk_size - 1) / chunk_size);

	printf("INJECT_XTRA: file=%s size=%u chunk=%u parts=%u format=XTRA%u\n",
	       path, total_size, chunk_size, total_parts, format_type);

	uint8_t  chunk[XTRA_MAX_CHUNK_SIZE];
	/* TLV overhead: 0x01(7)+0x02(5)+0x03(5)+0x04(3+2+chunk)+0x05(7) = 29+chunk */
	uint8_t  req[XTRA_MAX_CHUNK_SIZE + 64];
	uint8_t  resp[128];

	for (uint16_t part = 1; part <= total_parts; part++) {
		size_t n = fread(chunk, 1, chunk_size, f);
		if (n == 0)
			break;

		int pos = 0;

		/* TLV 0x01: totalSize (uint32) */
		req[pos++] = 0x01;
		req[pos++] = 0x04; req[pos++] = 0x00;
		memcpy(&req[pos], &total_size, 4); pos += 4;

		/* TLV 0x02: totalParts (uint16) */
		req[pos++] = 0x02;
		req[pos++] = 0x02; req[pos++] = 0x00;
		memcpy(&req[pos], &total_parts, 2); pos += 2;

		/* TLV 0x03: partNum (uint16, 1-indexed) */
		req[pos++] = 0x03;
		req[pos++] = 0x02; req[pos++] = 0x00;
		memcpy(&req[pos], &part, 2); pos += 2;

		/* TLV 0x04: partData (uint8<N> variable-length byte array)
		 * QMI IDL wire encoding for uint8 arrays:
		 *   [type:1][tlv_len:2][element_count:2][data:element_count bytes]
		 * The firmware decoder reads the 2-byte count prefix from the TLV
		 * value, validates count <= IDL max, then copies the data.
		 * Without this prefix, the first 2 bytes of XTRA data are
		 * misinterpreted as the count, causing QMI_ERR_ARG_TOO_LONG. */
		req[pos++] = 0x04;
		uint16_t data_len = (uint16_t)n;
		uint16_t tlv_len  = 2 + data_len;  /* 2-byte count + data */
		memcpy(&req[pos], &tlv_len, 2); pos += 2;  /* TLV length field */
		memcpy(&req[pos], &data_len, 2); pos += 2;  /* element count (uint16) */
		memcpy(&req[pos], chunk, n);     pos += n;

		/*
		 * TLV 0x05: formatType (uint32) - defined in QMI LOC IDL v2.35+
		 *   0=XTRA1, 1=XTRA2, 2=XTRA3
		 *
		 * OMITTED: MDM9207 firmware is a strict TLV decoder — it returns
		 * QMI_ERR_ENCODING (0x3A) for any optional TLV not present in
		 * its compiled IDL schema, even though the QMI spec mandates that
		 * unknown optional TLVs must be silently ignored.
		 * Sending TLV 0x05 causes ENCODING on every chunk regardless
		 * of chunk size (which led to the incorrect "IDL max < 32" theory).
		 * Without TLV 0x05 the modem accepts XTRA2 data natively.
		 * (void)format_type; */

		unsigned int resp_len = 0;
		qmi_client_error_type rc = qmi_client_send_raw_msg_sync(
			client, QMI_LOC_INJECT_PREDICTED_ORBITS,
			req, pos, resp, sizeof(resp), &resp_len, 10000);

		uint16_t qmi_error = 0;
		int qrc = check_qmi_result(resp, resp_len, "XTRA_PART", &qmi_error);

		if (part == 1 || part == total_parts || rc != 0 || qrc != 0) {
			printf("  XTRA part %u/%u (%zu bytes): rc=%d qrc=%d resp_len=%u\n",
			       part, total_parts, n, rc, qrc, resp_len);
			if (resp_len > 0)
				hexdump("    xtra_resp", resp, resp_len);
		}
		if (rc != 0 || qrc != 0) {
			printf("ERROR: XTRA injection failed at part %u (rc=%d qrc=%d error=0x%04x)\n",
			       part, rc, qrc, qmi_error);
			fclose(f);
			return rc != 0 ? rc : -1;
		}

		/* Brief yield - let the engine digest each chunk */
		usleep(50000);  /* 50 ms */
	}

	fclose(f);
	printf("INJECT_XTRA: all %u parts sent, waiting for engine to validate...\n",
	       total_parts);
	sleep(2);  /* give engine time to verify XTRA CRC/validity */
	return 0;
}

static int test_dms(void) {
	qmi_client_type client = NULL;
	qmi_idl_service_object_type svc_obj;
	qmi_client_error_type rc;

	printf("=== Testing DMS service ===\n");

	svc_obj = &dms_qmi_idl_service_object_v01;
	dump_service_object("DMS", svc_obj);

	rc = qmi_client_init_instance(svc_obj, 0, NULL, NULL, NULL, 10000, &client);
	if (rc != QMI_NO_ERR) {
		printf("ERROR: qmi_client_init_instance(DMS) failed: %d\n", rc);
		return -1;
	}
	printf("DMS client initialized: %p\n", client);

	/* Send DMS_GET_DEVICE_MFR (0x0021) - empty request */
	uint8_t resp[512];
	unsigned int resp_len = 0;
	memset(resp, 0, sizeof(resp));
	rc = qmi_client_send_raw_msg_sync(client, 0x0021, NULL, 0,
		resp, sizeof(resp), &resp_len, 5000);
	printf("DMS_GET_DEVICE_MFR: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == QMI_NO_ERR) {
		hexdump("  response", resp, resp_len > 0 ? resp_len : 64);
	}

	/* DMS_GET_DEVICE_MODEL (0x0022) */
	memset(resp, 0, sizeof(resp));
	resp_len = 0;
	rc = qmi_client_send_raw_msg_sync(client, 0x0022, NULL, 0,
		resp, sizeof(resp), &resp_len, 5000);
	printf("DMS_GET_DEVICE_MODEL: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == QMI_NO_ERR) {
		hexdump("  response", resp, resp_len > 0 ? resp_len : 64);
	}

	qmi_client_release(client);
	printf("DMS client released\n");
	return 0;
}

static int test_loc(const char *xtra_path) {
	qmi_client_type client = NULL;
	qmi_idl_service_object_type svc_obj;
	qmi_client_error_type rc;

	printf("\n=== Testing LOC service (fake service object) ===\n");

	/* Copy library_version from DMS to match what the modem expects */
	fake_loc_svc_obj.library_version = dms_qmi_idl_service_object_v01.library_version;
	svc_obj = &fake_loc_svc_obj;
	dump_service_object("LOC (fake)", svc_obj);

	printf("Calling qmi_client_init_instance for LOC (svc_id=0x10)...\n");
	rc = qmi_client_init_instance(svc_obj, 0, loc_ind_cb, NULL, NULL, 10000, &client);
	if (rc == QMI_TIMEOUT_ERR) {
		printf("LOC init TIMED OUT - service 0x10 may not exist on DSP\n");
		return -1;
	}
	if (rc != QMI_NO_ERR) {
		printf("ERROR: qmi_client_init_instance(LOC) failed: %d\n", rc);
		return -1;
	}
	printf("LOC client initialized: %p\n", client);

	/* Enable all NMEA sentence types (incl. PQXFI/PSTIS extended) */
	set_nmea_types(client, QMI_LOC_NMEA_TYPE_ALL);

	/* Query current engine state */
	printf("\n--- Querying engine configuration ---\n");
	query_operation_mode(client);
	query_engine_lock(client);

	/* Configure engine: unlock and set standalone mode */
	printf("\n--- Configuring engine ---\n");
	set_engine_lock(client, QMI_LOC_LOCK_NONE);
	set_operation_mode(client, QMI_LOC_OPER_MODE_STANDALONE);

	/* Inject current UTC time to reduce cold-start TTFF */
	inject_utc_time(client);

	/*
	 * Inject a coarse position seed.
	 * This tells the engine which part of the sky to search, reducing
	 * the satellite acquisition time from ~12 min to ~30-60 seconds.
	 * Replace YOUR_LAT/YOUR_LON with your approximate coordinates.
	 * Uncertainty radius of 10 km, so rough values are fine.
	 */
	inject_position(client, YOUR_LAT, YOUR_LON, 10000.0f);

	/* Inject XTRA predicted orbit data if a file was provided */
	if (xtra_path) {
		int xrc = inject_xtra_data(client, xtra_path);
		if (xrc != 0)
			printf("WARN: XTRA injection failed (%d), continuing without it\n", xrc);
	} else {
		printf("(no XTRA file provided; pass path as argv[1] to inject orbit data)\n");
	}

	/* LOC_REG_EVENTS: register for position + NMEA + SV info */
	printf("Sending LOC_REG_EVENTS...\n");
	uint8_t reg_req[16];
	memset(reg_req, 0, sizeof(reg_req));
	reg_req[0] = 0x01;
	reg_req[1] = 0x08; reg_req[2] = 0x00;
	uint64_t mask = QMI_LOC_EVENT_MASK_POSITION |
	                QMI_LOC_EVENT_MASK_GNSS_SV_INFO |
	                QMI_LOC_EVENT_MASK_NMEA |
	                QMI_LOC_EVENT_MASK_ENGINE_STATE |
	                QMI_LOC_EVENT_MASK_FIX_SESSION;
	memcpy(&reg_req[3], &mask, 8);

	uint8_t resp[512];
	unsigned int resp_len = 0;
	memset(resp, 0, sizeof(resp));
	rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_REG_EVENTS,
		reg_req, 11, resp, sizeof(resp), &resp_len, 5000);
	printf("LOC_REG_EVENTS: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == QMI_NO_ERR) hexdump("  response", resp, resp_len > 0 ? resp_len : 32);

	/*
	 * LOC_START: start positioning session.
	 * TLV 0x01: session_id (uint8) - mandatory
	 * TLV 0x10: fix_recurrence (uint32) - optional, 1=periodic, 2=single
	 * TLV 0x11: horizontalAccuracyLevel (uint32) - 1=LOW, 2=MED, 3=HIGH
	 * TLV 0x12: intermediateReportState (uint32) - optional, 1=on, 2=off
	 * TLV 0x13: minInterval (uint32 ms) - optional
	 */
	printf("Sending LOC_START...\n");
	uint8_t start_req[48];
	int pos = 0;
	memset(start_req, 0, sizeof(start_req));
	/* TLV 0x01: session_id = 1 (uint8) */
	start_req[pos++] = 0x01;
	start_req[pos++] = 0x01; start_req[pos++] = 0x00;
	start_req[pos++] = 0x01;
	/* TLV 0x10: fix_recurrence = 1 (PERIODIC) */
	start_req[pos++] = 0x10;
	start_req[pos++] = 0x04; start_req[pos++] = 0x00;
	uint32_t recurrence = 1; /* eQMI_LOC_RECURRENCE_PERIODIC */
	memcpy(&start_req[pos], &recurrence, 4); pos += 4;
	/* TLV 0x11: horizontalAccuracyLevel = 1 (LOW) - accept weaker signals */
	start_req[pos++] = 0x11;
	start_req[pos++] = 0x04; start_req[pos++] = 0x00;
	uint32_t accuracy = 1; /* LOW - most permissive */
	memcpy(&start_req[pos], &accuracy, 4); pos += 4;
	/* TLV 0x13: minInterval = 1000 ms */
	start_req[pos++] = 0x13;
	start_req[pos++] = 0x04; start_req[pos++] = 0x00;
	uint32_t interval = 1000;
	memcpy(&start_req[pos], &interval, 4); pos += 4;

	memset(resp, 0, sizeof(resp));
	resp_len = 0;
	rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_START,
		start_req, pos, resp, sizeof(resp), &resp_len, 5000);
	printf("LOC_START: rc=%d resp_len=%u\n", rc, resp_len);
	if (rc == QMI_NO_ERR) hexdump("  response", resp, resp_len > 0 ? resp_len : 32);

	/* Wait for indications */
	printf("\nWaiting for LOC indications (Ctrl+C to stop)...\n");
	printf("(Note: may take time for satellite acquisition)\n\n");

	int seconds = 0;
	while (g_running && seconds < 300) {
		sleep(1);
		seconds++;
		if (seconds % 30 == 0) {
			printf("[%ds] Waiting for indications...\n", seconds);
		}
	}
	if (seconds >= 300 && g_running) {
		printf("Timed out after 300s\n");
	}

	/* LOC_STOP */
	printf("\nSending LOC_STOP...\n");
	uint8_t stop_req[8];
	memset(stop_req, 0, sizeof(stop_req));
	stop_req[0] = 0x01;
	stop_req[1] = 0x01; stop_req[2] = 0x00;
	stop_req[3] = 0x01;

	memset(resp, 0, sizeof(resp));
	resp_len = 0;
	rc = qmi_client_send_raw_msg_sync(client, QMI_LOC_STOP,
		stop_req, 4, resp, sizeof(resp), &resp_len, 5000);
	printf("LOC_STOP: rc=%d\n", rc);

	qmi_client_release(client);
	printf("LOC client released\n");
	return 0;
}

int main(int argc, char *argv[]) {
	const char *xtra_path = (argc > 1) ? argv[1] : NULL;

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	printf("QMI LOC Test - using CCI library with fake LOC service object\n");
	printf("Target: Orbic RC400L (MDM9207)\n");
	if (xtra_path)
		printf("XTRA file: %s\n", xtra_path);
	printf("\n");

	if (test_dms() != 0) {
		printf("\nDMS test failed - QMI CCI framework not working\n");
		return 1;
	}

	printf("\nDMS test passed - CCI framework is working\n");
	test_loc(xtra_path);

	return 0;
}
