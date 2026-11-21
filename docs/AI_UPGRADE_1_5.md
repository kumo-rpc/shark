# kmcmake 1.5-era upgrade notes

**1.7:** follow [`AI_UPGRADE.md`](AI_UPGRADE.md) (checklist + `/tmp` skeleton). This file is kept so old links still resolve.

Procedure is the same: generate under `/tmp`, replace `kmcmake/`, copy `CMakePresets.json`. Use a **1.7.0** `KMCMAKE_SRC`. Do not pass `-DKMCMAKE_GEN_EXAMPLES=ON` unless you want teaching demos in the skeleton (you still must not copy those into the real `cmake/` / sources).
