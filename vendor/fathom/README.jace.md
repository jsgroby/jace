Fathom source: https://github.com/jdart1/Fathom
Pinned commit: c9c6fef0dddc05d2e242c183acf5833149ab676d
License: MIT; retained verbatim in LICENSE and upstream source headers.
Vendored files are unmodified. JACE's root fathom.c compiles this implementation
with TB_NO_HELPER_API and TB_NO_HW_POP_COUNT. Library threading is enabled for
concurrent WDL probes; init/free and root probing remain coordinator-only while
workers are idle. No platform-specific popcount requirement is
introduced. Third-party compiler warnings are isolated from JACE's strict checks.
