/* $Id$ */
/** @file
 * Guest Additions - Guest Control capability detection with pending messages.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#include <iprt/test.h>
#include "../VBoxGuestR3LibGuestCtrl.cpp"

/** @page pg_testcase_guestctrl_peek Guest Control capability detection
 * Exercises the guest library against synthetic HGCM replies without opening
 * a guest device or consuming a real Guest Control message.
 */

/** Reply scenario implemented by the fake HGCM transport. */
enum TSTREPLY
{
    TSTREPLY_EMPTY,
    TSTREPLY_PENDING,
    TSTREPLY_UNSUPPORTED,
    TSTREPLY_PARTIAL,
    TSTREPLY_ERROR,
    TSTREPLY_INTERRUPTED
};

static TSTREPLY g_enmReply;
static uint32_t g_cCalls;

VBGLR3DECL(int) VbglR3HGCMCall(PVBGLIOCHGCMCALL pCall, size_t cbCall)
{
    ++g_cCalls;
    RTTESTI_CHECK_RET(pCall->u32Function == GUEST_MSG_PEEK_NOWAIT, VERR_NOT_SUPPORTED);
    RTTESTI_CHECK_RET(pCall->cParms == 16, VERR_WRONG_PARAMETER_COUNT);
    RTTESTI_CHECK_RET(cbCall == sizeof(*pCall) + 16 * sizeof(HGCMFunctionParameter), VERR_INVALID_PARAMETER);
    if (g_enmReply == TSTREPLY_ERROR)
        return VERR_IO_GEN_FAILURE;
    if (g_enmReply == TSTREPLY_UNSUPPORTED)
        return VINF_SUCCESS;
    if (g_enmReply == TSTREPLY_INTERRUPTED && g_cCalls == 1)
        return VERR_INTERRUPTED;

    HGCMFunctionParameter *paParms = (HGCMFunctionParameter *)(pCall + 1);
    uint32_t const cSet = g_enmReply == TSTREPLY_PARTIAL ? 2 : 16;
    for (uint32_t i = 0; i < cSet; ++i)
        VbglHGCMParmUInt32Set(&paParms[i], 0);
    if (g_enmReply == TSTREPLY_EMPTY)
        return VERR_TRY_AGAIN;

    VbglHGCMParmUInt32Set(&paParms[0], HOST_MSG_SESSION_CREATE);
    VbglHGCMParmUInt32Set(&paParms[1], 2);
    if (g_enmReply != TSTREPLY_PARTIAL)
    {
        VbglHGCMParmUInt32Set(&paParms[2], ~(uint32_t)sizeof(uint32_t));
        VbglHGCMParmUInt32Set(&paParms[3], 16);
    }
    return VINF_SUCCESS;
}

VBGLR3DECL(int) VbglR3HGCMConnect(const char *pszService, uint32_t *pidClient)
{
    RT_NOREF(pszService, pidClient);
    return VERR_NOT_SUPPORTED;
}

VBGLR3DECL(int) VbglR3HGCMDisconnect(uint32_t idClient)
{
    RT_NOREF(idClient);
    return VERR_NOT_SUPPORTED;
}

VBGLR3DECL(int) VbglR3QuerySessionId(uint64_t *pidSession)
{
    RT_NOREF(pidSession);
    return VERR_NOT_SUPPORTED;
}

/** Checks detection and the subsequent cached choice of message protocol. */
static void tstReply(TSTREPLY enmReply, bool fExpected, uint32_t cExpectedCalls)
{
    g_enmReply = enmReply;
    g_cCalls = 0;
    g_fVbglR3GuestCtrlHavePeekGetCancel = -1;
    RTTESTI_CHECK(vbglR3GuestCtrlSupportsPeekGetCancel(1) == fExpected);
    RTTESTI_CHECK(g_cCalls == cExpectedCalls);
    RTTESTI_CHECK(vbglR3GuestCtrlGetMsgFunctionNo(1) == (fExpected ? GUEST_MSG_GET : GUEST_MSG_WAIT));
    RTTESTI_CHECK(g_cCalls == cExpectedCalls);
}

int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstGuestCtrlPeek", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);
    RTTestSub(hTest, "No pending message");
    tstReply(TSTREPLY_EMPTY, true, 1);
    RTTestSub(hTest, "Pending message");
    tstReply(TSTREPLY_PENDING, true, 1);
    RTTestSub(hTest, "Old host and incomplete replies");
    tstReply(TSTREPLY_UNSUPPORTED, false, 1);
    tstReply(TSTREPLY_PARTIAL, false, 1);
    tstReply(TSTREPLY_ERROR, false, 1);
    RTTestSub(hTest, "Interrupted probe");
    tstReply(TSTREPLY_INTERRUPTED, true, 2);
    RTTestSub(hTest, "Restore with an already queued message");
    g_fVbglR3GuestCtrlHavePeekGetCancel = 0;
    g_enmReply = TSTREPLY_PENDING;
    g_cCalls = 0;
    RTTESTI_CHECK(VbglR3GuestCtrlSessionHasChanged(1, 2) == VINF_SUCCESS);
    RTTESTI_CHECK(vbglR3GuestCtrlSupportsPeekGetCancel(1));
    RTTESTI_CHECK(g_cCalls == 1);
    return RTTestSummaryAndDestroy(hTest);
}
