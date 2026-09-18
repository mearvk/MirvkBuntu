/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TAC3_FORMAT_H
#define TAC3_FORMAT_H

#define TAC3_DISK_BLOCK_SIZE              4096u
#define TAC3_DISK_MAGIC_SIZE              8u
#define TAC3_DISK_MAGIC                   "TAC3FS1\0"
#define TAC3_DISK_FORMAT_MAJOR            1u
#define TAC3_DISK_FORMAT_MINOR            0u
#define TAC3_DISK_CHECKSUM_CRC32C         1u
#define TAC3_DISK_STATE_CLEAN             0u
#define TAC3_DISK_STATE_ACTIVE            1u
#define TAC3_DISK_STATE_RECOVERY_REQUIRED 2u
#define TAC3_DISK_MIN_BLOCKS              64ULL

#define TAC3_SB_OFF_MAGIC                 0u
#define TAC3_SB_OFF_MAJOR                 8u
#define TAC3_SB_OFF_MINOR                 10u
#define TAC3_SB_OFF_BLOCK_SIZE            12u
#define TAC3_SB_OFF_TOTAL_BLOCKS          16u
#define TAC3_SB_OFF_UUID                  24u
#define TAC3_SB_OFF_GENERATION            40u
#define TAC3_SB_OFF_MULTITUDE             48u
#define TAC3_SB_OFF_DEVICE_CLASS          52u
#define TAC3_SB_OFF_FILE_START            56u
#define TAC3_SB_OFF_FILE_BLOCKS           64u
#define TAC3_SB_OFF_HEALTH_START          72u
#define TAC3_SB_OFF_HEALTH_BLOCKS         80u
#define TAC3_SB_OFF_ADMIN_START           88u
#define TAC3_SB_OFF_ADMIN_BLOCKS          96u
#define TAC3_SB_OFF_RECOVERY_START        104u
#define TAC3_SB_OFF_RECOVERY_BLOCKS       112u
#define TAC3_SB_OFF_DATA_START            120u
#define TAC3_SB_OFF_STATE                 128u
#define TAC3_SB_OFF_CHECKSUM_ALGORITHM    132u
#define TAC3_SB_OFF_CHECKSUM              (TAC3_DISK_BLOCK_SIZE - 4u)
#define TAC3_DISK_OFF_CHECKSUM            TAC3_SB_OFF_CHECKSUM

#endif
