// WP-40: EditorReRootUtil.cs -- which parts can become the root of the craft and the hierarchy flip.
#pragma once

#include <functional>
#include <vector>

#include "ksp/parts/part.hpp"

namespace ksp {

// What isRootCandidate reads from EditorLogic: EditorLogic.SelectedPart and EditorLogic.fetch.ship.parts.Contains(p)
// (namespace scope: a nested struct with default member initializers cannot be used as a static member of its enclosing class on clang / gcc)
struct EditorReRootContext {
    Part* selectedPart = nullptr;
    std::function<bool(const Part*)> shipContains;
};

class EditorReRootUtil {
public:
    using Context = EditorReRootContext;
    // The context of the running editor (the host / EditorLogic fills it; unset: nothing selected, empty ship)
    static inline Context context;

    static std::vector<Part*> GetRootCandidates(const std::vector<Part*>& fromSet);   // :8
    static bool MakeRoot(Part* newRoot, Part* oldRoot);                               // :58
    static void StripInvalidSymmetrySets(Part* setRoot);                              // :75
    static bool SymmetrySetIsValid(Part* p);                                          // :82

private:
    static bool isRootCandidate(Part* p);                                             // :20
    static void StripInvalidSymmetrySets_recurseChildren(Part* p);                    // :80
};

}  // namespace ksp
