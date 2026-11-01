/* $Id$ */
/** @file
 * Main - Reentrant release of tracked objects during collector cleanup.
 */

/*
 * This testcase is licensed under the GNU General Public License, version 3.
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "VirtualBoxBase.h"
#include "ObjectsTracker.h"
#include <iprt/errcore.h>
#include <iprt/test.h>

/** @page pg_testcase_objects_tracker Tracked-object cleanup
 * Checks that an interface released during collector cleanup can invalidate
 * itself without searching a map which is being destroyed.
 */

TrackedObjectsCollector gTrackedObjectsCollector;

static HRESULT g_hrcInvalidation;
static uint32_t g_cReleased;

/** Object whose last release re-enters the collector, as tracked objects do. */
class TrackedObjectProbe final : public IUnknown
{
public:
    TrackedObjectProbe(TrackedObjectsCollector *pCollector, const char *pszId)
        : m_cRefs(1), m_pCollector(pCollector), m_strId(pszId) {}

#ifdef VBOX_WITH_XPCOM
    STDMETHOD(QueryInterface)(REFNSIID iid, void **ppv)
#else
    STDMETHOD(QueryInterface)(REFIID iid, void **ppv)
#endif
    {
        RT_NOREF(iid);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)() { return ++m_cRefs; }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG const cRefs = --m_cRefs;
        if (!cRefs)
        {
            g_hrcInvalidation = m_pCollector->invalidateObj(m_strId);
            ++g_cReleased;
            delete this;
        }
        return cRefs;
    }

private:
    ULONG                    m_cRefs;
    TrackedObjectsCollector *m_pCollector;
    com::Utf8Str              m_strId;
};

/** Exercises explicit clear as well as destructor cleanup. */
static void tstClear(bool fExplicit)
{
    g_hrcInvalidation = S_OK;
    g_cReleased = 0;
    {
        TrackedObjectsCollector collector;
        RTTESTI_CHECK(collector.init());
        const char *pszId = "a34f25cc-6d7c-4b7c-a9c8-8a683d1e52c0";
        TrackedObjectProbe *pProbe = new TrackedObjectProbe(&collector, pszId);
        RTTESTI_CHECK(collector.setObj(pszId, "00000000-0000-0000-c000-000000000046", 0, 0, pProbe) == S_OK);
        pProbe->Release();
        if (fExplicit)
        {
            RTTESTI_CHECK(collector.clear(TrackedObjectsCollector::ForceClearing) == S_OK);
            collector.uninit();
        }
    }
    RTTESTI_CHECK(g_cReleased == 1);
    RTTESTI_CHECK(g_hrcInvalidation == VBOX_E_OBJECT_NOT_FOUND);
}

int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstObjectsTracker", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);
    RTTestSub(hTest, "Reentrant explicit clear");
    tstClear(true);
    RTTestSub(hTest, "Reentrant destructor clear");
    tstClear(false);
    return RTTestSummaryAndDestroy(hTest);
}
