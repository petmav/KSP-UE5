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

    int Gears = 3;                                                    // :15
    int CurrentGear = 1;                                              // :17
    static inline std::vector<float> TransSensDefault{50.0f, 1.0f, 0.05f};     // :19
    static inline std::vector<float> TransSensMinDefault{1.0f, 0.1f, 0.01f};   // :21
    static inline std::vector<float> TransSensMaxDefault{100.0f, 10.0f, 1.0f}; // :23
    float PlayTransSens = TransSensDefault[1];                        // :25
    std::vector<float> TransSens = TransSensDefault;                  // :27
    std::vector<float> TransSensMin = TransSensMinDefault;            // :29
    std::vector<float> TransSensMax = TransSensMaxDefault;            // :31
    static constexpr float RotSensDefault = 1.0f;                     // :33
    static constexpr float RotSensMinDefault = 0.0f;                  // :35
    static constexpr float RotSensMaxDefault = 5.0f;                  // :37
    float PlayRotSens = 1.0f;                                         // :39
    float RotSens = 1.0f;                                             // :41
    float RotSensMin = 0.0f;                                          // :43
    float RotSensMax = 5.0f;                                          // :45

    // the static lock flags: the getters are true only outside play mode (Application.isPlaying == false)
    static bool LockTranslationX();   // :78
    static bool LockTranslationY();   // :94
    static bool LockTranslationZ();   // :110
    static bool LockTranslationAll(); // :126
    static bool LockRotationX();      // :142
    static bool LockRotationY();      // :158
    static bool LockRotationZ();      // :174
    static bool LockRotationAll();    // :190
    static void SetLockTranslationX(bool v) { lockTranslationX_ = v; }
    static void SetLockTranslationY(bool v) { lockTranslationY_ = v; }
    static void SetLockTranslationZ(bool v) { lockTranslationZ_ = v; }
    static void SetLockTranslationAll(bool v) { lockTranslationAll_ = v; }
    static void SetLockRotationX(bool v) { lockRotationX_ = v; }
    static void SetLockRotationY(bool v) { lockRotationY_ = v; }
    static void SetLockRotationZ(bool v) { lockRotationZ_ = v; }
    static void SetLockRotationAll(bool v) { lockRotationAll_ = v; }

    static bool Available() { return deviceAvailable_; }           // :206
    static SpaceNavigator* Instance();                             // :210 (getter)
    static void SetInstance(SpaceNavigator* value) { instance_ = value; }   // :228 (setter)
    static Vector3f Translation() { return Instance()->GetTranslation(); }  // :58
    static Quaternionf Rotation() { return Instance()->GetRotation(); }     // :60
    static Quaternionf RotationInLocalCoordSys(const Quaternionf& coordSysRotation);  // :234
    static void SetTranslationSensitivity(float newPlayTransSens);  // :240
    static void SetRotationSensitivity(float newPlayRotSens);       // :245

    virtual Vector3f GetTranslation() = 0;   // :250
    virtual Quaternionf GetRotation() = 0;   // :252
    void ReadSettings(const IPlayerPrefs& prefs);   // :256
    void WriteSettings(IPlayerPrefs& prefs) const;  // :276

    // tests / host: forget the singletons
    static void ResetStatics();

protected:
    friend class SpaceNavigatorWindows;
    static inline bool deviceAvailable_ = false;  // :9 (_deviceAvailable)
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
    static SpaceNavigatorWindows* SubInstance();   // :17
    Vector3f GetTranslation() override;            // :19
    Quaternionf GetRotation() override;            // :31
    static void ResetSubInstance();

private:
    SpaceNavigatorWindows();                       // :43
    std::shared_ptr<ISpaceNavSensor> sensor_;
};

}  // namespace ksp
