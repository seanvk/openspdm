/** @file
  Definitions of DSP0239 Management Component Transport Protocol (MCTP) IDs and Codes
  version 1.7.0 in Distributed Management Task Force (DMTF).

  Definitions of DSP0275 SPDM over MCTP Binding Specification
  version 1.0.0 in Distributed Management Task Force (DMTF).

  Definitions of DSP0276 Secured MCTP Messages over MCTP Binding Specification
  version 1.0.0 in Distributed Management Task Force (DMTF).

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MCTP_BINDING_H__
#define __MCTP_BINDING_H__

#pragma pack(1)

//
// MCTP header
//
typedef struct {
  UINT8    MessageType;
} MCTP_MESSAGE_HEADER;

#define MCTP_MESSAGE_TYPE_MCTP_CONTROL        0x00
#define MCTP_MESSAGE_TYPE_PLDM                0x01
#define MCTP_MESSAGE_TYPE_NCSI_CONTROL        0x02
#define MCTP_MESSAGE_TYPE_ETHERNET            0x03
#define MCTP_MESSAGE_TYPE_NVME_MANAGEMENT     0x04
#define MCTP_MESSAGE_TYPE_SPDM                0x05
#define MCTP_MESSAGE_TYPE_SECURED_MCTP        0x06
#define MCTP_MESSAGE_TYPE_VENDOR_DEFINED_PCI  0x7E
#define MCTP_MESSAGE_TYPE_VENDOR_DEFINED_IANA 0x7F

#pragma pack()

#endif
