// SPDX-License-Identifier: GPL-2.0
/*
 *  fs/partitions/mac.c
 *
 *  Code extracted from drivers/block/genhd.c
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 */

#include <linux/ctype.h>
#include "check.h"
#include "mac.h"


/*
 * Code to understand MacOS partition tables.
 */


int mac_partition(struct parsed_partitions *state)
{
	Sector sect;
	unsigned char *data;
	int slot, blocks_in_map;
	unsigned secsize, datasize, partoffset;
	struct mac_partition *part;
	struct mac_driver_desc *md;

	/* Get 0th block and look at the first partition map entry. */
	md = read_part_sector(state, 0, &sect);
	if (!md)
		return -1;
	if (be16_to_cpu(md->signature) != MAC_DRIVER_MAGIC) {
		put_dev_sector(sect);
		return 0;
	}
	secsize = be16_to_cpu(md->block_size);
	put_dev_sector(sect);

	/*
	 * If the "block size" is not a power of 2, things get weird - we might
	 * end up with a partition straddling a sector boundary, so we wouldn't
	 * be able to read a partition entry with read_part_sector().
	 * Real block sizes are probably (?) powers of two, so just require
	 * that.
	 */
	if (!is_power_of_2(secsize))
		return -1;
	datasize = round_down(secsize, 512);
	data = read_part_sector(state, datasize / 512, &sect);
	if (!data)
		return -1;
	partoffset = secsize % 512;
	if (partoffset + sizeof(*part) > datasize) {
		put_dev_sector(sect);
		return -1;
	}
	part = (struct mac_partition *) (data + partoffset);
	if (be16_to_cpu(part->signature) != MAC_PARTITION_MAGIC) {
		put_dev_sector(sect);
		return 0;		/* not a MacOS disk */
	}
	blocks_in_map = be32_to_cpu(part->map_count);
	if (blocks_in_map < 0 || blocks_in_map >= DISK_MAX_PARTS) {
		put_dev_sector(sect);
		return 0;
	}

	if (blocks_in_map >= state->limit)
		blocks_in_map = state->limit - 1;

	seq_buf_puts(&state->pp_buf, " [mac]");
	for (slot = 1; slot <= blocks_in_map; ++slot) {
		int pos = slot * secsize;
		put_dev_sector(sect);
		data = read_part_sector(state, pos/512, &sect);
		if (!data)
			return -1;
		part = (struct mac_partition *) (data + pos%512);
		if (be16_to_cpu(part->signature) != MAC_PARTITION_MAGIC)
			break;
		put_partition(state, slot,
			(sector_t)be32_to_cpu(part->start_block) * (secsize/512),
			(sector_t)be32_to_cpu(part->block_count) * (secsize/512));

		if (!strncasecmp(part->type, "Linux_RAID", 10))
			state->parts[slot].flags = ADDPART_FLAG_RAID;
	}

	put_dev_sector(sect);
	seq_buf_puts(&state->pp_buf, "\n");
	return 1;
}
