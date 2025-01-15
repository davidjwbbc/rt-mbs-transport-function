/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS Application interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author: David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-sbi.h"

#include <cctype>
#include <string>

#include "common.hh"

#include "Open5GSSBIRequest.hh"

MBSTF_NAMESPACE_START

template <class CharT>
class CaseInsensitiveTraits : public std::char_traits<CharT>
{
public:
    static bool eq(CharT a, CharT b) { return std::tolower(a) == std::tolower(b); };
    static bool lt(CharT a, CharT b) { return std::tolower(a) < std::tolower(b); };
};

std::string Open5GSSBIRequest::headerValue(const std::string &field, const std::string &defval) const
{
    typedef std::string::value_type C;
    std::basic_string_view<C, CaseInsensitiveTraits<C> > lfield(field.c_str());
    for (auto hi = ogs_hash_first(m_request->http.headers); hi; hi = ogs_hash_next(hi)) {
        std::basic_string_view<C, CaseInsensitiveTraits<C> > hdr_field(reinterpret_cast<const char*>(ogs_hash_this_key(hi)));
        if (lfield.compare(hdr_field)==0) {
            return std::string(reinterpret_cast<const char*>(ogs_hash_this_val(hi)));
        }
    }

    return defval;
}

MBSTF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
