# wpfix6

Root cause: tests/test_modules.cpp:331 passed int as Vessel host; CommNetVessel::OnAwake (wp39) writes Vessel::connection -> stack corruption. Fixed by real Vessel. ASan full run in progress.
