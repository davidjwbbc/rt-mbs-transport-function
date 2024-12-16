/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Common values and macros
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include "ogs-core.h"

int __mbstf_log_domain;

void initialise_logging(void)
{
    ogs_log_install_domain(&__mbstf_log_domain, "MBSTF", ogs_core()->log.level);
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
