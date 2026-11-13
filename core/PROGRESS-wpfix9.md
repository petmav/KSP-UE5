# wpfix9 progress
1. ContractParameter::CreateID: done (comment + test; the C# `!= null` guard is always true, HashString never null: no code change) - 10304de5
2. compare.py extra rows: done (ALLOWED_EXTRA_ROWS in compare.py: part_database 5/66/1907/5, thermal_step/vessel 6, orbit_rails_ingame/samples 900; ctest compare.extra_rows; manual bogus-row check fails on curves_eval and thermal_step) - see git log
3a. CraftProfileInfo .loadmeta cache: done (GetSFSMD5 + GenerateExcludedParts left: MD5 unused by GetSaveData; mission part filter) - git log
3b. SerenityContractDefs duplicate defaults: done (typed ContractDefs statics are the one copy; Lite node override kept) - git log
3c. CometVessel.OnStart: done (new class comet_vessel.{hpp,cpp}: OnAwake vessel.Comet, OnStart removal/type/SMA/ranges; ProtoVessel branch of the C# ported but untested (no cheap ProtoVessel fixture); events/VFX coroutine not ported) - git log
