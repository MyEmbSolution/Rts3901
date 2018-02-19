#ifndef __DWC_OTG_VERSION_H
#define __DWC_OTG_VERSION_H

#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE 199163	/* kernel version 3.10 */
#endif

#ifndef KERNEL_VERSION(a, b, c)
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

#endif /* #ifndef __DWC_OTG_VERSION_H */
