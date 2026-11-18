// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 Intel Corporation
 */


static inline void aosp_do_open(struct hci_dev *hdev) {}
static inline void aosp_do_close(struct hci_dev *hdev) {}

static inline bool aosp_has_quality_report(struct hci_dev *hdev)
{
	return false;
}

static inline int aosp_set_quality_report(struct hci_dev *hdev, bool enable)
{
	return -EOPNOTSUPP;
}

