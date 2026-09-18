/*
  Драйвер W5500
  Autor: DiMoon Electronics
  Date: 2024/06/04
*/

#ifndef __W5500_DEF_H__
#define __W5500_DEF_H__

#include <stdint.h>

/******************************************************************************/
/// Control Phase ///

/// Access ///
#define W5500_RWB_ACCESS_READ (0x00 << 2)
#define W5500_RWB_ACCESS_WRITE (0x01 << 2)

/// Operation Mode ///
#define W5500_OM_VDM (0x00)
#define W5500_OM_FDM_1 (0x01)
#define W5500_OM_FDM_2 (0x02)
#define W5500_OM_FDM_4 (0x03)

/// Block select ///
#define W5500_BSB_COMMON (0x00 << 3)

#define W5500_BSB_SOCKET0 (0x01 << 3)
#define W5500_BSB_TX_BUFF_SOCKET0 (0x02 << 3)
#define W5500_BSB_RX_BUFF_SOCKET0 (0x03 << 3)

#define W5500_BSB_SOCKET1 (0x05 << 3)
#define W5500_BSB_TX_BUFF_SOCKET1 (0x06 << 3)
#define W5500_BSB_RX_BUFF_SOCKET1 (0x07 << 3)

#define W5500_BSB_SOCKET2 (0x09 << 3)
#define W5500_BSB_TX_BUFF_SOCKET2 (0x0A << 3)
#define W5500_BSB_RX_BUFF_SOCKET2 (0x0B << 3)

#define W5500_BSB_SOCKET3 (0x0D << 3)
#define W5500_BSB_TX_BUFF_SOCKET3 (0x0E << 3)
#define W5500_BSB_RX_BUFF_SOCKET3 (0x0F << 3)

#define W5500_BSB_SOCKET4 (0x11 << 3)
#define W5500_BSB_TX_BUFF_SOCKET4 (0x12 << 3)
#define W5500_BSB_RX_BUFF_SOCKET4 (0x13 << 3)

#define W5500_BSB_SOCKET5 (0x15 << 3)
#define W5500_BSB_TX_BUFF_SOCKET5 (0x16 << 3)
#define W5500_BSB_RX_BUFF_SOCKET5 (0x17 << 3)

#define W5500_BSB_SOCKET6 (0x19 << 3)
#define W5500_BSB_TX_BUFF_SOCKET6 (0x1A << 3)
#define W5500_BSB_RX_BUFF_SOCKET6 (0x1B << 3)

#define W5500_BSB_SOCKET7 (0x1D << 3)
#define W5500_BSB_TX_BUFF_SOCKET7 (0x1E << 3)
#define W5500_BSB_RX_BUFF_SOCKET7 (0x1F << 3)

/******************************************************************************/

/// BSB Common regirsers offset ///
#define W5500_SR_GATEWAY 0x01
#define W5500_SR_SUBNET 0x05
#define W5500_SR_MAC 0x09
#define W5500_SR_IP 0x0F
#define W5500_SR_PHYCFGR 0x2E

/// Socket n registers offset ///
#define W5500_Sn_MR 0x00         // Socket n Mode
#define W5500_Sn_CR 0x01         // Socket n Command
#define W5500_Sn_IR 0x02         // Socket n Interrupt
#define W5500_Sn_SR 0x03         // Socket n Status
#define W5500_Sn_PORT 0x04       // Socket n Source Port
#define W5500_Sn_DHAR 0x06       // Socket n Destination Hardware Address
#define W5500_Sn_DIPR 0x0C       // Socket n Destination IP Address
#define W5500_Sn_DPORT 0x10      // Socket n Destination Port
#define W5500_Sn_MSSR 0x12       // Socket n Maximum Segment Size
#define W5500_Sn_TOS 0x15        // Socket n IP TOS
#define W5500_Sn_TTL 0x16        // Socket n IP TTL
#define W5500_Sn_RXBUF_SIZE 0x1E // Socket n Receive Buffer Size
#define W5500_Sn_TXBUF_SIZE 0x1F // Socket n Transmit Buffer Size
#define W5500_Sn_TX_FSR 0x20     // Socket n TX Free Size
#define W5500_Sn_TX_RD 0x22      // Socket n TX Read Pointer
#define W5500_Sn_TX_WR 0x24      // Socket n TX Write Pointer
#define W5500_Sn_RX_RSR 0x26     // Socket n RX Received Size
#define W5500_Sn_RX_RD 0x28      // Socket n RX Read Pointer
#define W5500_Sn_RX_WR 0x2A      // Socket n RX Write Pointer
#define W5500_Sn_IMR 0x2C        // Socket n Interrupt Mask
#define W5500_Sn_FRAG 0x2D       // Socket n Fragment Offset in IP header
#define W5500_Sn_KPALVTR 0x2F    // Keep alive timer

/******************************************************************************/

#define W5500_Sn_MR_Protocol_Closed (0x00 << 0)
#define W5500_Sn_MR_Protocol_TCP (0x01 << 0)
#define W5500_Sn_MR_Protocol_UDP (0x02 << 0)
#define W5500_Sn_MR_Protocol_RAW (0x04 << 0)

#define W5500_Sn_CR_OPEN (0x01)
#define W5500_Sn_CR_LISTEN (0x02)
#define W5500_Sn_CR_CONNECT (0x04)
#define W5500_Sn_CR_DISCON (0x08)
#define W5500_Sn_CR_CLOSE (0x10)
#define W5500_Sn_CR_SEND (0x20)
#define W5500_Sn_CR_SEND_MAC (0x21)
#define W5500_Sn_CR_SEND_KEEP (0x22)
#define W5500_Sn_CR_RECV (0x40)

#define W5500_Sn_SR_SOCK_CLOSED (0x00)
#define W5500_Sn_SR_SOCK_INIT (0x13)
#define W5500_Sn_SR_SOCK_LISTEN (0x14)
#define W5500_Sn_SR_SOCK_ESTABLISHED (0x17)
#define W5500_Sn_SR_SOCK_CLOSE_WAIT (0x1C)
#define W5500_Sn_SR_SOCK_UDP (0x22)
#define W5500_Sn_SR_SOCK_MACRAW (0x42)
#define W5500_Sn_SR_SOCK_SYNSENT (0x15)
#define W5500_Sn_SR_SOCK_SYNRECV (0x16)
#define W5500_Sn_SR_SOCK_FIN_WAIT (0x18)
#define W5500_Sn_SR_SOCK_CLOSING (0x1A)
#define W5500_Sn_SR_SOCK_TIME_WAIT (0x1B)
#define W5500_Sn_SR_SOCK_LAST_ACK (0x1D)

#define W5500_Sn_IR_SEND_OK (1 << 4)
#define W5500_Sn_IR_TIMEOUT (1 << 3)
#define W5500_Sn_IR_RECV (1 << 2)
#define W5500_Sn_IR_DISCON (1 << 1)
#define W5500_Sn_IR_CON (1 << 0)

/******************************************************************************/

#endif