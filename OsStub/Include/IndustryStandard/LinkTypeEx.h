/** @file
  Definition for pcap link types extension for SPDM transport layer (MCTP/PCI_DOE)

  https://www.tcpdump.org/linktypes.html

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LINK_TYPE_EX_H__
#define __LINK_TYPE_EX_H__

#pragma pack(1)

//
// 0 ~ 289 are defined by https://www.tcpdump.org/linktypes.html
//

//
// MCTP packet is defined in DMTF PMCI working group Management Component Transport Protocol (MCTP)
// Base Specification (https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf)
// 8.1 MCTP packet fields.
//
// It starts with MCTP transport header in Figure 4 – Generic message fields.
//
#define LINKTYPE_MCTP      291  // 0x0123

//
// PCI Data Object Exchange (DOE) is defined in PCI-SIG Data Object Exchange (DOE) ECN
// (https://members.pcisig.com/wg/PCI-SIG/document/14143) 6.xx.1 Data Objects.
//
// It starts with DOE Data Object Header 1 in Figure 6-x1: DOE Data Object Format.
//
#define LINKTYPE_PCI_DOE   292  // 0x0124

#pragma pack()

#endif
