// WP-50: ExtendedInput.cs.
#include "ksp/input/extended_input.hpp"

#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/utf16.hpp"

namespace ksp {

namespace {
std::u16string ToUpper(const std::u16string& s) {
    std::u16string r = s;
    for (char16_t& c : r) c = dotnet::CharToUpper(c);
    return r;
}
}  // namespace

// ExtendedInput.cs:16
void ExtendedInput::LateUpdate() {
    LastInputString = CurrentInputString;
    const std::u16string typed = InputEnvironment::Input().inputString();
    if (typed.empty() && EmtpyFrames <= FrameThreshold) {
        EmtpyFrames = f32store(fx(EmtpyFrames) + fx(Time::deltaTime));
        return;
    }
    CurrentInputString = ToUpper(typed);
    FrameThreshold = (CurrentInputString == LastInputString) ? 0.02f : 0.48f;
    EmtpyFrames = 0.0f;
}

// ExtendedInput.cs:31
bool ExtendedInput::GetKey(const KeyCodeExtended& key) {
    if (InputEnvironment::Input().GetKey(key.code)) return true;
    return IsPressed(key.name);
}

// ExtendedInput.cs:40
bool ExtendedInput::GetKeyDown(const KeyCodeExtended& key) {
    if (InputEnvironment::Input().GetKeyDown(key.code)) return true;
    if (!WasPressed(key.name)) return IsPressed(key.name);
    return false;
}

// ExtendedInput.cs:53
bool ExtendedInput::GetKeyUp(const KeyCodeExtended& key) {
    if (InputEnvironment::Input().GetKeyUp(key.code)) return true;
    if (WasPressed(key.name)) return !IsPressed(key.name);
    return false;
}

// ExtendedInput.cs:64
bool ExtendedInput::WasPressed(const std::string& key) {
    if (!key.empty() && !LastInputString.empty()) return LastInputString.find(ToUpper(dotnet::ToUtf16(key))) != std::u16string::npos;
    return false;
}

// ExtendedInput.cs:73
bool ExtendedInput::IsPressed(const std::string& key) {
    if (!key.empty() && !CurrentInputString.empty()) return CurrentInputString.find(ToUpper(dotnet::ToUtf16(key))) != std::u16string::npos;
    return false;
}

// ExtendedInput.cs:82
bool ExtendedInput::DetectKeyDown(const std::vector<KeyCode>* keyValues, KeyCodeExtended& key) {
    key = KeyCodeExtended();
    if (keyValues == nullptr) return false;  // Debug.LogError("Key values collection is null.")
    const IInputDevice& input = InputEnvironment::Input();
    const int count = static_cast<int>(keyValues->size());
    for (int num = 0; num < count; ++num) {
        if (input.GetKeyDown((*keyValues)[static_cast<size_t>(num)])) {
            key.code = (*keyValues)[static_cast<size_t>(num)];
            LastKeyDetected = (*keyValues)[static_cast<size_t>(num)];
            return true;
        }
    }
    if (!input.GetKey(LastKeyDetected) && !CurrentInputString.empty() && CurrentInputString != LastInputString) {
        const size_t index = CurrentInputString.size() - 1;
        if (CurrentInputString[index] != u'\b' && CurrentInputString[index] != u'\n') {
            key.name = dotnet::FromUtf16(std::u16string_view(&CurrentInputString[index], 1));
            if (LastInputString.empty()) return true;
            const size_t index2 = LastInputString.size() - 1;
            if (LastInputString[index2] != CurrentInputString[index]) return true;
        }
    }
    return false;
}

void ExtendedInput::Reset() {
    CurrentInputString.clear();
    LastInputString.clear();
    LastKeyDetected = KeyCode::None;
}

}  // namespace ksp
