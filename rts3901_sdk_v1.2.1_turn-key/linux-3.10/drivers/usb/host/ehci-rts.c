/*
 * EHCI-compliant USB host controller driver for Realtek IPCam SoCs
 *
 * Copyright 2014 Realtek Corp.
 *
 * Derived from the ehci-platform driver
 * Copyright 2007 Steven Brown <sbrown@cortland.com>
 * Copyright 2010-2012 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Derived from the ohci-ssb driver
 * Copyright 2007 Michael Buesch <m@bues.ch>
 *
 * Derived from the EHCI-PCI driver
 * Copyright (c) 2000-2004 by David Brownell
 *
 * Derived from the ohci-pci driver
 * Copyright 1999 Roman Weissgaerber
 * Copyright 2000-2002 David Brownell
 * Copyright 1999 Linus Torvalds
 * Copyright 1999 Gregory P. Smith
 *
 * Licensed under the GNU/GPL. See COPYING for details.
 */
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/usb/phy.h>
#include <linux/clk.h>

#include "ehci.h"

#define DRIVER_DESC "ehci-rts platform driver"

static const char hcd_name[] = "ehci-rts";
static struct usb_phy *phy;

struct ehci_dw_ext_regs {
	u32		insnreg[9];
};

#define DW_EXT_REGS_OFFSET	0x90

static int ehci_rts_reset(struct usb_hcd *hcd)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	struct usb_ehci_pdata *pdata = pdev->dev.platform_data;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;
	struct ehci_dw_ext_regs __iomem *dw_ext_regs = NULL;

	hcd->has_tt = pdata->has_tt;
	ehci->has_synopsys_hc_bug = pdata->has_synopsys_hc_bug;
	ehci->big_endian_desc = pdata->big_endian_desc;
	ehci->big_endian_mmio = pdata->big_endian_mmio;

	ehci->caps = hcd->regs + pdata->caps_offset;
	retval = ehci_setup(hcd);
	if (retval)
		return retval;

	if (pdata->no_io_watchdog)
		ehci->need_io_watchdog = 0;

	dw_ext_regs = (void __iomem *)ehci->caps + DW_EXT_REGS_OFFSET;

	/* set packet buffer Out/In threshold */
	ehci_writel(ehci, 0x00800080, &dw_ext_regs->insnreg[1]);

	return 0;
}

static struct hc_driver __read_mostly ehci_rts_hc_driver;

static const struct ehci_driver_overrides platform_overrides __initconst = {
	.reset =	ehci_rts_reset,
};

static struct usb_ehci_pdata ehci_rts_defaults;

static int ehci_rts_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	struct usb_ehci_pdata *pdata;
	int irq;
	int err = -ENOMEM;
	struct clk *phy_clk;

	if (usb_disabled())
		return -ENODEV;

	/*
	 * use reasonable defaults so platforms don't have to provide these.
	 * with DT probing on ARM, none of these are set.
	 */
	if (!pdev->dev.platform_data)
		pdev->dev.platform_data = &ehci_rts_defaults;
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	pdata = pdev->dev.platform_data;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq provided");
		return irq;
	}
	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		dev_err(&pdev->dev, "no memory resource provided");
		return -ENXIO;
	}

	if (pdata->power_on) {
		err = pdata->power_on(pdev);
		if (err < 0)
			return err;
	}

	hcd = usb_create_hcd(&ehci_rts_hc_driver, &pdev->dev,
			     dev_name(&pdev->dev));
	if (!hcd) {
		err = -ENOMEM;
		goto err_power;
	}

	hcd->rsrc_start = res_mem->start;
	hcd->rsrc_len = resource_size(res_mem);

	hcd->regs = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(hcd->regs)) {
		err = PTR_ERR(hcd->regs);
		goto err_put_hcd;
	}

	/* enable HOST_CLK_EN */
	phy_clk = clk_get(NULL, "usbphy_host_ck");
	if (!phy_clk) {
		printk("get clk usbphy host clk fail\n");
		return -1;
	}
	clk_prepare(phy_clk);
	clk_enable(phy_clk);
	clk_put(phy_clk);
	/**/

	phy = devm_usb_get_phy(&pdev->dev, USB_PHY_TYPE_USB2);
	if (IS_ERR(phy)) {
		dev_err(&pdev->dev, "unable to find usb phy\n");
		err = -ENODEV;
		goto err_put_hcd;
	}
	usb_phy_init(phy);

	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err)
		goto err_put_hcd;

	platform_set_drvdata(pdev, hcd);

	return err;

err_put_hcd:
	usb_put_hcd(hcd);
err_power:
	if (pdata->power_off)
		pdata->power_off(pdev);

	return err;
}

static int ehci_rts_remove(struct platform_device *dev)
{
	struct usb_hcd *hcd = platform_get_drvdata(dev);
	struct usb_ehci_pdata *pdata = dev->dev.platform_data;

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);
	platform_set_drvdata(dev, NULL);

	if (pdata->power_off)
		pdata->power_off(dev);

	if (pdata == &ehci_rts_defaults)
		dev->dev.platform_data = NULL;

	return 0;
}

#ifdef CONFIG_PM

static int ehci_rts_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct usb_ehci_pdata *pdata = dev->platform_data;
	struct platform_device *pdev =
		container_of(dev, struct platform_device, dev);
	bool do_wakeup = device_may_wakeup(dev);
	int ret;

	ret = ehci_suspend(hcd, do_wakeup);

	if (pdata->power_suspend)
		pdata->power_suspend(pdev);

	return ret;
}

static int ehci_rts_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct usb_ehci_pdata *pdata = dev->platform_data;
	struct platform_device *pdev =
		container_of(dev, struct platform_device, dev);

	if (pdata->power_on) {
		int err = pdata->power_on(pdev);
		if (err < 0)
			return err;
	}

	ehci_resume(hcd, false);
	return 0;
}

#else /* !CONFIG_PM */
#define ehci_rts_suspend	NULL
#define ehci_rts_resume		NULL
#endif /* CONFIG_PM */

static const struct dev_pm_ops ehci_rts_pm_ops = {
	.suspend	= ehci_rts_suspend,
	.resume		= ehci_rts_resume,
};

static struct platform_driver ehci_rts_driver = {
	.probe		= ehci_rts_probe,
	.remove		= ehci_rts_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ehci-platform",
		.pm	= &ehci_rts_pm_ops,
	}
};

static int __init ehci_rts_init(void)
{
	if (usb_disabled())
		return -ENODEV;

	pr_info("%s: " DRIVER_DESC "\n", hcd_name);

	ehci_init_driver(&ehci_rts_hc_driver, &platform_overrides);
	return platform_driver_register(&ehci_rts_driver);
}
module_init(ehci_rts_init);

static void __exit ehci_rts_cleanup(void)
{
	platform_driver_unregister(&ehci_rts_driver);
}
module_exit(ehci_rts_cleanup);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Wei WANG");
MODULE_LICENSE("GPL");
