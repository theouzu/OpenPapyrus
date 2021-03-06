/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2017 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
#include "dbinc/db_page.h"
#include "dbinc/heap.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

#ifdef HAVE_STATISTICS
/*
 * __heap_stat --
 *	Gather/print the heap statistics
 *
 * PUBLIC: int __heap_stat __P((DBC *, void *, uint32));
 */
int __heap_stat(DBC *dbc, void * spp, uint32 flags)
{
	DB * dbp;
	DB_HEAP_STAT * sp;
	DB_LOCK metalock;
	DB_MPOOLFILE * mpf;
	ENV * env;
	HEAPMETA * meta;
	db_pgno_t metapgno;
	int ret, t_ret, write_meta;
	dbp = dbc->dbp;
	env = dbp->env;
	meta = NULL;
	LOCK_INIT(metalock);
	mpf = dbp->mpf;
	sp = NULL;
	ret = t_ret = write_meta = 0;
	/* Allocate and clear the structure. */
	if((ret = __os_umalloc(env, sizeof(*sp), &sp)) != 0)
		goto err;
	memzero(sp, sizeof(*sp));
	/* Get the metadata page for the entire database. */
	metapgno = PGNO_BASE_MD;
	if((ret = __db_lget(dbc, 0, metapgno, DB_LOCK_READ, 0, &metalock)) != 0)
		goto err;
	if((ret = __memp_fget(mpf, &metapgno, dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		goto err;
	sp->heap_metaflags = meta->dbmeta.flags;
	sp->heap_pagecnt = meta->dbmeta.last_pgno + 1;
	sp->heap_pagesize = meta->dbmeta.pagesize;
	sp->heap_magic = meta->dbmeta.magic;
	sp->heap_version = meta->dbmeta.version;
	sp->heap_nregions = meta->nregions;
	sp->heap_regionsize = meta->region_size;
	if(LF_ISSET(DB_FAST_STAT)) {
		sp->heap_nrecs = meta->dbmeta.record_count;
	}
	else {
		/* Count the entries in the database. */
		if((ret = __heap_traverse(dbc, __heap_stat_callback, sp)) != 0)
			goto err;

		write_meta = !F_ISSET(dbp, DB_AM_RDONLY) &&
		    (!MULTIVERSION(dbp) || dbc->txn != NULL);
		if(write_meta) {
			ret = __memp_fput(mpf,
				dbc->thread_info, meta, dbc->priority);
			meta = NULL;
			if((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
				ret = t_ret;
			if(ret != 0)
				goto err;

			if((ret = __db_lget(dbc,
			    0, metapgno, DB_LOCK_WRITE, 0, &metalock)) != 0)
				goto err;
			if((ret = __memp_fget(mpf, &metapgno, dbc->thread_info,
			    dbc->txn, DB_MPOOL_DIRTY, &meta)) != 0)
				goto err;

			meta->dbmeta.key_count = sp->heap_nrecs;
			meta->dbmeta.record_count = sp->heap_nrecs;
		}
	}

	*(DB_HEAP_STAT**)spp = sp;

err:    /* Discard metadata page. */
	if((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;
	if(meta != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	if(ret != 0 && sp != NULL) {
		__os_ufree(env, sp);
		*(DB_BTREE_STAT**)spp = NULL;
	}

	return ret;
}

/*
 * __heap_stat_print --
 *	Display heap statistics.
 *
 * PUBLIC: int __heap_stat_print __P((DBC *, uint32));
 */
int __heap_stat_print(DBC *dbc, uint32 flags)
{
	DB * dbp;
	DB_HEAP_STAT * sp;
	ENV * env;
	int ret;
	dbp = dbc->dbp;
	env = dbp->env;
	if((ret = __heap_stat(dbc, &sp, LF_ISSET(DB_FAST_STAT))) != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Default Heap database information:");
	}
	__db_msg(env, "%lx\tHeap magic number", (u_long)sp->heap_magic);
	__db_msg(env, "%lu\tHeap version number", (u_long)sp->heap_version);
	__db_dl(env, "Underlying database page size", (u_long)sp->heap_pagesize);
	__db_dl(env, "Number of records in the database", (u_long)sp->heap_nrecs);
	__db_dl(env, "Number of external files in the database", (u_long)sp->heap_ext_files);
	__db_dl(env, "Number of database pages", (u_long)sp->heap_pagecnt);
	__db_dl(env, "Number of database regions", (u_long)sp->heap_nregions);
	__db_dl(env, "Number of pages in a region", (u_long)sp->heap_regionsize);
	__os_ufree(env, sp);
	return 0;
}
/*
 * __heap_print_cursor --
 *	Display the current cursor.
 *
 * PUBLIC: void __heap_print_cursor(DBC *);
 */
void __heap_print_cursor(DBC *dbc)
{
	COMPQUIET(dbc, NULL);
	return;
}
/*
 * __heap_stat_callback --
 *	Statistics callback.
 *
 * PUBLIC: int __heap_stat_callback __P((DBC *, PAGE *, void *, int *));
 */
int __heap_stat_callback(DBC *dbc, PAGE * h, void * cookie, int * putp)
{
	HEAPHDR * hdr;
	int i;
	DB * dbp = dbc->dbp;
	DB_HEAP_STAT * sp = (DB_HEAP_STAT *)cookie;
	*putp = 0;
	switch(TYPE(h)) {
		case P_HEAP:
		    /*
		     * We can't just use NUM_ENT, otherwise we'd mis-count split
		     * records.
		     */
		    for(i = 0; i <= HEAP_HIGHINDX(h); i++) {
			    hdr = (HEAPHDR*)P_ENTRY(dbp, h, i);
			    if(!F_ISSET(hdr, HEAP_RECSPLIT) || F_ISSET(hdr, HEAP_RECFIRST))
				    sp->heap_nrecs++;
			    if(F_ISSET(hdr, HEAP_RECBLOB)) {
				    sp->heap_nblobs++;
				    sp->heap_ext_files++;
			    }
		    }
		    break;
		case P_HEAPMETA: /* Fallthrough */
		case P_IHEAP: /* Fallthrough */
		default:
		    break;
	}
	return 0;
}

#else /* !HAVE_STATISTICS */

int __heap_stat(DBC *dbc, void * spp, uint32 flags)
{
	COMPQUIET(spp, NULL);
	COMPQUIET(flags, 0);
	return (__db_stat_not_built(dbc->env));
}
#endif
/*
 * __heap_traverse --
 *	Walk a Heap database.
 *
 * PUBLIC: int __heap_traverse __P((DBC *, int (*)(DBC *, PAGE *, void *, int *), void *));
 */
int __heap_traverse(DBC *dbc, int (*callback)(DBC *, PAGE *, void *, int *), void * cookie)
{
	DB * dbp;
	DB_LOCK lock;
	DB_MPOOLFILE * mpf;
	PAGE * h;
	db_pgno_t pgno;
	int already_put, ret, t_ret;
	dbp = dbc->dbp;
	mpf = dbp->mpf;
	LOCK_INIT(lock);
	pgno = FIRST_HEAP_DPAGE;
	for(;;) {
		already_put = 0;
		h = NULL;
		if((ret = __db_lget(dbc, 0, pgno, DB_LOCK_READ, 0, &lock)) != 0)
			break;
		if((ret = __memp_fget(mpf, &pgno, dbc->thread_info, dbc->txn, 0, &h)) != 0) {
			if(ret == DB_PAGE_NOTFOUND)
				ret = 0;
			if((t_ret = __TLPUT(dbc, lock)) != 0 && ret == 0)
				ret = t_ret;
			break;
		}

		ret = callback(dbc, h, cookie, &already_put);

		if(!already_put && (t_ret = __memp_fput(mpf, dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		if((t_ret = __TLPUT(dbc, lock)) != 0 && ret == 0)
			ret = t_ret;
		if(ret != 0)
			break;
		pgno++;
	}
	return ret;
}
