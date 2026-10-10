# PROGRESS WP-35 (science, R&D, tech tree)

Branch wp35 (worktree .worktrees/wp35) from main ad208205.

- [x] Step 0: read spec (ResearchAndDevelopment, ScienceUtil, ScienceSubject/Data/Experiment, modules, RDTechTree), core infra surveyed.
  Findings: no science golden/capture in harness; persistent.sfs has NO ResearchAndDevelopment SCENARIO node (round trip must use a node built through the API from mirror ScienceDefs/TechTree);
  mirror has GameData/Squad/Resources/{ScienceDefs,TechTree}.cfg; ModuleScienceLab in 1.12.5 has NO GenerateScience: the lab bonus maths is ExperimentResultDialogPage.UpdatePageLabValue
  (SurfaceBonus/ContextBonus/homeworldMultiplier), science generation is ModuleScienceConverter.PostProcess.
- [ ] Step 1: include/ksp/science/science.hpp + src/science/science.cpp (data classes, ScienceUtil, RDTechTree, ResearchAndDevelopment)
- [ ] Step 2: include/ksp/modules/science_modules.hpp + src/modules/science_modules.cpp (modules)
- [ ] Step 3: registration, gate capture.part_database
- [ ] Step 4: tools/wp35/science_reference.py + tests/test_science.cpp
- [ ] Step 5: STATUS/README, Linux run
- [x] Step 1+2+3 done: science.hpp/.cpp, research_and_development.cpp, science_modules.hpp/.cpp written, builds (MSVC), capture.part_database passes (0 ULP). Registered via detail::RegisterScienceModules (after the WP-28 registrations; ModuleScienceConverter removed from the BaseConverter Reg list).
- [ ] Step 4 next: tools/wp35/science_reference.py + tests/test_science.cpp (+ tests/CMakeLists.txt, tests/data/wp35_reference.json)
- [x] Step 4a: tools/wp35/science_reference.py + tests/data/wp35_reference.json; tests/test_science.cpp has 6 cases passing (ScienceDefs, TechTree, subject maths 1880 subjects/84600 floats 0 ULP, lab page 792 values 0 ULP, converter formulas, converter FixedUpdate 7 scenarios).
- [ ] Step 4b next: more unit cases (R&D scenario round trip + tech/part state, module cfg sweep, container, experiment flow on a real vessel, ScienceUtil strings, RnDCommsStream, lab search, recovery); then full run_tests, STATUS/README, WSL run.
