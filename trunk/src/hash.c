/*
 * =====================================================================================
 *
 *       Filename:  hash.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月08日 21时26分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

__ex_hash_t *ex_hash_init(__ex_hash_arg_t *args)
{
    if ( NULL == args )
    {
        return NULL;
    }
    if ( 0 == args->_key_size
            || 0 == args->_value_size
            || 0 == args->_bucket_num
            || NULL == args->_hash_fun
            || NULL == args->_hash_cmp )
    {
        return NULL;
    }
    __ex_hash_t *res = (__ex_hash_t *)calloc(1, sizeof(__ex_hash_t));
    if ( NULL == res )
    {
        return NULL;
    }
    memcpy(&res->_attrs, args, sizeof(*args));
    res->_buckets = (__slist_t *)calloc(res->_attrs._bucket_num, sizeof(__slist_t));
    if ( NULL == res->_buckets )
    {
        goto ERR;
    }
    res->_mp = mp_init(sizeof(__slist_t) + res->_attrs._key_size + res->_attrs._value_size);
    if ( NULL == res->_mp )
    {
        goto ERR;
    }
    return res;
ERR:
    ex_hash_free(res);
    return NULL;
}

void ex_hash_free(__ex_hash_t *ptr)
{
    if ( NULL == ptr )
    {
        return ;
    }
    if ( NULL != ptr->_buckets )
    {
        free(ptr->_buckets);
        ptr->_buckets = NULL;
    }
    if ( NULL != ptr->_mp )
    {
        mp_close(ptr->_mp);
        ptr->_mp = NULL;
    }
    free(ptr);
}

void ex_hash_renew(__ex_hash_t *ptr)
{
    if ( NULL == ptr )
    {
        return ;
    }
    memset(ptr->_buckets, 0, ptr->_attrs._bucket_num * sizeof(__slist_t));
    mp_renew(ptr->_mp);
}

int ex_hash_add(__ex_hash_t *ptr, const void *key, const void *value, int ow)
{
    if ( NULL == ptr || NULL == key || NULL == value )
    {
        return EX_HASH_ADD_FAIL;
    }
    uint32_t idx = ptr->_attrs._hash_fun(key) % ptr->_attrs._bucket_num;

    __slist_t *cur = SLIST_NEXT(&ptr->_buckets[idx]);
    while ( cur )
    {
        if ( ptr->_attrs._hash_cmp(key, cur + 1) == 0 )
        {
            if ( ow )
            {
                memcpy(((uint8_t *)(cur + 1)) + ptr->_attrs._key_size, value, ptr->_attrs._value_size);
                return EX_HASH_OVERWRITE;
            }
            return EX_HASH_EXIST;
        }
        cur = SLIST_NEXT(cur);
    }
    __slist_t *pnode = (__slist_t *)mp_alloc(ptr->_mp);
    if ( NULL == pnode )
    {
        return EX_HASH_ADD_FAIL;
    }
    SLIST_INIT(pnode);
    SLIST_INSERT_A(&ptr->_buckets[idx], pnode);
    memcpy(pnode + 1, key, ptr->_attrs._key_size);
    memcpy(((uint8_t *)(pnode + 1)) + ptr->_attrs._key_size, value, ptr->_attrs._value_size);
    ++ptr->_node_cnt;
    return EX_HASH_ADD_OK;
}

int ex_hash_seek(__ex_hash_t *ptr, const void *key, void *value)
{
    if ( NULL == ptr || NULL == key )
    {
        return EX_HASH_SEEK_FAIL;
    }
    uint32_t idx = ptr->_attrs._hash_fun(key) % ptr->_attrs._bucket_num;

    __slist_t *cur = SLIST_NEXT(&ptr->_buckets[idx]);
    while ( cur )
    {
        if ( ptr->_attrs._hash_cmp(key, cur + 1) == 0 )
        {
            if ( NULL != value )
            {
                memcpy(value, ((uint8_t *)(cur + 1)) + ptr->_attrs._key_size, ptr->_attrs._value_size);
            }
            return EX_HASH_SEEK_OK;
        }
        cur = SLIST_NEXT(cur);
    }

    return EX_HASH_SEEK_FAIL;
}

int ex_hash_del(__ex_hash_t *ptr, const void *key, void *value)
{
    if ( NULL == ptr || NULL == key )
    {
        return EX_HASH_DEL_FAIL;
    }
    uint32_t idx = ptr->_attrs._hash_fun(key) % ptr->_attrs._bucket_num;

    __slist_t *pre = &ptr->_buckets[idx];
    __slist_t *cur = SLIST_NEXT(pre);
    while ( cur )
    {
        if ( ptr->_attrs._hash_cmp(key, cur + 1) == 0 )
        {
            if ( NULL != value )
            {
                memcpy(value, ((uint8_t *)(cur + 1)) + ptr->_attrs._key_size, ptr->_attrs._value_size);
            }
            SLIST_REMOVE(pre, cur);
            mp_free(ptr->_mp, cur);
            --ptr->_node_cnt;
            return EX_HASH_DEL_OK;
        }
        pre = cur;
        cur = SLIST_NEXT(cur);
    }

    return EX_HASH_DEL_FAIL;
}
