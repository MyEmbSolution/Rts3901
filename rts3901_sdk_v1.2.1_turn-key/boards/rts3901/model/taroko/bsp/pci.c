/*
 * Realtek Semiconductor Corp.
 *
 * pci.c:
 *     bsp PCI initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/pci.h>

#include "bspchip.h"

extern struct pci_ops bsp_pcie_ops;

static struct resource pcie_mem_resource = {
	.name = "PCIe Memory resources",
	.start = BSP_PCIE_MEM_PADDR,
	.end = (BSP_PCIE_MEM_PADDR+ BSP_PCIE_MEM_PSIZE - 1),
	.flags = IORESOURCE_MEM,
};

static struct resource pcie_io_resource = {
	.name = "PCIe I/O resources",
	.start = BSP_PCIE_IO_PADDR,
	.end = (BSP_PCIE_IO_PADDR+ BSP_PCIE_IO_PSIZE- 1),
	.flags = IORESOURCE_IO,
};

static struct pci_controller bsp_pcie_controller = {
	.pci_ops = &bsp_pcie_ops,
	.mem_resource = &pcie_mem_resource,
	.mem_offset = BSP_PCIE_MEM_PADDR,
	.io_resource = &pcie_io_resource,
	.io_offset = BSP_PCIE_IO_PADDR,
};

static int __init bsp_pcie_init(void)
{
	struct pci_controller *pcie = &bsp_pcie_controller;
	unsigned int val;

	val = REG32(0xbb004000);
	if (val != 0x3) {
		printk("no device found, skipping PCIe initialization\n");
		return 0;
	}

	/* initialize Sheipa PCIe */
	printk("Initializing Sheipa PCIe controller\n");
	printk("I/O base = %lx, size = %lx\n", BSP_PCIE_IO_PADDR, BSP_PCIE_IO_PSIZE);
	printk("MEM base = %lx, size = %lx\n", BSP_PCIE_MEM_PADDR, BSP_PCIE_MEM_PSIZE);

	REG32(BSP_PCIE_RC_CFG + 0x04) = 0x00100007;
	REG32(BSP_PCIE_RC_CFG + 0x78) = 0x105030;

#if defined(CONFIG_DMA_COHERENT)
	/* Setup CAP access range for PCIE */
	REG32(BSP_PCIE_RC_CFG + 0x10f0a4) = 0x0;	/* Physical address for start */
	REG32(BSP_PCIE_RC_CFG + 0x10f0a8) = 0x0fffffff;	/* Physical address for end */
#endif

	pcie->io_map_base = (unsigned long)ioremap_nocache(BSP_PCIE_IO_PADDR,
							   BSP_PCIE_IO_PSIZE);
	set_io_port_base(pcie->io_map_base);
	register_pci_controller(pcie);
	return 0;
}
arch_initcall(bsp_pcie_init);
