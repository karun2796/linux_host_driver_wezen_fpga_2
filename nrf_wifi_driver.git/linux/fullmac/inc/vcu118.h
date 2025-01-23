/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef HACK_SERDES_CONFIG
void vcu118_int_enable(struct pci_dev *pdev);
void vcu118_serdes_reset(struct pci_dev *pdev);
#endif
