/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 * @brief Common interface between host and RPU
 *
 */
#ifndef __NRF_WIFI_HOST_RPU_COMMON_IFACE_H__
#define __NRF_WIFI_HOST_RPU_COMMON_IFACE_H__

#include "rpu_if.h"

#include "pack_def.h"

#define NRF_WIFI_UMAC_VER(version) (((version)&0xFF000000) >> 24)
#define NRF_WIFI_UMAC_VER_MAJ(version) (((version)&0x00FF0000) >> 16)
#define NRF_WIFI_UMAC_VER_MIN(version) (((version)&0x0000FF00) >> 8)
#define NRF_WIFI_UMAC_VER_EXTRA(version) (((version)&0x000000FF) >> 0)
#ifdef SOC_WEZEN
#define RPU_MEM_UMAC_BOOT_SIG 0x20080000
#define RPU_MEM_UMAC_VER 0x20080004
#define RPU_MEM_UMAC_PEND_Q_BMP 0x20080008
#define RPU_MEM_UMAC_CMD_ADDRESS 0x200807A8
#define RPU_MEM_UMAC_EVENT_ADDRESS 0x20080E28
#else
#define RPU_MEM_UMAC_BOOT_SIG 0xB0000000
#define RPU_MEM_UMAC_VER 0xB0000004
#define RPU_MEM_UMAC_PEND_Q_BMP 0xB0000008
#define RPU_MEM_UMAC_CMD_ADDRESS 0xB00007A8
#define RPU_MEM_UMAC_EVENT_ADDRESS 0xB0000E28
#endif
#define RPU_MEM_UMAC_PATCH_BIN 0x8008C000
#define RPU_MEM_UMAC_PATCH_BIMG 0x80099400

#define NRF_WIFI_UMAC_BOOT_SIG 0x5A5A5A5A
#define NRF_WIFI_UMAC_ROM_PATCH_OFFSET (RPU_MEM_UMAC_PATCH_BIMG - RPU_ADDR_UMAC_CORE_RET_START)
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_0 0x3c1a8000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_1 0x275a0000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_2 0x03400008
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_3 0x00000000

/**
 * @brief This enum defines the different categories of messages that can be exchanged between
 *  the Host and the RPU.
 *
 */
enum nrf_wifi_host_rpu_msg_type {
	/** System interface messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
	/** Unused */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SUPPLICANT,
	/** Data path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
	/** Control path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC
};
/**
 * @brief This structure defines the common message header used to encapsulate each message
 *  exchanged between the Host and UMAC.
 *
 */

struct host_rpu_msg {
	/** Header */
	struct host_rpu_msg_hdr hdr;
	/** Type of the RPU message see &enum nrf_wifi_host_rpu_msg_type */
	signed int type;
	/** Actual message */
	signed char msg[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VO (1 << 0)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VI (1 << 1)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BE (1 << 2)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BK (1 << 3)

/**
 * @brief This structure represents the bitmap of STA (Station) pending frames in
 *  SoftAP power save mode.
 *
 */

struct sap_pend_frames_bitmap {
	/** STA MAC address */
	unsigned char mac_addr[6];
	/** Pending frames bitmap for each access category */
	unsigned char pend_frames_bitmap;
} __NRF_WIFI_PKD;
#define NRF_WIFI_MAX_SAP_CLIENTS 4
/**
 * @brief This structure represents the information related to UMAC.
 *
 */
struct host_rpu_umac_info {
	/** Boot status signature */
	unsigned int boot_status;
	/** UMAC version */
	unsigned int version;
	/** @ref sap_pend_frames_bitmap */
	struct sap_pend_frames_bitmap sap_bitmap[NRF_WIFI_MAX_SAP_CLIENTS];
	/** Hardware queues info &enum host_rpu_hpqm_info */
	struct host_rpu_hpqm_info hpqm_info;
	/** OTP params */
	unsigned int info_part;
	/** OTP params */
	unsigned int info_variant;
	/** OTP params */
	unsigned int info_lromversion;
	/** OTP params */
	unsigned int info_uromversion;
	/** OTP params */
	unsigned int info_uuid[4];
	/** OTP params */
	unsigned int info_spare0;
	/** OTP params */
	unsigned int info_spare1;
	/** OTP params */
	unsigned int mac_address0[2];
	/** OTP params */
	unsigned int mac_address1[2];
	/** OTP params */
	unsigned int calib[9];
} __NRF_WIFI_PKD;

#ifdef SOFT_HPQM

#define HOST_RPU_CMD_BUFFERS 4
#define HOST_RPU_EVENT_BUFFERS 7
#define HOST_RPU_TX_DESC 12

struct soft_hpqm_info {
	volatile unsigned int host_cmd_free_index;
	volatile unsigned int rpu_cmd_busy_index;
	volatile unsigned int host_event_busy_index;
	volatile unsigned int rpu_event_free_index;
	volatile unsigned int host_tx_cmd_busy_index;
	volatile unsigned int rpu_tx_cmd_busy_index;
	volatile unsigned int host_tx_done_busy_index;
	volatile unsigned int rpu_tx_done_busy_index;
	volatile unsigned int cmd_free_buffs[HOST_RPU_CMD_BUFFERS];
	volatile unsigned int cmd_busy_buffs[HOST_RPU_CMD_BUFFERS];
	volatile unsigned int event_free_buffs[HOST_RPU_EVENT_BUFFERS];
	volatile unsigned int event_busy_buffs[HOST_RPU_EVENT_BUFFERS];
	volatile unsigned int tx_cmd_buffs[HOST_RPU_TX_DESC];
	volatile unsigned int tx_done_buffs[HOST_RPU_TX_DESC];
}; __NRF_WIFI_PKD;

#define HOST_RPU_GDRAM_START_ADDR     0x20084548
#define HOST_RPU_TX_CMD_START_ADDR    0x200800b8

#endif /* SOFT_HPQM */
#endif /* __NRF_WIFI_HOST_RPU_IFACE_H__ */
