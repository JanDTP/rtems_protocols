/*===============================================================*\
| Project: RTEMS support for MPC83xx                              |
+-----------------------------------------------------------------+
|                    Copyright (c) 2007                           |
|                    Embedded Brains GmbH                         |
|                    Obere Lagerstr. 30                           |
|                    D-82178 Puchheim                             |
|                    Germany                                      |
|                    rtems@embedded-brains.de                     |
+-----------------------------------------------------------------+
| The license and distribution terms for this file may be         |
| found in the file LICENSE in this distribution or at            |
|                                                                 |
| http://www.rtems.com/license/LICENSE.                           |
|                                                                 |
+-----------------------------------------------------------------+
| this file contains the low level MPC83xx SPI driver parameters  |
| and board-specific functions                                    |
\*===============================================================*/
#include <mpc83xx/mpc83xx_spidrv.h>
#include <bsp/irq.h>
#include <bsp.h>
#if defined(MPC8349EAMDS)
#include <libchip/i2c-2b-eeprom.h>
#include <libchip/spi-flash-m25p40.h>
#endif

/*=========================================================================*\
| Board-specific adaptation functions                                       |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
static rtems_status_code bsp_spi_sel_addr
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   address a slave device on the bus                                       |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 rtems_libi2c_bus_t *bh,                 /* bus specifier structure        */
 uint32_t addr,                          /* address to send on bus         */
 int rw                                  /* 0=write,1=read                 */
)
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    o = ok or error code                                                   |
\*=========================================================================*/
{
#if defined(MPC8349EAMDS)
  /*
   * check device address for valid range
   */
  if (addr > 0) {
    return RTEMS_INVALID_NUMBER;
  }
  /*
   * select given device
   * GPIO1[0] is nSEL_SPI for M25P40
   * set it to be active/low
   */
  mpc83xx.gpio[0].gpdat &= ~(1 << (31- 0));
#endif
#if defined(HSC_CM01)
  /*
   * check device address for valid range
   */
  if (addr > 7) {
    return RTEMS_INVALID_NUMBER;
  }
  /*
   * select given device
   */
  /*
   * GPIO1[24] is SPI_A0
   * GPIO1[25] is SPI_A1
   * GPIO1[26] is SPI_A2
   * set pins to address 
   */
  mpc83xx.gpio[0].gpdat = 
    (mpc83xx.gpio[0].gpdat & ~(0x7  << (31-26)))
    |                         (addr << (31-26));
  /*
   * GPIO1[27] is high-active strobe 
   */
  mpc83xx.gpio[0].gpdat |= (1 << (31- 27));
#endif
  return  RTEMS_SUCCESSFUL;
}

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
static rtems_status_code bsp_spi_send_start_dummy
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   dummy function, SPI has no start condition                              |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 rtems_libi2c_bus_t *bh                  /* bus specifier structure        */
)
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    o = ok or error code                                                   |
\*=========================================================================*/
{
#if defined(MPC8349EAMDS)
  /*
   * GPIO1[0] is nSEL_SPI for M25P40
   * set it to inactive/high
   */
  mpc83xx.gpio[0].gpdat |=  (1 << (31- 0));
#endif
#if defined(HSC_CM01)
  /*
   * GPIO1[27] is high-active strobe 
   * set it to inactive/ low
   */
  mpc83xx.gpio[0].gpdat &= ~(0x1 << (31-27));
#endif
  return 0;
}

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
static rtems_status_code bsp_spi_send_stop
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   deselect SPI                                                            |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 rtems_libi2c_bus_t *bh                  /* bus specifier structure        */
)
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    o = ok or error code                                                   |
\*=========================================================================*/
{
#if defined(DEBUG)
  printk("bsp_spi_send_stop called... ");
#endif
#if defined(MPC8349EAMDS)
  /*
   * deselect given device
   * GPIO1[0] is nSEL_SPI for M25P40
   * set it to be inactive/high
   */
  mpc83xx.gpio[0].gpdat |=  (1 << (31- 0));
#endif
#if defined(HSC_CM01)
  /*
   * deselect device
   * GPIO1[27] is high-active strobe 
   */
  mpc83xx.gpio[0].gpdat &= ~(1 << (31- 27));
#endif
#if defined(DEBUG)
  printk("... exit OK\r\n");
#endif
  return 0;
}

/*=========================================================================*\
| list of handlers                                                          |
\*=========================================================================*/

rtems_libi2c_bus_ops_t bsp_spi_ops = {
  init:             mpc83xx_spi_init,
  send_start:       bsp_spi_send_start_dummy,
  send_stop:        bsp_spi_send_stop,
  send_addr:        bsp_spi_sel_addr,
  read_bytes:       mpc83xx_spi_read_bytes,
  write_bytes:      mpc83xx_spi_write_bytes,
  ioctl:            mpc83xx_spi_ioctl
};

static mpc83xx_spi_desc_t bsp_spi_bus_desc = {
  {/* public fields */
    ops:	&bsp_spi_ops,
    size:	sizeof(bsp_spi_bus_desc),
  },
  { /* our private fields */
    reg_ptr:	 &mpc83xx.spi,
    initialized: FALSE,
    irq_number:  BSP_IPIC_IRQ_SPI
  }
};

/*=========================================================================*\
| initialization                                                            |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
rtems_status_code bsp_register_spi
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   register SPI bus and devices                                            |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 void                                    /* <none>                         */
)
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    0 or error code                                                        |
\*=========================================================================*/
{
  int ret_code;
  int spi_busno;

  /*
   * init I2C library (if not already done)
   */
  rtems_libi2c_initialize ();

  /*
   * init port pins used to address/select SPI devices
   */
#if defined(MPC8349EAMDS)
  /*
   * GPIO1[0] is nSEL_SPI for M25P40
   * set it to be output, high
   */
  mpc83xx.gpio[0].gpdat |=  (1 << (31- 0));
  mpc83xx.gpio[0].gpdir |=  (1 << (31- 0));
  mpc83xx.gpio[0].gpdr  &= ~(1 << (31- 0));
#endif
#if defined(HSC_CM01)
  /*
   * GPIO1[24] is SPI_A0
   * GPIO1[25] is SPI_A1
   * GPIO1[26] is SPI_A2
   * GPIO1[27] is high-active strobe 
   * set pins to be output, low
   */
  mpc83xx.gpio[0].gpdat &= ~(0xf << (31-27));
  mpc83xx.gpio[0].gpdir |=  (0xf << (31-27));
  mpc83xx.gpio[0].gpdr  &= ~(0xf << (31-27));
#endif
  /*
   * register SPI bus
   */
  ret_code = rtems_libi2c_register_bus("/dev/spi",
				       &(bsp_spi_bus_desc.bus_desc));
  if (ret_code < 0) {
    return -ret_code;
  }
  spi_busno = ret_code;
  /*
   * register M25P40 Flash, when available
   */
#if defined(MPC8349EAMDS)
  ret_code = rtems_libi2c_register_drv(RTEMS_BSP_SPI_FLASH_DEVICE_NAME,
				       spi_flash_m25p40_rw_driver_descriptor,
				       spi_busno,0x00);
  if (ret_code < 0) {
    return -ret_code;
  }
#endif
  /*
   * FIXME: further drivers, when available
   */
  return 0;
}
