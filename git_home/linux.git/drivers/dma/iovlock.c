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
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <net/tcp.h> /* for memcpy_toiovec */
#include <asm/io.h>
#include <asm/uaccess.h>

static int num_pages_spanned(struct iovec *iov)
{
	return
	((PAGE_ALIGN((unsigned long)iov->iov_base + iov->iov_len) -
	((unsigned long)iov->iov_base & PAGE_MASK)) >> PAGE_SHIFT);
}

#define NETDMA_MAX_NR_IOVECS	UIO_MAXIOV
#define NETDMA_MAX_NR_PAGES	NETDMA_MAX_NR_IOVECS
/*
 * Pin down all the iovec pages needed for len bytes.
 * return 0 on success
 *
 * We are allocating a single chunk of memory, and then carving it up into
 * 3 sections, the latter 2 whose size depends on the number of iovecs and the
 * total number of pages, respectively.
 */
int dma_pin_iovec_pages(struct tcp_sock *tp, struct iovec *iov, size_t len)
{
	struct dma_pinned_list *local_list;
	struct page **pages;
	int i;
	int ret;
	int nr_iovecs = 0;
	int iovec_len_used = 0;
	int iovec_pages_used = 0;

	if (!tp->ucopy.pinned_list) {
		/* single kmalloc for pinned list, page_list[], and the page arrays */
		local_list = kmalloc(sizeof(*local_list)
		+ (NETDMA_MAX_NR_IOVECS * sizeof (struct dma_page_list))
		+ (NETDMA_MAX_NR_PAGES * sizeof (struct page*)), GFP_KERNEL);

		/* handle malloc failure */
		if (!local_list)
			return -1;

		/* alloc sgt tables*/
		local_list->sgts = kmalloc(2 * sizeof(struct sg_table), GFP_KERNEL);
		if (!local_list->sgts)
			goto sgts_fail;
		ret = sg_alloc_table(local_list->sgts, NETDMA_MAX_NR_PAGES, GFP_KERNEL);
		if (ret)
			goto dst_sg_fail;
		ret = sg_alloc_table(local_list->sgts + 1, NETDMA_MAX_NR_IOVECS, GFP_KERNEL);
		if (ret)
			goto src_sg_fail;

		tp->ucopy.pinned_list = local_list;
		goto alloc_ok;
src_sg_fail:
		sg_free_table(local_list->sgts);
dst_sg_fail:
		kfree(local_list->sgts);
sgts_fail:
		kfree(local_list);
		return -1;
	}

alloc_ok:
	local_list = tp->ucopy.pinned_list;
	/* determine how many iovecs/pages there are, up front */
	do {
		iovec_len_used += iov[nr_iovecs].iov_len;
		iovec_pages_used += num_pages_spanned(&iov[nr_iovecs]);
		nr_iovecs++;
	} while (iovec_len_used < len);

	/* return error so DMA won't be used of the buffer is too large */
	if (unlikely((nr_iovecs > NETDMA_MAX_NR_IOVECS) || (iovec_pages_used > NETDMA_MAX_NR_PAGES)))
		return -1;

	local_list->nr_pages = iovec_pages_used;
	if (segment_eq(get_fs(), KERNEL_DS))
               local_list->kernel = 1;
        else
               local_list->kernel = 0;

	/* list of pages starts right after the page list array */
	pages = (struct page **) &local_list->page_list[nr_iovecs];

	local_list->nr_iovecs = 0;

	if (local_list->kernel) {
		for (i = 0; i < nr_iovecs; i++) {
			struct dma_page_list *page_list = &local_list->page_list[i];
			page_list->base_address = iov[i].iov_base;
			page_list->nr_pages = num_pages_spanned(&iov[i]);
			BUG_ON(num_pages_spanned(&iov[i]) != 1);
			*pages = kmap_to_page(iov[i].iov_base);
			page_list->pages = pages;
			pages++;
			local_list->nr_iovecs = i + 1;
		}
		pr_debug("%s %d: added kernel %d pages (%d vecs)\n", __func__, __LINE__,
				local_list->nr_pages, local_list->nr_iovecs);

	} else {
		for (i = 0; i < nr_iovecs; i++) {
			struct dma_page_list *page_list = &local_list->page_list[i];

			len -= iov[i].iov_len;

			if (!access_ok(VERIFY_WRITE, iov[i].iov_base, iov[i].iov_len))
				goto unpin;

			page_list->nr_pages = num_pages_spanned(&iov[i]);
			page_list->base_address = iov[i].iov_base;

			page_list->pages = pages;
			pages += page_list->nr_pages;

			/* pin pages down */
			down_read(&current->mm->mmap_sem);
			ret = get_user_pages(
					current,
					current->mm,
					(unsigned long) iov[i].iov_base,
					page_list->nr_pages,
					1,	/* write */
					0,	/* force */
					page_list->pages,
					NULL);
			up_read(&current->mm->mmap_sem);

			local_list->nr_iovecs = i + 1;
			if (ret != page_list->nr_pages) {
				pr_debug("%s %d: get_user_pages didn't succeed to pin all requested pages!!\n", __func__, __LINE__);
				/* set the nr_pages that really allocated so the unpin will release it*/
				page_list->nr_pages = ret > 0 ? ret : 0;
				goto unpin;
			}

		}
		pr_debug("%s %d: added user %d pages (%d vecs)\n", __func__, __LINE__,
				local_list->nr_pages, local_list->nr_iovecs);
	}
	return 0;

unpin:
	dma_unpin_iovec_pages(local_list);
	return -1;
}

void dma_unpin_iovec_pages(struct dma_pinned_list *pinned_list)
{
	int i, j;

	if (!pinned_list)
		return;

	if (!pinned_list->kernel) {
		for (i = 0; i < pinned_list->nr_iovecs; i++) {
			struct dma_page_list *page_list = &pinned_list->page_list[i];
			for (j = 0; j < page_list->nr_pages; j++) {
				set_page_dirty_lock(page_list->pages[j]);
				page_cache_release(page_list->pages[j]);
			}
		}
	}
}

void dma_free_iovec_data(struct tcp_sock *tp)
{
	struct dma_pinned_list *local_list = tp->ucopy.pinned_list;

	if (local_list) {
		sg_free_table(local_list->sgts);
		sg_free_table(local_list->sgts + 1);
		kfree(local_list->sgts);
		kfree(tp->ucopy.pinned_list);
		tp->ucopy.pinned_list = NULL;
	}
}

/* return number of sg elements created on success, and negative of failure */
int dma_memcpy_fill_sg_from_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, struct scatterlist *dst_sg,
	unsigned int offset, size_t len)
{
	int iov_byte_offset;
	int copy;
	int iovec_idx;
	int page_idx;
	int sg_nents = 0;

	pr_debug("%s %d:  nr iovecs %d. len 0x%x\n",
					__func__, __LINE__,
					 pinned_list->nr_iovecs, len);


	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		/* skip already used-up iovecs */
		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;


		pr_debug("%s %d: iov idx %d. len 0x%x\n",
					__func__, __LINE__,
					 iovec_idx, iov[iovec_idx].iov_len);

		/* break up copies to not cross page boundary */
		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			pr_debug("%s %d: add dst buf page %p. len 0x%x offset 0x%x\n",
					__func__, __LINE__,
					page_list->pages[page_idx], copy, iov_byte_offset);
			sg_set_page(dst_sg, page_list->pages[page_idx],
					copy, iov_byte_offset);

			dst_sg = sg_next(dst_sg);
			sg_nents++;

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			page_idx++;
			if (!len)
				return sg_nents;

			offset += copy;
			iov_byte_offset = 0;
		}
		iovec_idx++;
	}

	/* really bad if we ever run out of iovecs */
	BUG();
	return -EFAULT;
}
/*
 * We have already pinned down the pages we will be using in the iovecs.
 * Each entry in iov array has corresponding entry in pinned_list->page_list.
 * Using array indexing to keep iov[] and page_list[] in sync.
 * Initial elements in iov array's iov->iov_len will be 0 if already copied into
 *   by another call.
 * iov array length remaining guaranteed to be bigger than len.
 */
dma_cookie_t dma_memcpy_to_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, unsigned char *kdata, size_t len)
{
	int iov_byte_offset;
	int copy;
	dma_cookie_t dma_cookie = 0;
	int iovec_idx;
	int page_idx;

	if (!chan)
		return memcpy_toiovec(iov, kdata, len);

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		/* skip already used-up iovecs */
		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		/* break up copies to not cross page boundary */
		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			dma_cookie = dma_async_memcpy_buf_to_pg(chan,
					page_list->pages[page_idx],
					iov_byte_offset,
					kdata,
					copy);
			/* poll for a descriptor slot */
			if (unlikely(dma_cookie < 0)) {
				dma_async_issue_pending(chan);
				continue;
			}

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			if (!len)
				return dma_cookie;

			kdata += copy;
			iov_byte_offset = 0;
			page_idx++;
		}
		iovec_idx++;
	}

	/* really bad if we ever run out of iovecs */
	BUG();
	return -EFAULT;
}

dma_cookie_t dma_memcpy_pg_to_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, struct page *page,
	unsigned int offset, size_t len)
{
	int iov_byte_offset;
	int copy;
	dma_cookie_t dma_cookie = 0;
	int iovec_idx;
	int page_idx;
	int err;

	/* this needs as-yet-unimplemented buf-to-buff, so punt. */
	/* TODO: use dma for this */
	if (!chan || !pinned_list) {
		u8 *vaddr = kmap(page);
		err = memcpy_toiovec(iov, vaddr + offset, len);
		kunmap(page);
		return err;
	}

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		/* skip already used-up iovecs */
		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		/* break up copies to not cross page boundary */
		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			dma_cookie = dma_async_memcpy_pg_to_pg(chan,
					page_list->pages[page_idx],
					iov_byte_offset,
					page,
					offset,
					copy);
			/* poll for a descriptor slot */
			if (unlikely(dma_cookie < 0)) {
				dma_async_issue_pending(chan);
				continue;
			}

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			if (!len)
				return dma_cookie;

			offset += copy;
			iov_byte_offset = 0;
			page_idx++;
		}
		iovec_idx++;
	}

	/* really bad if we ever run out of iovecs */
	BUG();
	return -EFAULT;
}
