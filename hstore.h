/*
 * $PostgreSQL: pgsql/contrib/hstore/hstore.h,v 1.8 2009/06/11 14:48:51 momjian Exp $
 */
#ifndef __HSTORE_H__
#define __HSTORE_H__

#include "fmgr.h"
#include "utils/array.h"

/* there is one of these for each key _and_ value */
/* the value points to the _end_ so that we can get the length
 * by subtraction from the previous entry.
 */

typedef struct
{
	uint32  entry;
} HEntry;

#define HENTRY_ISFIRST 0x80000000
#define HENTRY_ISNULL  0x40000000
#define HENTRY_POSMASK 0x3FFFFFFF

/* determined by the size of "endpos", though this is a bit academic
 * since currently varlenas (and hence both the input and the whole hstore)
 * have the same limit
 */
#define HSTORE_MAX_KEY_LEN 0x3FFFFFFF
#define HSTORE_MAX_VALUE_LEN 0x3FFFFFFF

typedef struct
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int4		size;
    /* array of HEntry follows */
} HStore;

#define HSHRDSIZE	(sizeof(HStore))
#define CALCDATASIZE(x, lenstr) ( (x) * 2 * sizeof(HEntry) + HSHRDSIZE + (lenstr) )

/* note multiple evaluations of x */
#define ARRPTR(x)		( (HEntry*) ( (HStore*)(x) + 1 ) )
#define STRPTR(x)		( (char*)(ARRPTR(x) + ((HStore*)x)->size * 2) )

/* note multiple/non evaluations */
#define HSE_ISFIRST(he_) (((he_).entry & HENTRY_ISFIRST) != 0)
#define HSE_ISNULL(he_) (((he_).entry & HENTRY_ISNULL) != 0)
#define HSE_ENDPOS(he_) ((he_).entry & HENTRY_POSMASK)
#define HSE_OFF(he_) (HSE_ISFIRST(he_) ? 0 : HSE_ENDPOS((&(he_))[-1]))
#define HSE_LEN(he_) (HSE_ISFIRST(he_)	\
					  ? HSE_ENDPOS(he_) \
					  : HSE_ENDPOS(he_) - HSE_ENDPOS((&(he_))[-1]))

#define HS_KEY(arr_,str_,i_) ((str_) + HSE_OFF((arr_)[2*(i_)]))
#define HS_VAL(arr_,str_,i_) ((str_) + HSE_OFF((arr_)[2*(i_)+1]))
#define HS_KEYLEN(arr_,i_) (HSE_LEN((arr_)[2*(i_)]))
#define HS_VALLEN(arr_,i_) (HSE_LEN((arr_)[2*(i_)+1]))
#define HS_VALISNULL(arr_,i_) (HSE_ISNULL((arr_)[2*(i_)+1]))

/* currently, these following macros are the _only_ places that rely
 * on internal knowledge of HEntry. Everything else should be using
 * the above macros.
 */

/* copy one key/value pair (which must be contiguous starting at
 * sptr_) into an under-construction hstore; dent_ is an HEntry*,
 * dbuf_ is the destination's string buffer, dptr_ is the current
 * position in the destination. lots of modification and multiple
 * evaluation here.
 */
#define HS_COPYITEM(dent_,dbuf_,dptr_,sptr_,klen_,vlen_,vnull_)			\
    do {																\
		memcpy((dptr_), (sptr_), (klen_)+(vlen_));						\
		(dptr_) += (klen_)+(vlen_);										\
		(dent_)++->entry = ((dptr_) - (dbuf_) - (vlen_)) & HENTRY_POSMASK; \
		(dent_)++->entry = ((((dptr_) - (dbuf_)) & HENTRY_POSMASK)		\
							 | ((vnull_) ? HENTRY_ISNULL : 0));			\
	} while(0)

/* add one key/item pair, from a Pairs structure, into an
 * under-construction hstore
 */
#define HS_ADDITEM(dent_,dbuf_,dptr_,pair_)								\
	do {																\
		memcpy((dptr_), (pair_).key, (pair_).keylen);					\
		(dptr_) += (pair_).keylen;										\
		(dent_)++->entry = ((dptr_) - (dbuf_)) & HENTRY_POSMASK;		\
		if ((pair_).isnull)												\
			(dent_)++->entry = ((((dptr_) - (dbuf_)) & HENTRY_POSMASK)	\
								 | HENTRY_ISNULL);						\
		else															\
		{																\
			memcpy((dptr_), (pair_).val, (pair_).vallen);				\
			(dptr_) += (pair_).vallen;									\
			(dent_)++->entry = ((dptr_) - (dbuf_)) & HENTRY_POSMASK;	\
		}																\
	} while (0)

/* finalize a newly-constructed hstore */
#define HS_FINALIZE(hsp_,count_,buf_,ptr_)							\
	do {															\
		int	buflen = (ptr_) - (buf_);								\
		if ((count_))												\
			ARRPTR(hsp_)[0].entry |= HENTRY_ISFIRST;				\
		if ((count_) != (hsp_)->size)								\
		{															\
			(hsp_)->size = (count_);								\
			memmove(STRPTR(hsp_), (buf_), buflen);					\
		}															\
		SET_VARSIZE((hsp_), CALCDATASIZE((count_), buflen));		\
	} while (0)

/* ensure the varlena size of an existing hstore is correct */
#define HS_FIXSIZE(hsp_,count_)											\
	do {																\
		int bl = (count_) ? HSE_ENDPOS(ARRPTR(hsp_)[2*(count_)-1]) : 0;	\
		SET_VARSIZE((hsp_), CALCDATASIZE((count_),bl));					\
	} while (0)

#define PG_GETARG_HS(x) ((HStore*)PG_DETOAST_DATUM(PG_GETARG_DATUM(x)))

typedef struct
{
	char	   *key;
	char	   *val;
	size_t		keylen;
	size_t		vallen;
	bool		isnull;
	bool		needfree;
} Pairs;

int			hstoreUniquePairs(Pairs * a, int4 l, int4 *buflen);
HStore *    hstorePairs(Pairs *pairs, int4 pcount, int4 buflen);

size_t		hstoreCheckKeyLen(size_t len);
size_t		hstoreCheckValLen(size_t len);

int         hstoreFindKey(HStore * hs, int *lowbound, char *key, int keylen);
Pairs *     hstoreArrayToPairs(ArrayType *a, int* npairs);

#define HStoreContainsStrategyNumber	7
#define HStoreExistsStrategyNumber		9
#define HStoreExistsAnyStrategyNumber	10
#define HStoreExistsAllStrategyNumber	11

/* defining HSTORE_POLLUTE_NAMESPACE=0 will prevent this; for now, we default
 * to on for the benefit of people restoring old dumps
 */
#ifndef HSTORE_POLLUTE_NAMESPACE
#define HSTORE_POLLUTE_NAMESPACE 1
#endif

#if HSTORE_POLLUTE_NAMESPACE
#define HSTORE_POLLUTE(newname_,oldname_) \
	PG_FUNCTION_INFO_V1(oldname_);		  \
	Datum oldname_(PG_FUNCTION_ARGS);	  \
	Datum newname_(PG_FUNCTION_ARGS);	  \
	Datum oldname_(PG_FUNCTION_ARGS) { return newname_(fcinfo); }
#else
#define HSTORE_POLLUTE(newname_,oldname_)
#endif

#endif   /* __HSTORE_H__ */
