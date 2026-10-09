# save_corpus: WP-04 round-trip gate over decompiled/KSP-dev/saves (*.sfs, *.craft) and decompiled/KSP-dev/Ships.
add_executable(save_corpus save_corpus.cpp)
target_link_libraries(save_corpus PRIVATE kspcore)
ksp_set_build_flags(save_corpus)
