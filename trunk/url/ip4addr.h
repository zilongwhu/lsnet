/*
 * =====================================================================================
 *
 *       Filename:  ip4addr.h
 *
 *    Description:  ipv4 address parser
 *
 *        Version:  1.0
 *        Created:  2012年05月30日 22时49分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zilong.Zhu (), zilong.whu@gmail.com
 *        Company:  edu.whu
 *
 * =====================================================================================
 */
#ifndef __IPV4_ADDR_PARSER_H__
#define __IPV4_ADDR_PARSER_H__

const char *parse_dec_octet(const char *str, int *pval);
const char *parse_ipv4(const char *str, uint32_t *pval);

#endif
