/*
 * Copyright (c) 2004  Wolfgang Grandegger (wg@denx.de)
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 * 
 * Derived from rtai-24.1.12/spdrv/spdrv.[ch]:
 *  COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)
 *                      Giuseppe Renoldi (giuseppe@renoldi.org)
 * and from linuxppc_2_4_devel/arch/ppc/8xx_io/uart.c:
 *  UART driver for MPC8xx CPM SCC or SMC (CPM UART drivers 0.04)
 *  Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *  Copyright (c) 2001 Wolfgang Denk (wd@denx.de) [buffer config, HW handshake]
 */

#ifndef RTAI_SPDRV_MPC8XX_H
#define RTAI_SPDRV_MPC8XX_H

/* 
 * SPDRV related default settings. The can be over-written via
 * insmod parameters.
 */
#define SPBUFSIZ  512	/* Read/write buffer size */
#define SPPRIO    1	/* RTAI priority of service task */

/* 
 * Setup the UART ports for RTAI SPDRV: You can derive the macro 
 * definitions for the selected serial port from the Linux default
 * configuration of your board (check the ".config" file). Just 
 * replace the prefix "CONFIG" with "RTAI". 
 *
 * Note: You cannot use the UART port for both, RTAI and Linux.
 * 
 * Note: The number and size of the CPM buffer descripters for 
 *       RX or TX may influency the real-time behaviour of the
 *       RTAI serial port.
 */

//#define RTAI_SMC1_UART
#define RTAI_SMC2_UART
//#define RTAI_SCC1_UART
//#define RTAI_SCC2_UART
//#define RTAI_SCC3_UART
//#define RTAI_SCC4_UART

#ifdef RTAI_SMC1_UART
#define RTAI_SMC1_UART_RX_BDNUM 4
#define RTAI_SMC1_UART_RX_BDSIZE 32
#define RTAI_SMC1_UART_TX_BDNUM 4
#define RTAI_SMC1_UART_TX_BDSIZE 32
#endif

#ifdef RTAI_SMC2_UART
#define RTAI_UART_MAXIDL_SMC2 1
#define RTAI_SMC2_UART_RX_BDNUM 4
#define RTAI_SMC2_UART_RX_BDSIZE 32
#define RTAI_SMC2_UART_TX_BDNUM 4
#define RTAI_SMC2_UART_TX_BDSIZE 32
#if defined(CONFIG_RMU) 
/* For all MPC 850 and 823 based boards. */
#define RTAI_ALTSMC2
#endif
#endif

#ifdef CONFIG_IP860
#ifdef RTAI_SCC2_UART
#define RTAI_UART_CTS_CONTROL_SCC2
#define RTAI_UART_RTS_CONTROL_SCC2 2
#define RTAI_RTS2_PIN 18
#define RTAI_UART_CD_CONTROL_SCC2
#define RTAI_UART_MAXIDL_SCC2 1
#define RTAI_SCC2_UART_RX_BDNUM 4
#define RTAI_SCC2_UART_RX_BDSIZE 32
#define RTAI_SCC2_UART_TX_BDNUM 4
#define RTAI_SCC2_UART_TX_BDSIZE 32
#endif
#ifdef RTAI_SCC3_UART
#define RTAI_UART_CTS_CONTROL_SCC3
#define RTAI_CTS3_PIN 7
#define RTAI_UART_RTS_CONTROL_SCC3 4
#define RTAI_RTS3_PIN 7
#define RTAI_UART_CD_CONTROL_SCC3
#define RTAI_CD3_PIN 6
#define RTAI_UART_MAXIDL_SCC3 1
#define RTAI_SCC3_UART_RX_BDNUM 4
#define RTAI_SCC3_UART_RX_BDSIZE 32
#define RTAI_SCC3_UART_TX_BDNUM 4
#define RTAI_SCC3_UART_TX_BDSIZE 32
#endif
#ifdef RTAI_SCC4_UART
#define RTAI_UART_CTS_CONTROL_SCC4
#define RTAI_UART_RTS_CONTROL_SCC4 4
#define RTAI_RTS4_PIN 6
#define RTAI_UART_CD_CONTROL_SCC4
#define RTAI_UART_MAXIDL_SCC4 1
#define RTAI_SCC4_UART_RX_BDNUM 4
#define RTAI_SCC4_UART_RX_BDSIZE 32
#define RTAI_SCC4_UART_TX_BDNUM 4
#define RTAI_SCC4_UART_TX_BDSIZE 32
#endif
#endif

#endif /* RTAI_SPDRV_MPC8XX_H */
