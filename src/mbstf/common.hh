#ifndef _MBS_TF_COMMON_HH_
#define _MBS_TF_COMMON_HH_
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

#define MBSTF_NAMESPACE com::fiveg_mag::ref_tools
#define MBSTF_NAMESPACE_START namespace MBSTF_NAMESPACE {
#define MBSTF_NAMESPACE_STOP  }
#define MBSTF_NAMESPACE_USING using namespace MBSTF_NAMESPACE
#define MBSTF_NAMESPACE_NAME(a) MBSTF_NAMESPACE ## a

extern int __mbstf_log_domain;
#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __mbstf_log_domain

extern void initialise_logging(void);

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_COMMON_HH_ */
