Fathom source: https://github.com/jdart1/Fathom
Pinned commit: c9c6fef0dddc05d2e242c183acf5833149ab676d
License: MIT; retained verbatim in LICENSE and upstream source headers.
Vendored files are unmodified. JACE's root fathom.c compiles this implementation
with TB_NO_THREADS, TB_NO_HELPER_API and TB_NO_HW_POP_COUNT. The first flag must
be revisited before adding SMP. No platform-specific popcount requirement is
introduced. Third-party compiler warnings are isolated from JACE's strict checks.
