/* SPDX-License-Identifier: ISC */
/*
 * US rules from wireless-regdb 2026.09.03, db.txt.
 * https://www.kernel.org/pub/software/network/wireless-regdb/
 *
 * Copyright (c) 2008, Luis R. Rodriguez <mcgrof@gmail.com>
 * Copyright (c) 2008, Johannes Berg <johannes@sipsolutions.net>
 * Copyright (c) 2008, Michael Green <Michael.Green@Atheros.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

static const struct ieee80211_regdomain us_regdom = {
	.n_reg_rules = 11,
	.alpha2 = "US",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(902, 904, 2, 0, 30, 0),
		REG_RULE(904, 920, 16, 0, 30, 0),
		REG_RULE(920, 928, 8, 0, 30, 0),
		REG_RULE(2400, 2472, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 23, NL80211_RRF_AUTO_BW),
		REG_RULE(5250, 5350, 80, 0, 24,
			 NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
		REG_RULE(5470, 5730, 160, 0, 24, NL80211_RRF_DFS),
		REG_RULE(5730, 5850, 80, 0, 30, NL80211_RRF_AUTO_BW),
		REG_RULE(5850, 5895, 40, 0, 27,
			 NL80211_RRF_NO_OUTDOOR | NL80211_RRF_AUTO_BW |
			 NL80211_RRF_NO_IR),
		REG_RULE(5925, 7125, 320, 0, 12,
			 NL80211_RRF_NO_OUTDOOR | NL80211_RRF_NO_IR),
		REG_RULE(57240, 71000, 2160, 0, 40, 0),
	}
};
