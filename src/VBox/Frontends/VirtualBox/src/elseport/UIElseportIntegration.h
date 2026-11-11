/* $Id$ */
/** @file
 * Elseport - cleanup of completed Linux installation media.
 */
/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FEQT_INCLUDED_SRC_elseport_UIElseportIntegration_h
#define FEQT_INCLUDED_SRC_elseport_UIElseportIntegration_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
#pragma once
#endif
#include "UILibraryDefs.h"
class CMachine;
/** Removes only the completed installation's owned auxiliary files.
 * @returns True when no cleanup remains; false when installation or cleanup is pending.
 * @param machine Registered computer to inspect.
 */
SHARED_LIBRARY_STUFF bool uiElseportCleanupInstallation(CMachine &machine);
#endif
