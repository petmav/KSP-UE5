// WP-50: SpaceNavigator.cs, SpaceNavigatorNoDevice.cs, SpaceNavigatorWindows.cs, QuaternionExtensions.cs (Pitch / Yaw / Roll).
#include "ksp/input/space_navigator.hpp"

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {

// QuaternionExtensions.cs:20
float QuaternionPitch(const Quaternionf& q) {
    return Mathf::Atan2(f32store(2.0 * (fx(q.y) * fx(q.z) + fx(q.w) * fx(q.x))),
                        f32store(fx(q.w) * fx(q.w) - fx(q.x) * fx(q.x) - fx(q.y) * fx(q.y) + fx(q.z) * fx(q.z)));
}

// QuaternionExtensions.cs:25
float QuaternionYaw(const Quaternionf& q) { return Mathf::Asin(f32store(-2.0 * (fx(q.x) * fx(q.z) - fx(q.w) * fx(q.y)))); }

// QuaternionExtensions.cs:30
float QuaternionRoll(const Quaternionf& q) {
    return Mathf::Atan2(f32store(2.0 * (fx(q.x) * fx(q.y) + fx(q.w) * fx(q.z))),
                        f32store(fx(q.w) * fx(q.w) + fx(q.x) * fx(q.x) - fx(q.y) * fx(q.y) - fx(q.z) * fx(q.z)));
}

// SpaceNavigator.cs:78
bool SpaceNavigator::LockTranslationX() { return lockTranslationX_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:94
bool SpaceNavigator::LockTranslationY() { return lockTranslationY_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:110
bool SpaceNavigator::LockTranslationZ() { return lockTranslationZ_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:126
bool SpaceNavigator::LockTranslationAll() { return lockTranslationAll_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:142
bool SpaceNavigator::LockRotationX() { return lockRotationX_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:158
bool SpaceNavigator::LockRotationY() { return lockRotationY_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:174
bool SpaceNavigator::LockRotationZ() { return lockRotationZ_ && !InputEnvironment::applicationIsPlaying; }
// SpaceNavigator.cs:190
bool SpaceNavigator::LockRotationAll() { return lockRotationAll_ && !InputEnvironment::applicationIsPlaying; }

// SpaceNavigator.cs:210
SpaceNavigator* SpaceNavigator::Instance() {
    if (instance_ == nullptr) {
        instance_ = SpaceNavigatorWindows::SubInstance();
        if (!deviceAvailable_) instance_ = SpaceNavigatorNoDevice::SubInstance();
    }
    return instance_;
}

// SpaceNavigator.cs:234
Quaternionf SpaceNavigator::RotationInLocalCoordSys(const Quaternionf& coordSysRotation) {
    // coordSys.rotation * Rotation * Quaternion.Inverse(coordSys.rotation) (the native Inverse: the conjugate)
    const Quaternionf inv = UnityNative::Inverse(coordSysRotation);
    return coordSysRotation * Rotation() * inv;
}

// SpaceNavigator.cs:240
void SpaceNavigator::SetTranslationSensitivity(float newPlayTransSens) { Instance()->PlayTransSens = newPlayTransSens; }

// SpaceNavigator.cs:245
void SpaceNavigator::SetRotationSensitivity(float newPlayRotSens) { Instance()->PlayRotSens = newPlayRotSens; }

// SpaceNavigator.cs:256
void SpaceNavigator::ReadSettings(const IPlayerPrefs& prefs) {
    for (int i = 0; i < Gears; i++) {
        TransSens[static_cast<size_t>(i)] = prefs.GetFloat("Translation sensitivity" + std::to_string(i), TransSensDefault[static_cast<size_t>(i)]);
        TransSensMin[static_cast<size_t>(i)] =
            prefs.GetFloat("Translation sensitivity minimum" + std::to_string(i), TransSensMinDefault[static_cast<size_t>(i)]);
        TransSensMax[static_cast<size_t>(i)] =
            prefs.GetFloat("Translation sensitivity maximum" + std::to_string(i), TransSensMaxDefault[static_cast<size_t>(i)]);
    }
    lockTranslationAll_ = prefs.GetInt("Translation lock all", 0) == 1;
    lockTranslationX_ = prefs.GetInt("Translation lock X", 0) == 1;
    lockTranslationY_ = prefs.GetInt("Translation lock Y", 0) == 1;
    lockTranslationZ_ = prefs.GetInt("Translation lock Z", 0) == 1;
    RotSens = prefs.GetFloat("Rotation sensitivity", 1.0f);
    RotSensMin = prefs.GetFloat("Rotation sensitivity minimum", 0.0f);
    RotSensMax = prefs.GetFloat("Rotation sensitivity maximum", 5.0f);
    lockRotationAll_ = prefs.GetInt("Rotation lock all", 0) == 1;
    lockRotationX_ = prefs.GetInt("Rotation lock X", 0) == 1;
    lockRotationY_ = prefs.GetInt("Rotation lock Y", 0) == 1;
    lockRotationZ_ = prefs.GetInt("Rotation lock Z", 0) == 1;
}

// SpaceNavigator.cs:276
void SpaceNavigator::WriteSettings(IPlayerPrefs& prefs) const {
    for (int i = 0; i < Gears; i++) {
        prefs.SetFloat("Translation sensitivity" + std::to_string(i), TransSens[static_cast<size_t>(i)]);
        prefs.SetFloat("Translation sensitivity minimum" + std::to_string(i), TransSensMin[static_cast<size_t>(i)]);
        prefs.SetFloat("Translation sensitivity maximum" + std::to_string(i), TransSensMax[static_cast<size_t>(i)]);
    }
    prefs.SetInt("Translation lock all", lockTranslationAll_ ? 1 : 0);
    prefs.SetInt("Translation lock X", lockTranslationX_ ? 1 : 0);
    prefs.SetInt("Translation lock Y", lockTranslationY_ ? 1 : 0);
    prefs.SetInt("Translation lock Z", lockTranslationZ_ ? 1 : 0);
    prefs.SetFloat("Rotation sensitivity", RotSens);
    prefs.SetFloat("Rotation sensitivity minimum", RotSensMin);
    prefs.SetFloat("Rotation sensitivity maximum", RotSensMax);
    prefs.SetInt("Rotation lock all", lockRotationAll_ ? 1 : 0);
    prefs.SetInt("Rotation lock X", lockRotationX_ ? 1 : 0);
    prefs.SetInt("Rotation lock Y", lockRotationY_ ? 1 : 0);
    prefs.SetInt("Rotation lock Z", lockRotationZ_ ? 1 : 0);
}

void SpaceNavigator::ResetStatics() {
    instance_ = nullptr;
    deviceAvailable_ = false;
    lockTranslationX_ = lockTranslationY_ = lockTranslationZ_ = lockTranslationAll_ = false;
    lockRotationX_ = lockRotationY_ = lockRotationZ_ = lockRotationAll_ = false;
    SpaceNavigatorNoDevice::ResetSubInstance();
    SpaceNavigatorWindows::ResetSubInstance();
}

namespace {
std::unique_ptr<SpaceNavigatorNoDevice> g_noDevice;
std::unique_ptr<SpaceNavigatorWindows> g_windows;
}  // namespace

// SpaceNavigatorNoDevice.cs:13
SpaceNavigatorNoDevice* SpaceNavigatorNoDevice::SubInstance() {
    if (!g_noDevice) g_noDevice.reset(new SpaceNavigatorNoDevice());
    return g_noDevice.get();
}

void SpaceNavigatorNoDevice::ResetSubInstance() { g_noDevice.reset(); }

// SpaceNavigatorNoDevice.cs:15
Vector3f SpaceNavigatorNoDevice::GetTranslation() {
    float num = (InputEnvironment::applicationIsPlaying ? PlayTransSens : TransSens[static_cast<size_t>(CurrentGear)]);
    return Vector3f((SpaceNavigator::LockTranslationX() || SpaceNavigator::LockTranslationAll()) ? 0.0f : f32store(fx(fakeTranslationInput_.x) * fx(num)),
                    (SpaceNavigator::LockTranslationY() || SpaceNavigator::LockTranslationAll()) ? 0.0f : f32store(fx(fakeTranslationInput_.y) * fx(num)),
                    (SpaceNavigator::LockTranslationZ() || SpaceNavigator::LockTranslationAll()) ? 0.0f : f32store(fx(fakeTranslationInput_.z) * fx(num)));
}

// SpaceNavigatorNoDevice.cs:21
Quaternionf SpaceNavigatorNoDevice::GetRotation() {
    float num = (InputEnvironment::applicationIsPlaying ? PlayRotSens : RotSens);
    return UnityNative::Euler(
        Vector3f((SpaceNavigator::LockRotationX() || SpaceNavigator::LockRotationAll()) ? 0.0f : f32store(fx(fakeRotationInput_.x) * fx(num)),
                 (SpaceNavigator::LockRotationY() || SpaceNavigator::LockRotationAll()) ? 0.0f : f32store(fx(fakeRotationInput_.y) * fx(num)),
                 (SpaceNavigator::LockRotationZ() || SpaceNavigator::LockRotationAll()) ? 0.0f : f32store(fx(fakeRotationInput_.z) * fx(num))));
}

// SpaceNavigatorWindows.cs:43
SpaceNavigatorWindows::SpaceNavigatorWindows() {
    SpaceNavigator::deviceAvailable_ = false;
    try {
        if (sensorFactory) sensor_ = sensorFactory();
        if (!sensor_) return;  // no device found: IsConnected / Connect failed (COMException)
        SpaceNavigator::deviceAvailable_ = true;
    } catch (const std::exception&) {
        // catch (COMException ex): "Could not initialize device" is only logged
    }
}

// SpaceNavigatorWindows.cs:17
SpaceNavigatorWindows* SpaceNavigatorWindows::SubInstance() {
    if (!g_windows) g_windows.reset(new SpaceNavigatorWindows());
    return g_windows.get();
}

void SpaceNavigatorWindows::ResetSubInstance() { g_windows.reset(); }

// SpaceNavigatorWindows.cs:19
Vector3f SpaceNavigatorWindows::GetTranslation() {
    float num = (InputEnvironment::applicationIsPlaying ? PlayTransSens : TransSens[static_cast<size_t>(CurrentGear)]);
    if (SubInstance()->sensor_) {
        const SpaceNavSensorReading r = SubInstance()->sensor_->Read();
        const float x = (SpaceNavigator::LockTranslationX() || SpaceNavigator::LockTranslationAll()) ? 0.0f : static_cast<float>(r.translationX);
        const float y = (SpaceNavigator::LockTranslationY() || SpaceNavigator::LockTranslationAll()) ? 0.0f : static_cast<float>(r.translationY);
        const float z = (SpaceNavigator::LockTranslationZ() || SpaceNavigator::LockTranslationAll()) ? 0.0f : f32store(0.0 - fx(static_cast<float>(r.translationZ)));
        // new Vector3(x, y, z) * num * 0.0001f: Vector3 * float twice
        const Vector3f v = Vector3f(x, y, z) * num;
        return v * 0.0001f;
    }
    return Vector3f(0.0f, 0.0f, 0.0f);
}

// SpaceNavigatorWindows.cs:31
Quaternionf SpaceNavigatorWindows::GetRotation() {
    float num = (InputEnvironment::applicationIsPlaying ? PlayRotSens : RotSens);
    if (SubInstance()->sensor_) {
        const SpaceNavSensorReading r = SubInstance()->sensor_->Read();
        const float angle = f32store(fx(static_cast<float>(r.rotationAngle)) * fx(num) * fx(0.0008f));
        const float ax = (SpaceNavigator::LockRotationX() || SpaceNavigator::LockRotationAll()) ? 0.0f : f32store(0.0 - fx(static_cast<float>(r.rotationX)));
        const float ay = (SpaceNavigator::LockRotationY() || SpaceNavigator::LockRotationAll()) ? 0.0f : f32store(0.0 - fx(static_cast<float>(r.rotationY)));
        const float az = (SpaceNavigator::LockRotationZ() || SpaceNavigator::LockRotationAll()) ? 0.0f : static_cast<float>(r.rotationZ);
        return UnityNative::AngleAxis(angle, Vector3f(ax, ay, az));
    }
    return Quaternionf::identity();
}

}  // namespace ksp
