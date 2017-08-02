/*
 * fs/btrfs/csum.c
 *
 * Btrfs checksum utils
 *
 * Copyright (C) 2013 Annapurna Labs Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <crypto/hash.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/crc32c.h>
#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#include "csum.h"
#include "disk-io.h"

#include <asm/cputype.h>


#ifdef CONFIG_BTRFS_FS_AHASH_CRC

struct crc_req_data {
	struct scatterlist sg;
	struct completion completion;
	int req_err;
};

struct csum_mpage_data {
	struct scatterlist sg;
	unsigned int pages_n;
	atomic_t pages_counter;
	struct completion completion;
};

static struct kmem_cache *btrfs_mpage_cache;

DEFINE_PER_CPU(struct crypto_ahash *, tfm_percpu);


static void crc_complete(struct crypto_async_request *req, int err)
{
	struct crc_req_data *req_data = req->data;

	if (err == -EINPROGRESS)
		return;

	req_data->req_err = err;
	complete(&req_data->completion);
}

static void crc_mpage_complete(struct crypto_async_request *base, int err)
{
	struct csum_mpage_data *pdata = base->data;
	struct ahash_request *req;

	if (err == -EINPROGRESS)
		return;

	BUG_ON(err);

	if (atomic_inc_return(&pdata->pages_counter) == pdata->pages_n)
		complete(&pdata->completion);

	req = container_of(base, struct ahash_request, base);
	ahash_request_free(req);
}

static int _send_hash_request(struct ahash_request *req,
			      struct page *page,
			      unsigned int offset,
			      size_t len,
			      u32 *result,
			      crypto_completion_t complete,
			      void *comp_data,
			      struct scatterlist *sg)
{
	BUG_ON(len+offset > PAGE_SIZE);

	ahash_request_set_callback(req, 0, complete, comp_data);

	sg_init_table(sg, 1);
	sg_set_page(sg, page, len, offset);

	ahash_request_set_crypt(req, sg, (u8 *)result, len);

	return crypto_ahash_digest(req);
}

void btrfs_csum_page_digest(
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	struct ahash_request *req;
	struct crc_req_data req_data;
	int ret;
	struct crypto_ahash *tfm;

	init_completion(&req_data.completion);

	tfm = get_cpu_var(tfm_percpu);

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	BUG_ON(!req);

	ret = _send_hash_request(req, page, offset, len, result, crc_complete,
				 &req_data, &req_data.sg);

	put_cpu_var(tfm_percpu);

	/* wait */
	if (ret == -EINPROGRESS || ret == -EBUSY) {
		wait_for_completion(&req_data.completion);
		ret = req_data.req_err;
	}
	BUG_ON(ret);

	ahash_request_free(req);
}

void *btrfs_csum_mpage_init(unsigned int pages_n)
{
	struct csum_mpage_data *pdata;

	pdata = kmem_cache_alloc(btrfs_mpage_cache, GFP_KERNEL);
	BUG_ON(!pdata);

	pdata->pages_n = pages_n;
	atomic_set(&pdata->pages_counter, 0);

	init_completion(&pdata->completion);

	return pdata;
}

void btrfs_csum_mpage_digest(void *mpage_priv,
			     struct page *page, unsigned int offset,
			     size_t len, u32 *result)
{
	struct csum_mpage_data *pdata = (struct csum_mpage_data *)mpage_priv;
	struct ahash_request *req;
	int ret;
	struct crypto_ahash *tfm = get_cpu_var(tfm_percpu);

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	BUG_ON(!req);

	ret = _send_hash_request(req, page, offset, len, result,
				 crc_mpage_complete,
				 pdata,
				 &pdata->sg);

	put_cpu_var(tfm_percpu);

	if (ret != -EINPROGRESS && ret != -EBUSY)
		BUG();
}

void btrfs_csum_mpage_final(void *mpage_priv)
{
	struct csum_mpage_data *pdata = (struct csum_mpage_data *)mpage_priv;

	wait_for_completion(&pdata->completion);

	kmem_cache_free(btrfs_mpage_cache, pdata);
}

int btrfs_csum_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct crypto_ahash *tfm = crypto_alloc_ahash("crc32c", 0, 0);
		if (IS_ERR(tfm))
			goto free_hashes;

		per_cpu(tfm_percpu, cpu) = tfm;
	}

	btrfs_mpage_cache = kmem_cache_create("btrfs_mpage_cache",
							sizeof(struct csum_mpage_data), 0,
							SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
							NULL);

	if (btrfs_mpage_cache == NULL)
		goto free_hashes;

	return 0;

free_hashes:
	btrfs_csum_exit();
	return -ENOMEM;
}

void btrfs_csum_exit(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct crypto_ahash *tfm = per_cpu(tfm_percpu, cpu);
		if (tfm)
			crypto_free_ahash(tfm);
	}

	if (btrfs_mpage_cache != NULL)
		kmem_cache_destroy(btrfs_mpage_cache);
}

#elif defined(CONFIG_ARCH_ALPINE) && defined(CONFIG_BTRFS_AL_FAST_CRC_DMA)
#include "../../arch/arm/mach-alpine/include/al_hal/al_hal_ssm_crc_memcpy.h"
#include "../../drivers/crypto/al/al_crypto.h"


#define CRC_SIZE 4

extern struct al_ssm_dma *al_btrfs_crc_dma[];
extern int al_btrfs_crc_dma_qid[];
extern int al_btrfs_crc_q_count;

/* One Q per CPU */
int crc_q_cnt[NR_CPUS];
struct al_crc_transaction xaction[NR_CPUS];
struct al_buf xact_bufs[NR_CPUS];
u32 zero_crc;

static inline void btrfs_csum_page_digest_sw(struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	u32 crc;
	void *kaddr = kmap_atomic(page);
	crc = btrfs_csum_data(kaddr + offset, ~(u32)0, len);
	kunmap_atomic(kaddr);
	btrfs_csum_final(crc, (char *)result);
}

static inline int fast_crc_dma(int cpuid,
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	int status;

	xaction[cpuid].src.bufs[0].addr = pfn_to_dma(NULL, page_to_pfn(page))
							+ offset;
	xaction[cpuid].src.bufs[0].len = len;
	xaction[cpuid].crc_out.addr = dma_map_single(NULL, result, CRC_SIZE,
							DMA_BIDIRECTIONAL);
	status = al_crc_csum_prepare(al_btrfs_crc_dma[cpuid],
			al_btrfs_crc_dma_qid[cpuid],
			   &xaction[cpuid]);
	if (likely(!status)) {
		al_crc_memcpy_dma_action(al_btrfs_crc_dma[cpuid],
			al_btrfs_crc_dma_qid[cpuid], xaction[cpuid].tx_descs_count);
		crc_q_cnt[cpuid] += 1;
		return 0;
	}
	return -1;
}

void btrfs_csum_page_digest(
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	int cpuid = smp_processor_id();
	u32 status;

	if (fast_crc_dma(cpuid, page, offset, len, result)) {
		btrfs_csum_page_digest_sw(page, offset, len, result);
		return;
	}
	while (crc_q_cnt[cpuid]) {
		if (al_crc_memcpy_dma_completion(al_btrfs_crc_dma[cpuid],
			al_btrfs_crc_dma_qid[cpuid],
			&status))
			crc_q_cnt[cpuid] -= 1;
	}
}

void *btrfs_csum_mpage_init(unsigned int pages_n)
{
	/* Nothing to do here... */
	return 0;
}

void btrfs_csum_mpage_digest(
	void		*mpage_priv,
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	int cpuid = smp_processor_id();
	if (fast_crc_dma(cpuid, page, offset, len, result))
		btrfs_csum_page_digest_sw(page, offset, len, result);
}

void btrfs_csum_mpage_final(void *mpage_priv)
{
	int cpuid = smp_processor_id();
	u32 status;

	while (crc_q_cnt[cpuid]) {
		if (al_crc_memcpy_dma_completion(al_btrfs_crc_dma[cpuid],
				al_btrfs_crc_dma_qid[cpuid],
				&status))
			crc_q_cnt[cpuid] -= 1;
	}
}

int btrfs_csum_init(void)
{
	int i;
	static bool once;
	BUG_ON(al_btrfs_crc_q_count < NR_CPUS);
	/* initalize on first mount */
	if (!once) {
		for (i = 0; i < NR_CPUS; i++) {
			crc_q_cnt[i] = 0;
			memset((void *)&xaction[i], 0, sizeof(struct al_crc_transaction));
			xaction[i].crcsum_type = AL_CRC_CHECKSUM_CRC32C;
			xaction[i].src.bufs = &xact_bufs[i];
			xaction[i].src.num = 1;
			xaction[i].crc_out.len = CRC_SIZE;
			xaction[i].xor_valid = AL_TRUE;
			xaction[i].in_xor = ~0;
			xaction[i].res_xor = ~0;
			xaction[i].crc_iv_in.addr = dma_map_single(NULL, &zero_crc, CRC_SIZE,
								DMA_BIDIRECTIONAL);
			xaction[i].crc_iv_in.len = CRC_SIZE;
		}
		once = true;
	}
	return 0;
}

void btrfs_csum_exit(void)
{
}


#else /* FAST_CRC */
void btrfs_csum_page_digest(
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	u32 crc;
	void *kaddr = kmap_atomic(page);

	crc = btrfs_csum_data(kaddr + offset, ~(u32)0, len);
	kunmap_atomic(kaddr);

	btrfs_csum_final(crc, (char *)result);
}

void *btrfs_csum_mpage_init(unsigned int pages_n)
{
	/* Nothing to do here... */
	return 0;
}

void btrfs_csum_mpage_digest(
	void		*mpage_priv,
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result)
{
	btrfs_csum_page_digest(page, offset, len, result);
}

void btrfs_csum_mpage_final(void *mpage_priv)
{
	/* Nothing to do here... */
}

int btrfs_csum_init(void)
{
	/* Nothing to do here... */
	return 0;
}

void btrfs_csum_exit(void)
{
	/* Nothing to do here... */
}

#endif

