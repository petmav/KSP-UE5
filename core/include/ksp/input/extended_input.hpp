// WP-50: ExtendedInput (Assembly-CSharp/ExtendedInput.cs): key state that also matches typed characters (Input.inputString) for
// bindings that are a character, not a KeyCode (non-US layouts). The strings are UTF-16 like the C# ones; ToUpper is
// the invariant per-char upper-casing of ksp::dotnet (CharToUpper).
#pragma once

#include <string>
#include <vector>

#include "ksp/input/input_binding.hpp"

namespace ksp {

class ExtendedInput {
public:
    // ExtendedInput.cs:23 (MonoBehaviour.LateUpdate: once per frame, after every Update)
    void LateUpdate();

    static bool GetKey(const KeyCodeExtended& key);      // :29
    static bool GetKeyDown(const KeyCodeExtended& key);  // :38
    static bool GetKeyUp(const KeyCodeExtended& key);    // :53
    // :95 DetectKeyDown(List<KeyCode> keyValues, out KeyCodeExtended key): false (and an empty key) for a null list
    static bool DetectKeyDown(const std::vector<KeyCode>* keyValues, KeyCodeExtended& key);

    // tests / host: the C# static fields
    static inline std::u16string CurrentInputString;  // :6
    static inline std::u16string LastInputString;     // :8
    static inline KeyCode LastKeyDetected = KeyCode::None;  // :14
    static void Reset();

private:
    static bool WasPressed(const std::string& key);  // :44
    static bool IsPressed(const std::string& key);   // :73

    float EmtpyFrames = 0.0f;    // :10
    float FrameThreshold = 0.0f;  // :12
};

}  // namespace ksp
