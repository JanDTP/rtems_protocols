/*
 * Copyright (c) 2013 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <info@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#include <bsp.h>
#include <bsp/bootcard.h>
#include <bsp/irq-generic.h>
#include <bsp/fdt.h>
#include <bsp/linker-symbols.h>

#include "bspdebug.h"

void bsp_start(void)
{
#if IS_DM3730
  const char* type = "dm3730-based";
#endif
#if IS_AM335X
  const char* type = "am335x-based";
#endif
  bsp_interrupt_initialize();
  printk("\nRTEMS Beagleboard: %s\n", type);
  printk("        ARM Debug: 0x%08x\n", (intptr_t) bbb_arm_debug_registers());
  rtems_cache_coherent_add_area(
      bsp_section_nocacheheap_begin,
      (uintptr_t) bsp_section_nocacheheap_size
  );
}

uint32_t bsp_fdt_map_intr(const uint32_t *intr, size_t icells)
{
  return intr[0];
}
