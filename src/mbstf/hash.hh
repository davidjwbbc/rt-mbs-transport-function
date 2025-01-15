/*
License: 5G-MAG Public License (v1.0)
Copyright: (C) 2024 British Broadcasting Corporation

For full license terms please see the LICENSE file distributed with this
program. If this file is missing then the license can be retrieved from
https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
*/

#ifndef MSAF_HASH_H
#define MSAF_HASH_H

#include <string>
#include <vector>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>

template <class T, class A>
std::string calculate_hash(const std::vector<T,A> &buf)
{
    size_t result_len = gnutls_hash_get_len(GNUTLS_DIG_SHA256);
    unsigned char result[result_len];
    gnutls_datum_t data = {
        .data = reinterpret_cast<unsigned char*>(const_cast<T*>(buf.data())),
        .size = static_cast<unsigned int>(sizeof(T) * buf.size())
    };
    char hash[result_len*2 + 1];

    gnutls_fingerprint(GNUTLS_DIG_SHA256, &data, result, &result_len);
    for (size_t i = 0; i < result_len; i++)
    {
        sprintf(hash+i*2, "%02x", result[i]);
    }

    return std::string(hash);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* MSAF_HASH_H */
