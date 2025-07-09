/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
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
#include <pcie_dvsec_rmeda.h>
#include <platform.h>
#include <plat_pcie_enum.h>
#include <tftf_lib.h>

#define PCIE_DEBUG	VERBOSE

const struct pcie_info_table *g_pcie_info_table;
static pcie_device_bdf_table_t *g_pcie_bdf_table;
static pcie_device_bdf_table_t pcie_bdf_table;

static uint32_t g_pcie_index;
static uint32_t g_enumerate;

/* 64-bit address initialisation */
static uint64_t g_bar64_p_start;
static uint64_t g_rp_bar64_value;
static uint64_t g_bar64_p_max;
static uint32_t g_64_bus, g_bar64_size;

/* 32-bit address initialisation */
static uint32_t g_bar32_np_start;
static uint32_t g_bar32_p_start;
static uint32_t g_rp_bar32_value;
static uint32_t g_bar32_np_max;
static uint32_t g_bar32_p_max;
static uint32_t g_np_bar_size, g_p_bar_size;
static uint32_t g_np_bus, g_p_bus;

static uintptr_t pcie_cfg_addr(uint32_t bdf)
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
static bool pcie_is_host_bridge(uint32_t bdf)
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
static bool pcie_check_device_valid(uint32_t bdf)
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
static bool pcie_is_onchip_peripheral(uint32_t bdf)
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
static uint32_t pcie_device_port_type(uint32_t bdf)
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
 * @return pcie_dev for success, NULL for failure.
 */
static pcie_dev_t *pcie_get_rootport(uint32_t bdf)
{
	uint32_t seg_num, sec_bus, sub_bus;
	uint32_t reg_value, dp_type, index = 0;
	uint32_t rp_bdf;

	dp_type = pcie_device_port_type(bdf);

	PCIE_DEBUG("DP type 0x%x\n", dp_type);

	/* If the device is RP or iEP_RP, set its rootport value to same */
	if ((dp_type == RP) || (dp_type == iEP_RP)) {
		return NULL;
	}

	/* If the device is RCiEP and RCEC, set RP as 0xff */
	if ((dp_type == RCiEP) || (dp_type == RCEC)) {
		return NULL;
	}

	assert(g_pcie_bdf_table != NULL);

	while (index < g_pcie_bdf_table->num_entries) {
		rp_bdf = g_pcie_bdf_table->device[index].bdf;

		/*
		 * Extract Secondary and Subordinate Bus numbers of the
		 * upstream Root port and check if the input function's
		 * bus number falls within that range.
		 */
		reg_value = pcie_read_cfg(rp_bdf, TYPE1_PBN);
		seg_num = PCIE_EXTRACT_BDF_SEG(rp_bdf);
		sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
		sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);
		dp_type = pcie_device_port_type(rp_bdf);

		if (((dp_type == RP) || (dp_type == iEP_RP)) &&
		    (sec_bus <= PCIE_EXTRACT_BDF_BUS(bdf)) &&
		    (sub_bus >= PCIE_EXTRACT_BDF_BUS(bdf)) &&
		    (seg_num == PCIE_EXTRACT_BDF_SEG(bdf))) {
			return &g_pcie_bdf_table->device[index];
		}

		index++;
	}

	/* Return failure */
	ERROR("PCIe Hierarchy fail: RP of bdf 0x%x not found\n", bdf);
	return NULL;
}

/*
 * @brief  Sanity checks that all Endpoints must have a Rootport
 *
 * @param  None
 * @return 0 if sanity check passes, 1 if sanity check fails
 */
static uint32_t pcie_populate_device_rootport(void)
{
	uint32_t bdf;
	pcie_device_bdf_table_t *bdf_tbl_ptr = g_pcie_bdf_table;
	pcie_dev_t *rp_dev;

	assert(bdf_tbl_ptr != NULL);

	for (unsigned int tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries;
								tbl_index++) {
		bdf = bdf_tbl_ptr->device[tbl_index].bdf;

		/* Checks if the BDF has RootPort */
		rp_dev = pcie_get_rootport(bdf);

		bdf_tbl_ptr->device[tbl_index].rp_dev = rp_dev;

		if (rp_dev != NULL) {
			INFO("Dev bdf: 0x%x RP bdf: 0x%x\n", bdf,
			     rp_dev->bdf);
		} else {
			INFO("Dev bdf: 0x%x RP bdf: none\n", bdf);
		}
	}

	return 0;
}

/*
 * @brief  Returns the header type of the input pcie device function
 *
 * @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
 * @return TYPE0_HEADER for functions with Type 0 config space header,
 *         TYPE1_HEADER for functions with Type 1 config space header,
 */
static uint32_t pcie_function_header_type(uint32_t bdf)
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
static uintptr_t pcie_get_ecam_base(uint32_t bdf)
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

static void pcie_devices_init_fields(void)
{
	pcie_device_bdf_table_t *bdf_tbl_ptr = g_pcie_bdf_table;
	pcie_dev_t *pcie_dev;
	uint32_t status;
	uint32_t base;
	uint32_t bdf;

	assert(bdf_tbl_ptr != NULL);

	for (uint32_t i = 0; i < bdf_tbl_ptr->num_entries; i++) {
		pcie_dev = &bdf_tbl_ptr->device[i];
		bdf = pcie_dev->bdf;

		pcie_dev->dp_type = pcie_device_port_type(bdf);
		pcie_dev->ecam_base = pcie_get_ecam_base(bdf);

		/* Has DOE? */
		status = pcie_find_capability(bdf, PCIE_ECAP, ECID_DOE, &base);
		if (status == PCIE_SUCCESS) {
			pcie_dev->cflags |= PCIE_DEV_CFLAG_DOE;
			pcie_dev->doe_cap_base = base;
		}

		/* Has IDE? */
		status = pcie_find_capability(bdf, PCIE_ECAP, ECID_IDE, &base);
		if (status == PCIE_SUCCESS) {
			pcie_dev->cflags |= PCIE_DEV_CFLAG_IDE;
			pcie_dev->ide_cap_base = base;
		}

		if (pcie_dev->dp_type == RP) {
			status = pcie_find_rmeda_capability(bdf, &base);
			if (status == PCIE_SUCCESS) {
				pcie_dev->cflags |= PCIE_DEV_CFLAG_DVSEC_RMEDA;
				pcie_dev->dvsec_rmeda_cap_base = base;
			}
		}
	}
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
static void pcie_create_device_bdf_table(void)
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

						assert(g_pcie_bdf_table->num_entries < PCIE_DEVICES_MAX);
					}
				}
			}
		}
	}

	/* Sanity Check : Confirm all EP (normal, integrated) have a rootport */
	pcie_populate_device_rootport();

	/*
	 * Once devices are enumerated and rootports are assigned, initialize
	 * the rest of pcie_dev fields
	 */
	pcie_devices_init_fields();

	INFO("Number of BDFs found     : %u\n", g_pcie_bdf_table->num_entries);
}

/*
 * @brief   This API prints all the PCIe Devices info
 * 1. Caller	   -  Validation layer.
 * 2. Prerequisite -  val_pcie_create_info_table()
 * @param   None
 * @return  None
 */
static void pcie_print_device_info(void)
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
static void pcie_create_info_table(void)
{
	unsigned int num_ecam;

	INFO("Creating PCIe info table\n");
	g_pcie_bdf_table = &pcie_bdf_table;

	num_ecam = g_pcie_info_table->num_entries;
	INFO("Number of ECAM regions   : %u\n", num_ecam);
	if ((num_ecam == 0) || (num_ecam > MAX_PCIE_INFO_ENTRIES)) {
		ERROR("PCIe info entries invalid\n");
		panic();
	}
	pcie_create_device_bdf_table();
	pcie_print_device_info();
}

static void pal_pci_cfg_write(uint32_t bus, uint32_t dev, uint32_t func,
			      uint32_t offset, uint32_t data)
{
	pcie_write_cfg(PCIE_CREATE_BDF(0, bus, dev, func), offset, data);
}

static void pal_pci_cfg_read(uint32_t bus, uint32_t dev, uint32_t func,
			     uint32_t offset, uint32_t *value)
{
	*value = pcie_read_cfg(PCIE_CREATE_BDF(0, bus, dev, func), offset);
}

/*
 * This API programs the Memory Base and Memeory limit register of the Bus,
 * Device and Function of Type1 Header
 */
static void get_resource_base_32(uint32_t bus, uint32_t dev, uint32_t func,
				 uint32_t bar32_p_base, uint32_t bar32_np_base,
				 uint32_t bar32_p_limit, uint32_t bar32_np_limit)
{
	uint32_t mem_bar_np;
	uint32_t mem_bar_p;

	/* Update the 32 bit NP-BAR start address for the next iteration */
	if (bar32_np_base != g_bar32_np_start) {
		if ((g_bar32_np_start << 12) != 0) {
			g_bar32_np_start = (g_bar32_np_start &
					    MEM_BASE32_LIM_MASK) + BAR_INCREMENT;
		}

		if (bar32_np_limit == g_bar32_np_start) {
			bar32_np_limit = bar32_np_limit - BAR_INCREMENT;
		}

		pal_pci_cfg_read(bus, dev, func, NON_PRE_FET_OFFSET,
				 &mem_bar_np);
		mem_bar_np = ((bar32_np_limit & MEM_BASE32_LIM_MASK) |
			      mem_bar_np);
		pal_pci_cfg_write(bus, dev, func, NON_PRE_FET_OFFSET,
				  mem_bar_np);
	}

	/* Update the 32 bit P-BAR start address for the next iteration */
	if (bar32_p_base != g_bar32_p_start) {
		if ((g_bar32_p_start  << 12) != 0) {
			g_bar32_p_start  = (g_bar32_p_start &
					    MEM_BASE32_LIM_MASK) + BAR_INCREMENT;
		}

		if (bar32_p_limit == g_bar32_p_start) {
			bar32_p_limit = bar32_p_limit - BAR_INCREMENT;
		}

		pal_pci_cfg_read(bus, dev, func, PRE_FET_OFFSET, &mem_bar_p);
		mem_bar_p = ((bar32_p_limit & MEM_BASE32_LIM_MASK) | mem_bar_p);
		pal_pci_cfg_write(bus, dev, func, PRE_FET_OFFSET, mem_bar_p);
	}
}

/*
 * This API programs the Memory Base and Memeory limit register of the Bus,
 * Device and Function of Type1 Header
 */
static void get_resource_base_64(uint32_t bus, uint32_t dev, uint32_t func,
				 uint64_t bar64_p_base, uint64_t g_bar64_p_max)
{
	uint32_t bar64_p_lower32_base = (uint32_t)bar64_p_base;
	uint32_t bar64_p_upper32_base = (uint32_t)(bar64_p_base >> 32);
	uint32_t bar64_p_lower32_limit = (uint32_t)g_bar64_p_max;
	uint32_t bar64_p_upper32_limit = (uint32_t)(g_bar64_p_max >> 32);

	/* Obtain the memory base and memory limit */
	bar64_p_lower32_base = REG_MASK_SHIFT(bar64_p_lower32_base);
	bar64_p_lower32_limit = REG_MASK_SHIFT(bar64_p_lower32_limit);
	uint32_t mem_bar_p = ((bar64_p_lower32_limit << 16) |
			      bar64_p_lower32_base);

	/* Configure Memory base and Memory limit register */
	if ((bar64_p_base != g_bar64_p_max) && (g_bar64_p_start <=
						g_bar64_p_max)) {
		if ((g_bar64_p_start  << 12) != 0) {
			g_bar64_p_start = (g_bar64_p_start &
					   MEM_BASE64_LIM_MASK) + BAR_INCREMENT;
		}

		if (bar64_p_lower32_limit == g_bar64_p_start) {
			bar64_p_lower32_limit = bar64_p_lower32_limit -
				BAR_INCREMENT;
		}

		g_bar64_p_start = (g_bar64_p_start & MEM_BASE64_LIM_MASK) +
			BAR_INCREMENT;

		pal_pci_cfg_write(bus, dev, func, PRE_FET_OFFSET, mem_bar_p);
		pal_pci_cfg_write(bus, dev, func, PRE_FET_OFFSET + 4,
				  bar64_p_upper32_base);
		pal_pci_cfg_write(bus, dev, func, PRE_FET_OFFSET + 8,
				  bar64_p_upper32_limit);
	}
}

static void pcie_rp_program_bar(uint32_t bus, uint32_t dev, uint32_t func)
{
	uint64_t bar_size, bar_upper_bits;
	uint32_t offset = BAR0_OFFSET;
	uint32_t bar_reg_value, bar_lower_bits;

	while (offset <= TYPE1_BAR_MAX_OFF) {
		pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);

		if (BAR_REG(bar_reg_value) == BAR_64_BIT) {
			/*
			 * BAR supports 64-bit address therefore, write all 1's
			 * to BARn and BARn+1 and identify the size requested
			 */
			pal_pci_cfg_write(bus, dev, func, offset, 0xFFFFFFF0);
			pal_pci_cfg_write(bus, dev, func, offset + 4,
					  0xFFFFFFFF);
			pal_pci_cfg_read(bus, dev, func, offset,
					 &bar_lower_bits);
			bar_size = bar_lower_bits & BAR_MASK;

			pal_pci_cfg_read(bus, dev, func, offset + 4,
					 &bar_reg_value);
			bar_upper_bits = bar_reg_value;
			bar_size = bar_size | (bar_upper_bits << 32);

			bar_size = ~bar_size + 1;

			/*
			 * If BAR size is 0, then BAR not implemented, move to
			 * next BAR
			 */
			if (bar_size == 0) {
				offset = offset + 8;
				continue;
			}

			pal_pci_cfg_write(bus, dev, func, offset,
					  (uint32_t)g_rp_bar64_value);
			pal_pci_cfg_write(bus, dev, func, offset + 4,
					  (uint32_t)(g_rp_bar64_value >> 32));
			offset = offset + 8;
		} else {
			/*
			 * BAR supports 32-bit address. Write all 1's to BARn
			 * and identify the size requested
			 */
			pal_pci_cfg_write(bus, dev, func, offset, 0xFFFFFFF0);
			pal_pci_cfg_read(bus, dev, func, offset,
					 &bar_lower_bits);
			bar_reg_value = bar_lower_bits & BAR_MASK;
			bar_size = ~bar_reg_value + 1;

			/*
			 * If BAR size is 0, then BAR not implemented, move to
			 * next BAR
			 */
			if (bar_size == 0) {
				offset = offset + 4;
				continue;
			}

			pal_pci_cfg_write(bus, dev, func, offset,
					  g_rp_bar32_value);
			g_rp_bar32_value = g_rp_bar32_value + (uint32_t)bar_size;
			offset = offset + 4;
		}
	}
}

/*
 * This API programs all the BAR register in PCIe config space pointed by Bus,
 *  Device and Function for an End Point PCIe device
 */
static void pcie_program_bar_reg(uint32_t bus, uint32_t dev, uint32_t func)
{
	uint64_t bar_size, bar_upper_bits;
	uint32_t bar_reg_value, bar_lower_bits;
	uint32_t offset = BAR0_OFFSET;
	uint32_t np_bar_size = 0;
	uint32_t p_bar_size = 0, p_bar64_size = 0;

	while (offset <= TYPE0_BAR_MAX_OFF) {
		pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);

		if (BAR_MEM(bar_reg_value) == BAR_PRE_MEM) {
			if (BAR_REG(bar_reg_value) == BAR_64_BIT) {
				/*
				 * BAR supports 64-bit address therefore,
				 * write all 1's to BARn and BARn+1 and identify
				 * the size requested
				 */

				pal_pci_cfg_write(bus, dev, func, offset,
						  0xFFFFFFF0);
				pal_pci_cfg_write(bus, dev, func, offset + 4,
						  0xFFFFFFFF);
				pal_pci_cfg_read(bus, dev, func, offset,
						 &bar_lower_bits);
				bar_size = bar_lower_bits & BAR_MASK;

				pal_pci_cfg_read(bus, dev, func, offset + 4,
						 &bar_reg_value);
				bar_upper_bits = bar_reg_value;
				bar_size = bar_size | (bar_upper_bits << 32);

				bar_size = ~bar_size + 1;

				/*
				 * If BAR size is 0, then BAR not implemented,
				 * move to next BAR
				 */
				if (bar_size == 0) {
					offset = offset + 8;
					continue;
				}

				/*
				 * If p_bar64_size = 0 and bus number is same as
				 * bus of previous bus number, then check if the
				 * current PCIe Device BAR size is greater than
				 * the previous BAR size, if yes then add current
				 * BAR size to the updated start address else
				 * add the previous BAR size to the updated
				 * start address
				 */
				if ((p_bar64_size == 0) && ((g_64_bus == bus))) {
					if (g_bar64_size < bar_size) {
						g_bar64_p_start =
							g_bar64_p_start +
							bar_size;
					} else {
						g_bar64_p_start =
							g_bar64_p_start +
							g_bar64_size;
					}
				} else if ((g_bar64_size < bar_size) &&
					   (p_bar64_size != 0)) {
					g_bar64_p_start = g_bar64_p_start +
						bar_size;
				} else {
					g_bar64_p_start = g_bar64_p_start +
						p_bar64_size;
				}

				pal_pci_cfg_write(bus, dev, func, offset,
						  (uint32_t)g_bar64_p_start);
				pal_pci_cfg_write(bus, dev, func, offset + 4,
						  (uint32_t)(g_bar64_p_start >>
							     32));

				p_bar64_size = (uint32_t)bar_size;
				g_bar64_size = (uint32_t)bar_size;
				g_64_bus = bus;
				offset = offset + 8;
			} else {
				/*
				 * BAR supports 32-bit address. Write all 1's
				 * to BARn and identify the size requested
				 */
				pal_pci_cfg_write(bus, dev, func, offset,
						  0xFFFFFFF0);
				pal_pci_cfg_read(bus, dev, func, offset,
						 &bar_lower_bits);
				bar_reg_value = bar_lower_bits & BAR_MASK;
				bar_size = ~bar_reg_value + 1;

				/*
				 * If BAR size is 0, then BAR not implemented,
				 * move to next BAR
				 */
				if (bar_size == 0) {
					offset = offset + 4;
					continue;
				}

				/*
				 * If p_bar_size = 0 and bus number is same as
				 * bus of previous bus number, then check if the
				 * current PCIe Device BAR size is greater than
				 * the previous BAR size, if yes then add
				 * current BAR size to the updated start
				 * address else add the previous BAR size to the
				 * updated start address
				 */
				if ((p_bar_size == 0) && ((g_p_bus == bus))) {
					if (g_p_bar_size < bar_size) {
						g_bar32_p_start =
							g_bar32_p_start +
							(uint32_t)bar_size;
					} else {
						g_bar32_p_start =
							g_bar32_p_start +
							g_p_bar_size;
					}
				} else if ((g_p_bar_size < bar_size) &&
					   (p_bar_size != 0)) {
					g_bar32_p_start = g_bar32_p_start +
						(uint32_t)bar_size;
				} else {
					g_bar32_p_start = g_bar32_p_start +
						p_bar_size;
				}

				pal_pci_cfg_write(bus, dev, func, offset,
						  g_bar32_p_start);
				p_bar_size = (uint32_t)bar_size;
				g_p_bar_size = (uint32_t)bar_size;
				g_p_bus = bus;

				offset = offset + 4;
			}
		} else {
			/*
			 * BAR supports 32-bit address. Write all 1's to BARn
			 * and identify the size requested
			 */
			pal_pci_cfg_write(bus, dev, func, offset, 0xFFFFFFF0);
			pal_pci_cfg_read(bus, dev, func, offset,
					 &bar_lower_bits);
			bar_reg_value = bar_lower_bits & BAR_MASK;
			bar_size = ~bar_reg_value + 1;

			/*
			 * If BAR size is 0, then BAR not implemented, move to
			 * next BAR
			 */
			if (bar_size == 0) {
				if (BAR_REG(bar_lower_bits) == BAR_64_BIT) {
					offset = offset + 8;
				}

				if (BAR_REG(bar_lower_bits) == BAR_32_BIT) {
					offset = offset + 4;
				}

				continue;
			}

			/*
			 * If np_bar_size = 0 and bus number is same as bus of
			 * previous bus number, then check if the current PCIe
			 * Device BAR size is greater than the previous BAR
			 * size, if yes then add current BAR size to the
			 * updated start address else add the previous BAR size
			 * to the updated start address
			 */
			if ((np_bar_size == 0) && ((g_np_bus == bus))) {
				if (g_np_bar_size < bar_size) {
					g_bar32_np_start = g_bar32_np_start +
						(uint32_t)bar_size;
				} else {
					g_bar32_np_start = g_bar32_np_start +
						g_np_bar_size;
				}
			} else if ((g_np_bar_size < bar_size) &&
				   (np_bar_size != 0)) {
				g_bar32_np_start = g_bar32_np_start +
					(uint32_t)bar_size;
			} else {
				g_bar32_np_start = g_bar32_np_start +
					np_bar_size;
			}

			pal_pci_cfg_write(bus, dev, func, offset,
					  g_bar32_np_start);
			np_bar_size = (uint32_t)bar_size;
			g_np_bar_size = (uint32_t)bar_size;
			g_np_bus = bus;

			pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);
			if (BAR_REG(bar_reg_value) == BAR_64_BIT) {
				pal_pci_cfg_write(bus, dev, func,
						  offset + 4, 0);
				offset = offset + 8;
			}

			if (BAR_REG(bar_reg_value) == BAR_32_BIT) {
				offset = offset + 4;
			}
		}

		g_bar32_p_max = g_bar32_p_start;
		g_bar32_np_max = g_bar32_np_start;
		g_bar64_p_max =  g_bar64_p_start;
	}
}

/*
 * This API performs the PCIe bus enumeration
 *
 * bus,sec_bus - Bus(8-bits), secondary bus (8-bits)
 * sub_bus - Subordinate bus
 */
static uint32_t pcie_enumerate_device(uint32_t bus, uint32_t sec_bus)
{
	uint32_t vendor_id = 0;
	uint32_t header_value;
	uint32_t sub_bus = bus;
	uint32_t dev;
	uint32_t func;
	uint32_t class_code;
	uint32_t com_reg_value;
	uint32_t bar32_p_limit;
	uint32_t bar32_np_limit;
	uint32_t bar32_p_base = g_bar32_p_start;
	uint32_t bar32_np_base = g_bar32_np_start;
	uint64_t bar64_p_base = g_bar64_p_start;

	if (bus == ((g_pcie_info_table->block[g_pcie_index].end_bus_num) + 1)) {
		return sub_bus;
	}

	for (dev = 0; dev < PCIE_MAX_DEV; dev++) {
		for (func = 0; func < PCIE_MAX_FUNC; func++) {
			pal_pci_cfg_read(bus, dev, func, 0, &vendor_id);

			if ((vendor_id == 0x0) || (vendor_id == 0xFFFFFFFF)) {
				continue;
			}

			/* Skip Hostbridge configuration */
			pal_pci_cfg_read(bus, dev, func, TYPE01_RIDR,
					 &class_code);

			if ((((class_code >> CC_BASE_SHIFT) & CC_BASE_MASK) ==
			     HB_BASE_CLASS) &&
			    (((class_code >> CC_SUB_SHIFT) & CC_SUB_MASK)) ==
			    HB_SUB_CLASS) {
				continue;
			}

			pal_pci_cfg_read(bus, dev, func, HEADER_OFFSET,
					 &header_value);
			if (PCIE_HEADER_TYPE(header_value) == TYPE1_HEADER) {
				/*
				 * Enable memory access, Bus master enable and
				 * I/O access
				 */
				pal_pci_cfg_read(bus, dev, func,
						 COMMAND_REG_OFFSET,
						 &com_reg_value);

				pal_pci_cfg_write(bus, dev, func,
						  COMMAND_REG_OFFSET,
						  (com_reg_value |
						   REG_ACC_DATA));

				pal_pci_cfg_write(bus, dev, func,
						  BUS_NUM_REG_OFFSET,
						  BUS_NUM_REG_CFG(0xFF, sec_bus,
								  bus));

				pal_pci_cfg_write(bus, dev, func,
						  NON_PRE_FET_OFFSET,
						  ((g_bar32_np_start >> 16) &
						   0xFFF0));

				pal_pci_cfg_write(bus, dev, func,
						  PRE_FET_OFFSET,
						  ((g_bar32_p_start >> 16) &
						   0xFFF0));

				sub_bus = pcie_enumerate_device(sec_bus,
								(sec_bus + 1));
				pal_pci_cfg_write(bus, dev, func,
						  BUS_NUM_REG_OFFSET,
						  BUS_NUM_REG_CFG(sub_bus,
								  sec_bus, bus));
				sec_bus = sub_bus + 1;

				/*
				 * Obtain the start memory base address & the
				 * final memory base address of 32 bit BAR
				 */
				bar32_p_limit = g_bar32_p_max;
				bar32_np_limit = g_bar32_np_max;

				get_resource_base_32(bus, dev, func,
						     bar32_p_base,
						     bar32_np_base,
						     bar32_p_limit,
						     bar32_np_limit);

				/*
				 * Obtain the start memory base address & the
				 * final memory base address of 64 bit BAR
				 */
				get_resource_base_64(bus, dev, func,
						     bar64_p_base,
						     g_bar64_p_max);

				/* Update the BAR values of Type 1 Devices */
				pcie_rp_program_bar(bus, dev, func);

				/* Update the base and limit values */
				bar32_p_base = g_bar32_p_start;
				bar32_np_base = g_bar32_np_start;
				bar64_p_base = g_bar64_p_start;
			}

			if (PCIE_HEADER_TYPE(header_value) == TYPE0_HEADER) {
				pcie_program_bar_reg(bus, dev, func);
				sub_bus = sec_bus - 1;
			}
		}
	}

	return sub_bus;
}

/*
 * This API clears the primary bus number configured in the Type1 Header.
 * Note: This is done to make sure the hardware is compatible
 * with Linux enumeration.
 */
static void pcie_clear_pri_bus(void)
{
	uint32_t bus;
	uint32_t dev;
	uint32_t func;
	uint32_t bus_value;
	uint32_t header_value;
	uint32_t vendor_id;

	for (bus = 0; bus <= g_pcie_info_table->block[g_pcie_index].end_bus_num;
	     bus++) {
		for (dev = 0; dev < PCIE_MAX_DEV; dev++) {
			for (func = 0; func < PCIE_MAX_FUNC; func++) {
				pal_pci_cfg_read(bus, dev, func, 0, &vendor_id);

				if ((vendor_id == 0x0) ||
				    (vendor_id == 0xFFFFFFFF)) {
					continue;
				}

				pal_pci_cfg_read(bus, dev, func, HEADER_OFFSET,
						 &header_value);
				if (PCIE_HEADER_TYPE(header_value) ==
				    TYPE1_HEADER) {
					pal_pci_cfg_read(bus, dev, func,
							 BUS_NUM_REG_OFFSET,
							 &bus_value);

					bus_value = bus_value &
						PRI_BUS_CLEAR_MASK;

					pal_pci_cfg_write(bus, dev, func,
							  BUS_NUM_REG_OFFSET,
							  bus_value);
				}
			}
		}
	}
}

static void pcie_enumerate_devices(void)
{
	uint32_t pri_bus, sec_bus;
	int rc;

	g_pcie_info_table = plat_pcie_get_info_table();
	if (g_pcie_info_table == NULL) {
		ERROR("PCIe info not returned by platform\n");
		panic();
	}

	if (g_pcie_info_table->num_entries == 0) {
		INFO("Skipping Enumeration\n");
		return;
	}

	/* Get platform specific bar config parameters */
	rc = plat_pcie_get_bar_config(&g_bar64_p_start, &g_rp_bar64_value,
				      &g_bar32_np_start, &g_bar32_p_start,
				      &g_rp_bar32_value);
	if (rc != 0) {
		ERROR("PCIe bar config parameters not returned by platform\n");
		panic();
	}

	INFO("Starting Enumeration\n");
	while (g_pcie_index < g_pcie_info_table->num_entries) {
		pri_bus = g_pcie_info_table->block[g_pcie_index].start_bus_num;

		sec_bus = pri_bus + 1;

		pcie_enumerate_device(pri_bus, sec_bus);
		pcie_clear_pri_bus();

		g_pcie_index++;
	}
	g_enumerate = 0;
	g_pcie_index = 0;
}

void pcie_init(void)
{
	static bool is_init;

	/* Create PCIe table and enumeration */
	if (!is_init) {
		pcie_enumerate_devices();

		pcie_create_info_table();
		is_init = true;
	}
}
