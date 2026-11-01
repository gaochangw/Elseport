/* $Id$ */
/** @file
 * Guest/Host - opt-in desktop scale negotiation.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef VBOX_INCLUDED_GuestHost_DisplayScale_h
#define VBOX_INCLUDED_GuestHost_DisplayScale_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** A running session advertises this only for a supported single-monitor desktop. */
#define VBOX_DISPLAY_SCALE_CAPABILITY    "/VirtualBox/GuestAdd/Display/AutoScale"
/** Version of the GNOME single-monitor scale protocol. */
#define VBOX_DISPLAY_SCALE_GNOME_SINGLE  "gnome-single-v1"
/** Host request: integer percent (100, 200 or 300), colon, fresh opaque token. */
#define VBOX_DISPLAY_SCALE_REQUEST       "/VirtualBox/Host/Display/ScaleRequest"
/** Guest acknowledgement: the complete request, after successfully applying it. */
#define VBOX_DISPLAY_SCALE_APPLIED       "/VirtualBox/GuestAdd/Display/ScaleApplied"

#endif /* !VBOX_INCLUDED_GuestHost_DisplayScale_h */
