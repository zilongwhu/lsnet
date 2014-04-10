/*
 * =====================================================================================
 *
 *       Filename:  hash.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月08日 21时14分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */

#ifndef __HASH_H__
#define __HASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include "slist.h"
#include "mempool.h"

typedef uint32_t __ex_hash_fun_t(const void *key);
typedef int __ex_hash_cmp_t(const void *key1, const void *key2);

typedef struct __ex_hash_arg
{
    size_t _bucket_num;
    size_t _key_size;
    size_t _value_size;

    __ex_hash_fun_t *_hash_fun;
    __ex_hash_cmp_t *_hash_cmp;
} __ex_hash_arg_t;


typedef struct __ex_hash
{
    __ex_hash_arg_t _attrs;

    size_t _node_cnt;

    __slist_t *_buckets;

    mempool_t _mp;
} __ex_hash_t;

#define ex_hash_nodenum(ptr) ((ptr) ? (ptr)->node_cnt : 0)

__ex_hash_t *ex_hash_init(__ex_hash_arg_t *args);
void ex_hash_free(__ex_hash_t *ptr);
void ex_hash_renew(__ex_hash_t *ptr);

#define EX_HASH_ADD_OK 0
#define EX_HASH_OVERWRITE 1
#define EX_HASH_EXIST 2
#define EX_HASH_ADD_FAIL -1

int ex_hash_add(__ex_hash_t *ptr, const void *key, const void *value, int ow);

#define EX_HASH_SEEK_OK 0
#define EX_HASH_SEEK_FAIL -1

int ex_hash_seek(__ex_hash_t *ptr, const void *key, void *value);

#define EX_HASH_DEL_OK 0
#define EX_HASH_DEL_FAIL -1

int ex_hash_del(__ex_hash_t *ptr, const void *key, void *value);

#ifdef __cplusplus
}
#endif

#endif
