// WP-50: the input-device seam. kspcore never reads an OS device: everything the ported input code takes from
// UnityEngine.Input / Time / Application is behind IInputDevice and InputEnvironment, which the host (UE5 shell or a test)
// fills. Defaults: NullInputDevice (nothing pressed, no joysticks), realtimeSinceStartup 0, Application.isPlaying true.
#pragma once

#include <string>
#include <vector>

#include "ksp/input/keycode.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// UnityEngine.Input, the members KSP's input classes read (Input.GetKey / GetKeyDown / GetKeyUp, GetAxis(string),
// GetMouseButton*, mousePosition, inputString, GetJoystickNames).
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
    virtual bool GetKey(KeyCode key) const = 0;
    virtual bool GetKeyDown(KeyCode key) const = 0;
    virtual bool GetKeyUp(KeyCode key) const = 0;
    virtual float GetAxis(const std::string& axisName) const = 0;
    virtual bool GetMouseButton(int button) const = 0;
    virtual bool GetMouseButtonDown(int button) const = 0;
    virtual bool GetMouseButtonUp(int button) const = 0;
    virtual Vector3f mousePosition() const = 0;
    // Input.inputString: the characters typed this frame (UTF-16 like the C# string)
    virtual std::u16string inputString() const = 0;
    virtual std::vector<std::string> GetJoystickNames() const = 0;
};

class NullInputDevice final : public IInputDevice {
public:
    bool GetKey(KeyCode) const override { return false; }
    bool GetKeyDown(KeyCode) const override { return false; }
    bool GetKeyUp(KeyCode) const override { return false; }
    float GetAxis(const std::string&) const override { return 0.0f; }
    bool GetMouseButton(int) const override { return false; }
    bool GetMouseButtonDown(int) const override { return false; }
    bool GetMouseButtonUp(int) const override { return false; }
    Vector3f mousePosition() const override { return Vector3f(0.0f, 0.0f, 0.0f); }
    std::u16string inputString() const override { return std::u16string(); }
    std::vector<std::string> GetJoystickNames() const override { return {}; }
};

// UnityEngine.Input as a process-wide static (like Unity's): `Device()` is never null.
struct InputEnvironment {
    static inline const IInputDevice* device = nullptr;  // null = NullInputDevice
    static const IInputDevice& Input() {
        static const NullInputDevice none;
        return device != nullptr ? *device : none;
    }
    // Time.realtimeSinceStartup (float); the host advances it (Time.time / frameCount / deltaTime are ksp::Time)
    static inline float realtimeSinceStartup = 0.0f;
    // Application.isPlaying
    static inline bool applicationIsPlaying = true;
    // Screen.height (Mouse.Update flips the y axis)
    static inline int screenHeight = 1080;
    static inline int screenWidth = 1920;
};

}  // namespace ksp
