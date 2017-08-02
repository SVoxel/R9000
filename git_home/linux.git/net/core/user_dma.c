/*
 * Copyright(c) 2004 - 2006 Intel Corporation. All rights reserved.
 * Portions based on net/core/datagram.c and copyrighted by their authors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called COPYING.
 */

/*
 * This code allows the net stack to make use of a DMA engine for
 * skb to iovec copies.
 */

#include <linux/dmaengine.h>
#include <linux/socket.h>
#include <linux/export.h>
#include <net/tcp.h>
#include <net/netdma.h>

#define NET_DMA_DEFAULT_COPYBREAK  (1 << 20) /* don't enable NET_DMA by default */

int sysctl_tcp_dma_copybreak = NET_DMA_DEFAULT_COPYBREAK;
EXPORT_SYMBOL(sysctl_tcp_dma_copybreak);

/**
 *	dma_skb_copy_datagram_iovec - Copy a datagram to an iovec.
 *	@skb - buffer to copy
 *	@offset - offset in the buffer to start copying from
 *	@iovec - io vector to copy to
 *	@len - amount of data to copy from buffer to iovec
 *	@pinned_list - locked iovec buffer data
 *
 *	Note: the iovec is modified during the copy.
 */
int dma_skb_copy_datagram_iovec(struct dma_chan *chan,
			struct sk_buff *skb, int offset, struct iovec *to,
			size_t len, struct dma_pinned_list *pinned_list)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	dma_cookie_t cookie = 0;
	struct sg_table	*dst_sgt;
	struct sg_table	*src_sgt;
	struct scatterlist *dst_sg;
	int	dst_sg_len;
	struct scatterlist *src_sg;
	int		src_sg_len = skb_shinfo(skb)->nr_frags;
	size_t	dst_len = len;
	size_t	dst_offset = offset;

	pr_debug("%s %d copy %d len %d nr_iovecs %d skb frags %d\n",
			__func__, __LINE__, copy, len, pinned_list->nr_iovecs,
			src_sg_len);

	dst_sgt = pinned_list->sgts;

	if (copy > 0)
		src_sg_len += 1;

	src_sgt = pinned_list->sgts + 1;

	dst_sg = dst_sgt->sgl;
	src_sg = src_sgt->sgl;
	src_sg_len = 0;
	/* Copy header. */
	if (copy > 0) {
		if (copy > len)
			copy = len;

		sg_set_buf(src_sg, skb->data + offset, copy);
		pr_debug("%s %d: add src buf page %p. addr %p len 0x%x\n", __func__,
				__LINE__, virt_to_page(skb->data),
				skb->data, copy);

		len -= copy;
		src_sg_len++;
		if (len == 0)
			goto fill_dst_sg;
		offset += copy;
		src_sg = sg_next(src_sg);
	}

	/* Copy paged appendix. Hmm... why does this look so complicated? */
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		copy = end - offset;
		if (copy > 0) {
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;

			sg_set_page(src_sg, page, copy, frag->page_offset +
					offset - start);
			pr_debug("%s %d: add src buf [%d] page %p. len 0x%x\n", __func__,
				__LINE__, i, page, copy);
			src_sg = sg_next(src_sg);
			src_sg_len++;
			len -= copy;
			if (len == 0)
				break;
			offset += copy;
		}
		start = end;
	}

fill_dst_sg:
	dst_sg_len = dma_memcpy_fill_sg_from_iovec(chan, to, pinned_list, dst_sg, dst_offset, dst_len);
	BUG_ON(dst_sg_len <= 0);

	cookie = dma_async_memcpy_sg_to_sg(chan,
					dst_sgt->sgl,
					dst_sg_len,
					src_sgt->sgl,
					src_sg_len);


	if (!len) {
		skb->dma_cookie = cookie;

		return cookie;
	}

	return -EFAULT;
}
