/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Response interface
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author: David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"

#include <string>

#include "common.hh"

#include "Open5GSSBIResponse.hh"

MBSTF_NAMESPACE_START

template <class CharT>
class CaseInsensitiveTraits : public std::char_traits<CharT>
{
public:
    static bool eq(CharT a, CharT b) { return std::tolower(a) == std::tolower(b); };
    static bool lt(CharT a, CharT b) { return std::tolower(a) < std::tolower(b); };
};

bool Open5GSSBIResponse::headerSet(const std::string &field, const std::string &value)
{
    typedef std::string::value_type C;
    std::basic_string_view<C, CaseInsensitiveTraits<C> > lfield(field.c_str());
    // If header already exists, replace it.
    for (auto hi = ogs_hash_first(m_response->http.headers); hi; hi = ogs_hash_next(hi)) {
        const char *hdrfield;
        char *hdrval;
        int hdrfield_len;
        ogs_hash_this(hi, reinterpret_cast<const void**>(&hdrfield), &hdrfield_len, reinterpret_cast<void**>(&hdrval));

        std::basic_string_view<C, CaseInsensitiveTraits<C> > hdr_field(hdrfield);
        if (lfield.compare(hdr_field)==0) {
            if (value == hdrval) return false;
            ogs_hash_set(m_response->http.headers, reinterpret_cast<const void*>(hdrfield), hdrfield_len, reinterpret_cast<const void*>(ogs_strdup(value.c_str())));
            return true;
        }
    }

    // Add new header
    ogs_hash_set(m_response->http.headers, ogs_strdup(field.c_str()), OGS_HASH_KEY_STRING, ogs_strdup(value.c_str()));

    return true;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
