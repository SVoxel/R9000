#ifndef __LINUX_USB_PCI_QUIRKS_H
#define __LINUX_USB_PCI_QUIRKS_H

#ifdef CONFIG_PCI
void uhci_reset_hc(struct pci_dev *pdev, unsigned long base);
int uhci_check_and_reset_hc(struct pci_dev *pdev, unsigned long base);
#endif  /* CONFIG_PCI */

#if defined(CONFIG_PCI) && !defined(CONFIG_PCI_DISABLE_COMMON_QUIRKS)
int usb_amd_find_chipset_info(void);
void usb_amd_dev_put(void);
void usb_amd_quirk_pll_disable(void);
void usb_amd_quirk_pll_enable(void);
bool usb_is_intel_switchable_xhci(struct pci_dev *pdev);
void usb_enable_xhci_ports(struct pci_dev *xhci_pdev);
void usb_disable_xhci_ports(struct pci_dev *xhci_pdev);
#else
static inline int usb_amd_find_chipset_info(void)
{
	return 0;
}
static inline void usb_amd_quirk_pll_disable(void) {}
static inline void usb_amd_quirk_pll_enable(void) {}
static inline void usb_amd_dev_put(void) {}

static inline bool usb_is_intel_switchable_xhci(struct pci_dev *pdev)
{
	return false;
}
static inline void usb_enable_xhci_ports(struct pci_dev *xhci_pdev) {}
static inline void usb_disable_xhci_ports(struct pci_dev *xhci_pdev) {}
#endif

#endif  /*  __LINUX_USB_PCI_QUIRKS_H  */
