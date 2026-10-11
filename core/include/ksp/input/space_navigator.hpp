// WP-50: SpaceNavigator (Assembly-CSharp-firstpass/SpaceNavigator.cs, SpaceNavigatorNoDevice.cs, SpaceNavigatorWindows.cs,
// QuaternionExtensions.cs Pitch/Yaw/Roll): the 6-DOF controller's sensitivity / lock model and the sensor-to-translation /
// rotation scaling. The 3Dconnexion COM sensor (TDx.TDxInput) is the seam ISpaceNavSensor; PlayerPrefs is IPlayerPrefs.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// QuaternionExtensions.cs:20 / :25 / :30 (radians)
float QuaternionPitch(const Quaternionf& q);
float QuaternionYaw(const Quaternionf& q);
float QuaternionRoll(const Quaternionf& q);

// UnityEngine.PlayerPrefs (the members SpaceNavigator.ReadSettings / WriteSettings use)
class IPlayerPrefs {
public:
    virtual ~IPlayerPrefs() = default;
    virtual float GetFloat(const std::string& key, float defaultValue) const = 0;
    virtual int GetInt(const std::string& key, int defaultValue) const = 0;
    virtual void SetFloat(const std::string& key, float value) = 0;
    virtual void SetInt(const std::string& key, int value) = 0;
};

// TDx.TDxInput Sensor.Translation / Sensor.Rotation (doubles, as the COM interop reports them)
struct SpaceNavSensorReading {
    double translationX = 0.0, translationY = 0.0, translationZ = 0.0;
    double rotationX = 0.0, rotationY = 0.0, rotationZ = 0.0, rotationAngle = 0.0;
};
class ISpaceNavSensor {
public:
    virtual ~ISpaceNavSensor() = default;
    virtual SpaceNavSensorReading Read() const = 0;
};

class SpaceNavigator {
public:
    virtual ~SpaceNavigator() = default;

    int Gears = 3;                                                    // :25
    int CurrentGear = 1;                                              // :27
    static inline std::vector<float> TransSensDefault{50.0f, 1.0f, 0.05f};     // :19
    static inline std::vector<float> TransSensMinDefault{1.0f, 0.1f, 0.01f};   // :21
    static inline std::vector<float> TransSensMaxDefault{100.0f, 10.0f, 1.0f}; // :23
    float PlayTransSens = TransSensDefault[1];                        // :35
    std::vector<float> TransSens = TransSensDefault;                  // :37
    std::vector<float> TransSensMin = TransSensMinDefault;            // :39
    std::vector<float> TransSensMax = TransSensMaxDefault;            // :41
    static constexpr float RotSensDefault = 1.0f;                     // :43
    static constexpr float RotSensMinDefault = 0.0f;                  // :45
    static constexpr float RotSensMaxDefault = 5.0f;                  // :47
    float PlayRotSens = 1.0f;                                         // :49
    float RotSens = 1.0f;                                             // :51
    float RotSensMin = 0.0f;                                          // :53
    float RotSensMax = 5.0f;                                          // :55

    // the static lock flags: the getters are true only outside play mode (Application.isPlaying == false)
    static bool LockTranslationX();   // :91
    static bool LockTranslationY();   // :107
    static bool LockTranslationZ();   // :123
    static bool LockTranslationAll(); // :139
    static bool LockRotationX();      // :155
    static bool LockRotationY();      // :171
    static bool LockRotationZ();      // :187
    static bool LockRotationAll();    // :203
    static void SetLockTranslationX(bool v) { lockTranslationX_ = v; }
    static void SetLockTranslationY(bool v) { lockTranslationY_ = v; }
    static void SetLockTranslationZ(bool v) { lockTranslationZ_ = v; }
    static void SetLockTranslationAll(bool v) { lockTranslationAll_ = v; }
    static void SetLockRotationX(bool v) { lockRotationX_ = v; }
    static void SetLockRotationY(bool v) { lockRotationY_ = v; }
    static void SetLockRotationZ(bool v) { lockRotationZ_ = v; }
    static void SetLockRotationAll(bool v) { lockRotationAll_ = v; }

    static bool Available() { return deviceAvailable_; }           // :219
    static SpaceNavigator* Instance();                             // :221 (getter)
    static void SetInstance(SpaceNavigator* value) { instance_ = value; }   // :221 (setter)
    static Vector3f Translation() { return Instance()->GetTranslation(); }  // :57
    static Quaternionf Rotation() { return Instance()->GetRotation(); }     // :71
    static Quaternionf RotationInLocalCoordSys(const Quaternionf& coordSysRotation);  // :241
    static void SetTranslationSensitivity(float newPlayTransSens);  // :246
    static void SetRotationSensitivity(float newPlayRotSens);       // :251

    virtual Vector3f GetTranslation() = 0;   // :256
    virtual Quaternionf GetRotation() = 0;   // :258
    void ReadSettings(const IPlayerPrefs& prefs);   // :262
    void WriteSettings(IPlayerPrefs& prefs) const;  // :283

    // tests / host: forget the singletons
    static void ResetStatics();

protected:
    friend class SpaceNavigatorWindows;
    static inline bool deviceAvailable_ = false;  // :23 (_deviceAvailable)
    static inline SpaceNavigator* instance_ = nullptr;

private:
    static inline bool lockTranslationX_ = false, lockTranslationY_ = false, lockTranslationZ_ = false, lockTranslationAll_ = false;
    static inline bool lockRotationX_ = false, lockRotationY_ = false, lockRotationZ_ = false, lockRotationAll_ = false;
};

// SpaceNavigatorNoDevice.cs:3
class SpaceNavigatorNoDevice final : public SpaceNavigator {
public:
    static SpaceNavigatorNoDevice* SubInstance();  // :13
    Vector3f GetTranslation() override;            // :15
    Quaternionf GetRotation() override;            // :21
    static void ResetSubInstance();

private:
    Vector3f fakeRotationInput_;     // :5 (never written in the game: zero)
    Vector3f fakeTranslationInput_;  // :7
};

// SpaceNavigatorWindows.cs:6
class SpaceNavigatorWindows final : public SpaceNavigator {
public:
    // the COM device (new DeviceClass(); Connect()): null / throwing = "No device found" (_deviceAvailable stays false)
    static inline std::function<std::shared_ptr<ISpaceNavSensor>()> sensorFactory;
    static SpaceNavigatorWindows* SubInstance();   // :18
    Vector3f GetTranslation() override;            // :20
    Quaternionf GetRotation() override;            // :30
    static void ResetSubInstance();

private:
    SpaceNavigatorWindows();                       // :40
    std::shared_ptr<ISpaceNavSensor> sensor_;
};

}  // namespace ksp
