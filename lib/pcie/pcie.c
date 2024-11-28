/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <debug.h>
#include <mmio.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <pcie_spec.h>
#include <platform.h>
#include <tftf_lib.h>

#define PCIE_DEBUG	VERBOSE

const struct pcie_info_table *g_pcie_info_table;
pcie_device_bdf_table_t *g_pcie_bdf_table;

pcie_device_bdf_table_t pcie_bdf_table[PCIE_DEVICE_BDF_TABLE_SZ];

uintptr_t pcie_cfg_addr(uint32_t bdf)
{
	uint32_t bus = PCIE_EXTRACT_BDF_BUS(bdf);
	uint32_t dev = PCIE_EXTRACT_BDF_DEV(bdf);
	uint32_t func = PCIE_EXTRACT_BDF_FUNC(bdf);
	uint32_t segment = PCIE_EXTRACT_BDF_SEG(bdf);
	uint32_t cfg_addr;
	uintptr_t ecam_base = 0;
	unsigned int i = 0;

	assert((bus < PCIE_MAX_BUS) && (dev < PCIE_MAX_DEV) && (func < PCIE_MAX_FUNC));
	assert(g_pcie_info_table != NULL);

	while (i < g_pcie_info_table->num_entries) {
		/* Derive ECAM specific information */
		const pcie_info_block_t *block = &g_pcie_info_table->block[i];

		if ((bus >= block->start_bus_num) &&
			(bus <= block->end_bus_num) &&
			(segment == block->segment_num)) {
			ecam_base = block->ecam_base;
			break;
		}
		i++;
	}

	assert(ecam_base != 0);

	/*
	 * There are 8 functions / device
	 * 32 devices / Bus and each has a 4KB config space
	 */
	cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE) +
		(dev * PCIE_MAX_FUNC * PCIE_CFG_SIZE) + (func * PCIE_CFG_SIZE);

	return ecam_base + cfg_addr;
}

/*
 * @brief  This API reads 32-bit data from PCIe config space pointed by Bus,
 *	   Device, Function and register offset.
 *	   1. Caller       -  Test Suite
 *	   2. Prerequisite -  pcie_create_info_table
 * @param  bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
 * @param  offset - Register offset within a device PCIe config space
 *
 * @return  32-bit data read from the config space
 */
uint32_t pcie_read_cfg(uint32_t bdf, uint32_t offset)
{
	uintptr_t addr = pcie_cfg_addr(bdf);

	return mmio_read_32(addr + offset);
}

/*
 * @brief  This API writes 32-bit data to PCIe config space pointed by Bus,
 *	   Device, Function and register offset.
 *	   1. Caller       -  Test Suite
 *	   2. Prerequisite -  val_pcie_create_info_table
 * @param  bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
 * @param  offset - Register offset within a device PCIe config space
 * @param  data   - data to be written to the config space
 *
 * @return  None
 */
void pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data)
{
	uintptr_t addr = pcie_cfg_addr(bdf);

	mmio_write_32(addr + offset, data);
}

/*
 * @brief   Check if BDF is PCIe Host Bridge.
 *
 * @param   bdf - Function's Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
 * @return  false If not a Host Bridge, true If it's a Host Bridge.
 */
bool pcie_is_host_bridge(uint32_t bdf)
{
	uint32_t reg_value = pcie_read_cfg(bdf, TYPE01_RIDR);

	if ((HB_BASE_CLASS == ((reg_value >> CC_BASE_SHIFT) & CC_BASE_MASK)) &&
		(HB_SUB_CLASS == ((reg_value >> CC_SUB_SHIFT) & CC_SUB_MASK))) {
		return true;
	}

	return false;
}

/*
 * @brief  Find a Function's config capability offset matching it's input parameter
 *	   cid. cid_offset set to the matching cpability offset w.r.t. zero.
 *
 * @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
 * @param  cid        - Capability ID
 * @param  cid_offset - On return, points to cid offset in Function config space
 * @return PCIE_CAP_NOT_FOUND, if there was a failure in finding required capability.
 *	   PCIE_SUCCESS, if the search was successful.
 */
uint32_t pcie_find_capability(uint32_t bdf, uint32_t cid_type, uint32_t cid,
				uint32_t *cid_offset)
{
	uint32_t reg_value, next_cap_offset;

	if (cid_type == PCIE_CAP) {
		/* Search in PCIe configuration space */
		reg_value = pcie_read_cfg(bdf, TYPE01_CPR);

		next_cap_offset = (reg_value & TYPE01_CPR_MASK);
		while (next_cap_offset != 0) {
			reg_value = pcie_read_cfg(bdf, next_cap_offset);
			if ((reg_value & PCIE_CIDR_MASK) == cid) {
				*cid_offset = next_cap_offset;
				return PCIE_SUCCESS;
			}
			next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) &
						PCIE_NCPR_MASK);
		}
	} else if (cid_type == PCIE_ECAP) {
		/* Search in PCIe extended configuration space */
		next_cap_offset = PCIE_ECAP_START;
		while (next_cap_offset != 0) {
			reg_value = pcie_read_cfg(bdf, next_cap_offset);
			if ((reg_value & PCIE_ECAP_CIDR_MASK) == cid) {
				*cid_offset = next_cap_offset;
				return PCIE_SUCCESS;
			}
			next_cap_offset = ((reg_value >> PCIE_ECAP_NCPR_SHIFT) &
						PCIE_ECAP_NCPR_MASK);
		}
	}

	/* The capability was not found */
	return PCIE_CAP_NOT_FOUND;
}

/*
 * @brief  This API is used as placeholder to check if the bdf
 *	   obtained is valid or not
 *
 * @param  bdf
 * @return true if bdf is valid else false
 */
bool pcie_check_device_valid(uint32_t bdf)
{
	(void) bdf;
	/*
	 * Add BDFs to this function if PCIe tests
	 * need to be ignored for a BDF for any reason
	 */
	return true;
}

/*
 * @brief  Returns whether a PCIe Function is an on-chip peripheral or not
 *
 * @param  bdf	   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
 * @return Returns TRUE if the Function is on-chip peripheral, FALSE if it is
 *	   not an on-chip peripheral
 */
bool pcie_is_onchip_peripheral(uint32_t bdf)
{
	(void)bdf;
	return false;
}

/*
 * @brief  Returns the type of pcie device or port for the given bdf
 *
 * @param  bdf     - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
 * @return Returns (1 << 0b1001) for RCiEP,  (1 << 0b1010) for RCEC,
 *		   (1 << 0b0000) for EP,     (1 << 0b0100) for RP,
 *		   (1 << 0b1100) for iEP_EP, (1 << 0b1011) for iEP_RP,
 *		   (1 << PCIECR[7:4]) for any other device type.
 */
uint32_t pcie_device_port_type(uint32_t bdf)
{
	uint32_t pciecs_base, reg_value, dp_type;

	/*
	 * Get the PCI Express Capability structure offset and
	 * use that offset to read pci express capabilities register
	 */
	pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
	reg_value = pcie_read_cfg(bdf, pciecs_base + CIDR_OFFSET);

	/* Read Device/Port bits [7:4] in Function's PCIe Capabilities register */
	dp_type = (reg_value >> ((PCIECR_OFFSET - CIDR_OFFSET)*8 +
				PCIECR_DPT_SHIFT)) & PCIECR_DPT_MASK;
	dp_type = (1 << dp_type);

	/* Check if the device/port is an on-chip peripheral */
	if (pcie_is_onchip_peripheral(bdf)) {
		if (dp_type == EP) {
			dp_type = iEP_EP;
		} else if (dp_type == RP) {
			dp_type = iEP_RP;
		}
	}

	/* Return device/port type */
	return dp_type;
}

/*
 * @brief  Returns BDF of the upstream Root Port of a pcie device function.
 *
 * @param  bdf	     - Function's Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
 * @param  usrp_bdf  - Upstream Rootport bdf in PCIE_CREATE_BDF format
 * @return 0 for success, 1 for failure.
 */
uint32_t pcie_get_rootport(uint32_t bdf, uint32_t *rp_bdf)
{
	uint32_t seg_num, sec_bus, sub_bus;
	uint32_t reg_value, dp_type, index = 0;

	dp_type = pcie_device_port_type(bdf);

	PCIE_DEBUG("DP type 0x%x\n", dp_type);

	/* If the device is RP or iEP_RP, set its rootport value to same */
	if ((dp_type == RP) || (dp_type == iEP_RP)) {
		*rp_bdf = bdf;
		return 0;
	}

	/* If the device is RCiEP and RCEC, set RP as 0xff */
	if ((dp_type == RCiEP) || (dp_type == RCEC)) {
		*rp_bdf = 0xffffffff;
		return 1;
	}

	assert(g_pcie_bdf_table != NULL);

	while (index < g_pcie_bdf_table->num_entries) {
		*rp_bdf = g_pcie_bdf_table->device[index++].bdf;

		/*
		 * Extract Secondary and Subordinate Bus numbers of the
		 * upstream Root port and check if the input function's
		 * bus number falls within that range.
		 */
		reg_value = pcie_read_cfg(*rp_bdf, TYPE1_PBN);
		seg_num = PCIE_EXTRACT_BDF_SEG(*rp_bdf);
		sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
		sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);
		dp_type = pcie_device_port_type(*rp_bdf);

		if (((dp_type == RP) || (dp_type == iEP_RP)) &&
			(sec_bus <= PCIE_EXTRACT_BDF_BUS(bdf)) &&
			(sub_bus >= PCIE_EXTRACT_BDF_BUS(bdf)) &&
			(seg_num == PCIE_EXTRACT_BDF_SEG(bdf)))
			return 0;
		}

		/* Return failure */
		ERROR("PCIe Hierarchy fail: RP of bdf 0x%x not found\n", bdf);
		*rp_bdf = 0;
		return 1;
}

/*
 * @brief  Sanity checks that all Endpoints must have a Rootport
 *
 * @param  None
 * @return 0 if sanity check passes, 1 if sanity check fails
 */
static uint32_t pcie_populate_device_rootport(void)
{
	uint32_t bdf, rp_bdf;
	pcie_device_bdf_table_t *bdf_tbl_ptr = g_pcie_bdf_table;

	assert(bdf_tbl_ptr != NULL);

	for (unsigned int tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries;
								tbl_index++) {
		bdf = bdf_tbl_ptr->device[tbl_index].bdf;

		/* Checks if the BDF has RootPort */
		pcie_get_rootport(bdf, &rp_bdf);

		bdf_tbl_ptr->device[tbl_index].rp_bdf = rp_bdf;
		PCIE_DEBUG("Dev bdf: 0x%x RP bdf: 0x%x\n", bdf, rp_bdf);
	}

	return 0;
}

/*
 * @brief  Returns the BDF Table pointer
 *
 * @param  None
 *
 * @return BDF Table pointer
 */
pcie_device_bdf_table_t *pcie_get_bdf_table(void)
{
	assert(g_pcie_bdf_table != NULL);

	return g_pcie_bdf_table;
}

/*
 * @brief   This API creates the device bdf table from enumeration
 *
 * @param   None
 *
 * @return  None
 */
void pcie_create_device_bdf_table(void)
{
	uint32_t seg_num, start_bus, end_bus;
	uint32_t bus_index, dev_index, func_index, ecam_index;
	uint32_t bdf, reg_value, cid_offset, status;

	assert(g_pcie_bdf_table != NULL);

	g_pcie_bdf_table->num_entries = 0;

	assert(g_pcie_info_table != NULL);
	assert(g_pcie_info_table->num_entries != 0);

	for (ecam_index = 0; ecam_index < g_pcie_info_table->num_entries; ecam_index++) {
		/* Derive ECAM specific information */
		const pcie_info_block_t *block = &g_pcie_info_table->block[ecam_index];

		seg_num = block->segment_num;
		start_bus = block->start_bus_num;
		end_bus = block->end_bus_num;

		/* Iterate over all buses, devices and functions in this ecam */
		for (bus_index = start_bus; bus_index <= end_bus; bus_index++) {
			for (dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++) {
				for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++) {
					/* Form BDF using seg, bus, device, function numbers */
					bdf = PCIE_CREATE_BDF(seg_num, bus_index, dev_index,
										func_index);

					/* Probe PCIe device Function with this BDF */
					reg_value = pcie_read_cfg(bdf, TYPE01_VIDR);

					/* Store the Function's BDF if there was a valid response */
					if (reg_value != PCIE_UNKNOWN_RESPONSE) {
						/* Skip if the device is a host bridge */
						if (pcie_is_host_bridge(bdf)) {
							continue;
						}

						/* Skip if the device is a PCI legacy device */
						if (pcie_find_capability(bdf, PCIE_CAP,
							CID_PCIECS,  &cid_offset) != PCIE_SUCCESS) {
							continue;
						}

						status = pcie_check_device_valid(bdf);
						if (!status) {
							continue;
						}

						g_pcie_bdf_table->device[
							g_pcie_bdf_table->num_entries++].bdf = bdf;
					}
				}
			}
		}
	}

	/* Sanity Check : Confirm all EP (normal, integrated) have a rootport */
	pcie_populate_device_rootport();
	INFO("Number of BDFs found     : %u\n", g_pcie_bdf_table->num_entries);
}

/*
 * @brief  Returns the header type of the input pcie device function
 *
 * @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
 * @return TYPE0_HEADER for functions with Type 0 config space header,
 *         TYPE1_HEADER for functions with Type 1 config space header,
 */
uint32_t pcie_function_header_type(uint32_t bdf)
{
	/* Read four bytes of config space starting from cache line size register */
	uint32_t reg_value = pcie_read_cfg(bdf, TYPE01_CLSR);

	/* Extract header type register value */
	reg_value = ((reg_value >> TYPE01_HTR_SHIFT) & TYPE01_HTR_MASK);

	/* Header layout bits within header type register indicate the header type */
	return ((reg_value >> HTR_HL_SHIFT) & HTR_HL_MASK);
}

/*
 * @brief  Returns the ECAM address of the input PCIe function
 *
 * @param  bdf   - Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
 * @return ECAM address if success, else NULL address
 */
uintptr_t pcie_get_ecam_base(uint32_t bdf)
{
	uint8_t ecam_index = 0, sec_bus = 0, sub_bus;
	uint16_t seg_num = (uint16_t)PCIE_EXTRACT_BDF_SEG(bdf);
	uint32_t reg_value;
	uintptr_t ecam_base = 0;

	assert(g_pcie_info_table != NULL);

	while (ecam_index < g_pcie_info_table->num_entries) {
		/* Derive ECAM specific information */
		const pcie_info_block_t *block = &g_pcie_info_table->block[ecam_index];

		if (seg_num == block->segment_num) {
			if (pcie_function_header_type(bdf) == TYPE0_HEADER) {
				/* Return ecam_base if Type0 Header */
				ecam_base = block->ecam_base;
				break;
			}

			/* Check for Secondary/Subordinate bus if Type1 Header */
			reg_value = pcie_read_cfg(bdf, TYPE1_PBN);
			sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
			sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

			if ((sec_bus >= block->start_bus_num) &&
			    (sub_bus <= block->end_bus_num)) {
				ecam_base = block->ecam_base;
				break;
			}
		}
		ecam_index++;
	}

	return ecam_base;
}

/*
 * @brief   This API prints all the PCIe Devices info
 * 1. Caller	   -  Validation layer.
 * 2. Prerequisite -  val_pcie_create_info_table()
 * @param   None
 * @return  None
 */
void pcie_print_device_info(void)
{
	uint32_t bdf, dp_type;
	uint32_t tbl_index = 0;
	uint32_t ecam_index = 0;
	uint32_t ecam_base, ecam_start_bus, ecam_end_bus;
	pcie_device_bdf_table_t *bdf_tbl_ptr = g_pcie_bdf_table;
	uint32_t num_rciep __unused = 0, num_rcec __unused = 0;
	uint32_t num_iep __unused = 0, num_irp __unused = 0;
	uint32_t num_ep __unused = 0, num_rp __unused = 0;
	uint32_t num_dp __unused = 0, num_up __unused = 0;
	uint32_t num_pcie_pci __unused = 0, num_pci_pcie __unused = 0;
	uint32_t bdf_counter;

	assert(bdf_tbl_ptr != NULL);

	if (bdf_tbl_ptr->num_entries == 0) {
		INFO("BDF Table: No RCiEP or iEP found\n");
		return;
	}

	for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++) {
		bdf = bdf_tbl_ptr->device[tbl_index].bdf;
		dp_type = pcie_device_port_type(bdf);

		switch (dp_type) {
		case RCiEP:
			num_rciep++;
			break;
		case RCEC:
			num_rcec++;
			break;
		case EP:
			num_ep++;
			break;
		case RP:
			num_rp++;
			break;
		case iEP_EP:
			num_iep++;
			break;
		case iEP_RP:
			num_irp++;
			break;
		case UP:
			num_up++;
			break;
		case DP:
			num_dp++;
			break;
		case PCI_PCIE:
			num_pci_pcie++;
			break;
		case PCIE_PCI:
			num_pcie_pci++;
			break;
		default:
			ERROR("Unknown dp_type 0x%x\n", dp_type);
		}
	}

	INFO("Number of RCiEP          : %u\n", num_rciep);
	INFO("Number of RCEC           : %u\n", num_rcec);
	INFO("Number of EP             : %u\n", num_ep);
	INFO("Number of RP             : %u\n", num_rp);
	INFO("Number of iEP_EP         : %u\n", num_iep);
	INFO("Number of iEP_RP         : %u\n", num_irp);
	INFO("Number of UP of switch   : %u\n", num_up);
	INFO("Number of DP of switch   : %u\n", num_dp);
	INFO("Number of PCI/PCIe Bridge: %u\n", num_pci_pcie);
	INFO("Number of PCIe/PCI Bridge: %u\n", num_pcie_pci);

	assert(g_pcie_info_table != NULL);

	while (ecam_index < g_pcie_info_table->num_entries) {

		/* Derive ECAM specific information */
		const pcie_info_block_t *block = &g_pcie_info_table->block[ecam_index];

		ecam_base = block->ecam_base;
		ecam_start_bus = block->start_bus_num;
		ecam_end_bus = block->end_bus_num;
		tbl_index = 0;
		bdf_counter = 0;

		INFO("ECAM %u: base 0x%x\n", ecam_index, ecam_base);

		while (tbl_index < bdf_tbl_ptr->num_entries) {
			uint32_t seg_num, bus_num, dev_num, func_num;
			uint32_t device_id __unused, vendor_id __unused, reg_value;
			uint32_t bdf, dev_ecam_base;

			bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
			seg_num  = PCIE_EXTRACT_BDF_SEG(bdf);
			bus_num  = PCIE_EXTRACT_BDF_BUS(bdf);
			dev_num  = PCIE_EXTRACT_BDF_DEV(bdf);
			func_num = PCIE_EXTRACT_BDF_FUNC(bdf);

			reg_value = pcie_read_cfg(bdf, TYPE01_VIDR);
			device_id = (reg_value >> TYPE01_DIDR_SHIFT) & TYPE01_DIDR_MASK;
			vendor_id = (reg_value >> TYPE01_VIDR_SHIFT) & TYPE01_VIDR_MASK;

			dev_ecam_base = pcie_get_ecam_base(bdf);

			if ((ecam_base == dev_ecam_base) &&
				(bus_num >= ecam_start_bus) &&
				(bus_num <= ecam_end_bus)) {
				bdf_counter = 1;
				bdf = PCIE_CREATE_BDF(seg_num, bus_num, dev_num, func_num);
				INFO("  BDF: 0x%x\n", bdf);
				INFO("  Seg: 0x%x Bus: 0x%x Dev: 0x%x "
					"Func: 0x%x Dev ID: 0x%x Vendor ID: 0x%x\n",
					seg_num, bus_num, dev_num, func_num,
					device_id, vendor_id);
			}
		}

		if (bdf_counter == 0) {
			INFO("  No BDF devices in ECAM region index %d\n", ecam_index);
		}

		ecam_index++;
	}
}

/*
 * @brief	Create PCIe table and PCI enumeration
 * @param	void
 * @return	void
 */
void pcie_create_info_table(void)
{
	unsigned int num_ecam;

	INFO("Creating PCIe info table\n");

	g_pcie_info_table = plat_pcie_get_info_table();
	if (g_pcie_info_table == NULL) {
		ERROR("PCIe info not returned by platform\n");
		panic();
	}

	g_pcie_bdf_table = pcie_bdf_table;

	num_ecam = g_pcie_info_table->num_entries;
	INFO("Number of ECAM regions   : %u\n", num_ecam);
	if ((num_ecam == 0) || (num_ecam > MAX_PCIE_INFO_ENTRIES)) {
		ERROR("PCIe info entries invalid\n");
		panic();
	}
	pcie_create_device_bdf_table();
	pcie_print_device_info();
}

void pcie_init(void)
{
	static bool is_init;

	/* Create PCIe table and enumeration */
	if (!is_init) {
		pcie_create_info_table();
		is_init = true;
	}
}
