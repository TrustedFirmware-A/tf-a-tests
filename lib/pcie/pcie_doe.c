/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdlib.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <tftf_lib.h>

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define DOE_INFO(...)	mp_printf(__VA_ARGS__)
#else
#define DOE_INFO(...)
#endif

static int pcie_doe_wait_ready(uint32_t bdf, uint32_t doe_cap_base)
{
	uint32_t value;

	for (unsigned int i = 0; i < PCI_DOE_POLL_LOOP; i++) {
		value = pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG);

		if ((value & DOE_STATUS_BUSY_BIT) != 0) {
			ERROR("DOE Busy bit is set\n");
			return -EBUSY;
		}

		if ((value & DOE_STATUS_ERROR_BIT) != 0) {
			ERROR("DOE Error bit is set\n");
			return -EIO;
		}

		if ((value & DOE_STATUS_READY_BIT) != 0) {
			return 0;
		}

		waitms(PCI_DOE_POLL_TIME);
	}

	ERROR("DOE Timeout, status 0x%x\n", value);
	return -ETIMEDOUT;
}

static const char * const doe_object_type[] = {
	"DOE Discovery",
	"CMA-SPDM",
	"Secured CMA-SPDM",
	/* PCI Express Base Specification Revision 6.1 */
	"CMA/SPDM with Connection ID",
	"Secured CMA/SPDM with Connection ID",
	"Async Message"
};

void print_doe_disc(pcie_doe_disc_resp_t *data)
{
	uint8_t type = data->data_object_type;

	INFO("Vendor ID: 0x%x, ", data->vendor_id);

	if (type >= ARRAY_SIZE(doe_object_type)) {
		DOE_INFO("Unknown type: 0x%x\n", type);
	} else {
		DOE_INFO("%s\n", doe_object_type[type]);
	}
}

static void print_doe_data(uint32_t idx, uint32_t data, bool last)
{
	uint32_t j = idx + DOE_HEADER_LENGTH;

	if (last) {
		if ((j & 7) == 0) {
			INFO(" %08x\n", data);
		} else {
			DOE_INFO(" %08x\n", data);
		}
	} else if ((j & 7) == 0) {
		INFO(" %08x", data);
	} else if ((j & 7) == 7) {
		DOE_INFO(" %08x\n", data);
	} else {
		DOE_INFO(" %08x", data);
	}
}

/*
 * @brief   This API sends DOE request to PCI device.
 * @param   bdf - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
 * @param   doe_cap_base - DOE capability base offset
 * @param   *req_addr - DOE request payload buffer
 * @param   req_len - DOE request payload length in bytes
 *
 * @return  0 on success, negative code on failure
 */
int pcie_doe_send_req(uint32_t header, uint32_t bdf, uint32_t doe_cap_base,
			uint32_t *req_addr, uint32_t req_len)
{
	uint32_t value, i, send_length, rem_length, doe_length;

	value = pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG);
	if ((value & DOE_STATUS_BUSY_BIT) != 0) {
		ERROR("DOE Busy bit is set\n");
		return -EBUSY;
	}

	if ((value & DOE_STATUS_ERROR_BIT) != 0) {
		ERROR("DOE Error bit is set\n");
		return -EIO;
	}

	send_length = req_len >> 2;
	rem_length = req_len & 3;

	/* Calculated adjusted data length in DW */
	doe_length = (rem_length == 0) ? send_length : (send_length + 1);

	INFO(">%08x", header);

	pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG,
								header);
	DOE_INFO(" %08x", doe_length + DOE_HEADER_LENGTH);

	pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG,
				doe_length + DOE_HEADER_LENGTH);
	/* Write data */
	for (i = 0; i < send_length; i++) {
		print_doe_data(i, req_addr[i], false);
		pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG,
								req_addr[i]);
	}

	/* Check for remaining bytes */
	if (rem_length != 0) {
		value = 0;
		(void)memcpy(&value, &req_addr[i], rem_length);
		print_doe_data(i, value, true);
		pcie_write_cfg(bdf, doe_cap_base + DOE_WRITE_DATA_MAILBOX_REG, value);

	} else if (((i + DOE_HEADER_LENGTH) & 7) != 0) {
		DOE_INFO("\n");
	}

	/* Set Go bit */
	pcie_write_cfg(bdf, doe_cap_base + DOE_CTRL_REG, DOE_CTRL_GO_BIT);
	return 0;
}

/*
 * @brief   This API receives DOE response from PCI device.
 * @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
 * @param   doe_cap_base - DOE capability base offset
 * @param   *resp_addr - DOE response payload buffer
 * @param   *resp_len - DOE response payload length in bytes
 *
 * @return  0 on success, negative code on failure
 */
int pcie_doe_recv_resp(uint32_t bdf, uint32_t doe_cap_base,
			uint32_t *resp_addr, uint32_t *resp_len)
{
	uint32_t i, value, length;
	int ret;

	/* Wait for Ready bit */
	ret = pcie_doe_wait_ready(bdf, doe_cap_base);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Reading DOE Header 1:
	 * Vendor ID and Data Object Type
	 */
	value = pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG);
	INFO("<%08x", value);

	/* Indicate a successful transfer of the current data object DW */
	pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);

	/*
	 * Reading DOE Header 2:
	 * Length in DW
	 */
	value = pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG);
	DOE_INFO(" %08x", value);

	pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);

	/* Check value */
	if ((value & PCI_DOE_RESERVED_MASK) != 0) {
		DOE_INFO("\n");
		ERROR("DOE Data Object Header 2 error\n");
		return -EIO;
	}

	/* Value of 00000h indicates 2^18 DW */
	length = (value != 0) ? (value - DOE_HEADER_LENGTH) :
				(PCI_DOE_MAX_LENGTH - DOE_HEADER_LENGTH);

	/* Response payload length in bytes */
	*resp_len = length << 2;

	for (i = 0; i < length; i++) {
		value = pcie_read_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG);
		*resp_addr++ = value;
		print_doe_data(i, value, false);
		pcie_write_cfg(bdf, doe_cap_base + DOE_READ_DATA_MAILBOX_REG, 0);
	}

	if (((i + DOE_HEADER_LENGTH) & 7) != 0) {
		DOE_INFO("\n");
	}

	value = pcie_read_cfg(bdf, doe_cap_base + DOE_STATUS_REG);
	if ((value & (DOE_STATUS_READY_BIT | DOE_STATUS_ERROR_BIT)) != 0) {
		ERROR("DOE Receive error, status 0x%x\n", value);
		return -EIO;
	}

	return 0;
}
