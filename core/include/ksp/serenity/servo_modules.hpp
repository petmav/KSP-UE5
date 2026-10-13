// WP-52: the four Breaking Ground servo modules (Assembly-CSharp/Expansions/Serenity): ModuleRoboticServoHinge.cs, ModuleRoboticServoPiston.cs,
// ModuleRoboticServoRotor.cs, ModuleRoboticRotationServo.cs, on top of BaseServo (base_servo.hpp), and HeadlessServoPlant (the reference plant of
// the joint seam, servo_joint.hpp).
//
// Ported 1:1: target / traverse-velocity / damping handling, the soft (UI) min-max limits and the hard limits, the per-tick MoveTowards of the drive
// target (traverseVelocity * fixedDeltaTime), the joint drive configuration (position spring / damper / maximum force from the motor output,
// the limit springs, the power-loss lock), the lock angle bookkeeping, mirrored / symmetric hinge handling (CheckSymmetry + mirrorRotation +
// OnEditorPartEvent), the rotor spool-up / brake / ratchet, the piston extension and slave transforms, the drag-cube weights.
//
// Not ported (UI / PAW / audio / FX): UI_FloatRange / UI_MinMaxRange / UI_Cycle control limits and visibility, the part-action-window items, GetInfo /
// GetModuleDisplayName text, the localized strings caches, Debug.Log. The editor scene's GameObject re-parenting (BaseServo.SetChildParentTransform) is the
// host's scene graph.
#pragma once

#include "ksp/serenity/base_servo.hpp"

namespace ksp::Expansions::Serenity {

// ---------------------------------------------------------------------------------------------------------------- hinge
// ModuleRoboticServoHinge.cs:6
class ModuleRoboticServoHinge : public BaseServo {
public:
    explicit ModuleRoboticServoHinge(const ModuleClassInfo& cls) : BaseServo(cls) {}

    // [KSPField] accessors
    Vector2f softMinMaxAngles() const { return Get<Vector2f>("softMinMaxAngles"); }   // :10
    float targetAngle() const { return Get<float>("targetAngle"); }                   // :17
    float modelInitialAngle() const { return Get<float>("modelInitialAngle"); }       // :20
    bool mirrorRotation() const { return Get<bool>("mirrorRotation"); }               // :23
    float currentAngle() const { return Get<float>("currentAngle"); }                 // :30
    float traverseVelocity() const { return Get<float>("traverseVelocity"); }         // :34
    float hingeDamping() const { return Get<float>("hingeDamping"); }                 // :41
    float previousTargetAngle() const { return Get<float>("previousTargetAngle"); }   // :71

    // non-field state
    bool checkSymmetry = false;                 // :25
    float workingJointTargetAngle = 0.0f;       // :27
    std::string pistonTransforms = "Bar1,Bar2,TopPlateKnuckle,BasePlateKnuckle";  // :74 [SerializeField]
    float displacementLimitIgnoreRange = 0.025f;          // :77
    float displacementLimitIgnoreFrameDistance = 0.025f;  // :80
    SoftJointLimit upperLimit, lowerLimit;      // :83 :86
    SoftJointLimitSpring xSpringLimit;          // :89
    JointDrive xDrive;                          // :92
    Quaternionf editorRotation = Quaternionf::identity();  // :95
    float lockedTargetAngle = 0.0f;             // :98
    bool initComplete = false;                  // :100
    BaseAxisField* targetAngleAxisField = nullptr, *traverseVelocityAxisField = nullptr;  // :104 :108
    float lastDragCubeAngle = 0.0f;             // :112
    float powerLossLockAngle = 0.0f;            // :114
    float maxAnglePerFrame = 0.0f;              // :116 public
    float driveTargetAngle = 0.0f;              // :118 public
    float angleDiff = 0.0f;                     // :120
    float lowerDisplacementLimit = 0.0f, upperDisplacementLimit = 0.0f;  // :122 :124
    std::vector<std::string> togglePistonsLog;  // (C++) the GameObject.SetActive calls TogglePistons made: "name=0/1"

    float JointTargetAngle();                                  // :126
    bool IsMultipleCubesActive() const { return useMultipleDragCubes(); }  // :137
    void MaximumAngleAction(KSPActionParam&) { SetMaximumAngle(); }  // :140
    void MinimumAngleAction(KSPActionParam&) { SetMinimumAngle(); }  // :146
    void ToggleHingeAction(KSPActionParam&);                   // :152
    void SetMinimumAngle();                                    // :164
    void SetMaximumAngle();                                    // :177
    void CheckSymmetry();                                      // :190
    void OnEditorPartEvent(ConstructionEventType evt, Part* p);  // :216
    void OnStart(StartState state) override;                   // :239
    void TogglePistons(bool active);                           // :287
    void OnInventoryModeDisable() override;                    // :300
    void OnInventoryModeEnable() override;                     // :306
    void OnDestroy() override;                                 // :312
    void OnCopy(PartModule& fromModule) override;              // :328
    void OnJointInit(bool goodSetup) override;                 // :337
    void OnPostStartJointInit() override;                      // :388
    void OnPreModifyServo() override;                          // :401
    void OnVisualizeServo(bool rotateBase) override;           // :415
    void OnModifyServo() override;                             // :437
    void ResetLaunchPosition() override;                       // :474
    void ServoFixedUpdate() override;                          // :480
    bool IsMoving() override { return transformRateOfMotion > 0.049f; }  // :517
    float GetFrameDisplacement() override;                     // :522
    void SetInitialDisplacement() override { previousDisplacement = currentTransformAngle(); }  // :541
    void UpdatePAWUI(UI_Scene currentScene) override;          // :546 (state side only)
    void OnServoLockApplied() override;                        // :573
    void OnServoLockRemoved() override;                        // :578
    void OnServoMotorEngaged() override;                       // :583
    void OnServoMotorDisengaged() override { motorManualDisengaged = true; }  // :589
    void OnSaveShip() override;                                // :594
    void ModifyLimits(const ModuleValue* field);               // :607
    void ModifyTraverseLimits(const ModuleValue* field);       // :628
    Quaternionf SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation);  // :642
    void OnResourceNonemptyEmpty(PartResource* resource);      // :652
    void InitAxisFieldLimits() override;                       // :669
    void UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :679
    void UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :688
    std::vector<std::string> GetDragCubeNames() const;         // :697 (empty = null)
    bool UsesProceduralDragCubes() const { return false; }     // :714
    void SetDragCubes(float angle);                            // :719
};

// ---------------------------------------------------------------------------------------------------------------- piston
// ModuleRoboticServoPiston.cs:6
class ModuleRoboticServoPiston : public BaseServo {
public:
    explicit ModuleRoboticServoPiston(const ModuleClassInfo& cls) : BaseServo(cls) {}

    Vector2f softMinMaxExtension() const { return Get<Vector2f>("softMinMaxExtension"); }  // :10
    float targetExtension() const { return Get<float>("targetExtension"); }                // :18
    float currentExtension() const { return Get<float>("currentExtension"); }              // :28
    float traverseVelocity() const { return Get<float>("traverseVelocity"); }              // :32
    float pistonDamping() const { return Get<float>("pistonDamping"); }                    // :39

    float powerLossExtension = 0.0f;           // :21
    BaseAxisField* targetExtensionAxisField = nullptr;  // :25
    SoftJointLimit linearLimit;                // :66
    SoftJointLimitSpring linearLimitSpring;    // :69
    JointDrive jointDrive;                     // :72
    Vector3f editorPostion{0, 0, 0};           // :75
    float previousTargetPosition = 0.0f;       // :78
    float driveTargetPosition = 0.0f;          // :80 public
    float targetPosition = 0.0f;               // :82
    std::vector<std::shared_ptr<ServoNode>> slaveTransforms;  // :84 (empty = null)
    bool slaveTransformsSet = false;
    bool initComplete = false;                 // :86
    float positionDiff = 0.0f;                 // :90

    bool IsMultipleCubesActive() const { return true; }  // :92
    void OnStart(StartState state) override;           // :99
    void OnDestroy() override;                         // :190
    void ModifyLimits(const ModuleValue* field);       // :205
    void UpdateTargetPosition(const ModuleValue* field);  // :241
    void UpdateTraverseVelocity(const ModuleValue* field);  // :248
    void OnJointInit(bool goodSetup) override;         // :254
    void OnPostStartJointInit() override;              // :298
    void OnPreModifyServo() override {}                // :313
    void OnVisualizeServo(bool moveBase) override;     // :317
    void LateUpdate();                                 // :339 (the slave transforms follow the moving / base objects)
    void ServoFixedUpdate() override;                  // :354
    void SetJointTargetPosition();                     // :393
    void OnModifyServo() override;                     // :399
    void UpdateCurrentExtension();                     // :436
    void OnServoLockApplied() override;                // :446
    void OnServoLockRemoved() override;                // :453
    void OnServoMotorEngaged() override;               // :460
    void OnServoMotorDisengaged() override { motorManualDisengaged = true; }  // :467
    bool IsMoving() override { return transformRateOfMotion > 0.049f; }  // :472
    float GetFrameDisplacement() override;             // :477
    void SetInitialDisplacement() override { previousDisplacement = currentTransformPosition(); }  // :483
    void ResetLaunchPosition() override;               // :488
    void OnSaveShip() override;                        // :494
    void UpdatePAWUI(UI_Scene currentScene) override;  // :500
    Vector3f CalcTargetPosition(const Vector3f& startingPosition, float offset, bool relative);  // :531
    void ExtendPistonAction(KSPActionParam&) { ExtendPiston(); }    // :554
    void RetractPistonAction(KSPActionParam&) { RetractPiston(); }  // :560
    void TogglePistonAction(KSPActionParam&);          // :566
    void ExtendPiston();                               // :578
    void RetractPiston();                              // :591
    void OnResourceNonemptyEmpty(PartResource* resource);  // :604
    void InitAxisFieldLimits() override;               // :620
    void UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :630
    void UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :639
    std::vector<std::string> GetDragCubeNames() const { return {"0", "50", "100"}; }  // :648
    void SetDragCubes(float extension);                // :662
};

// ---------------------------------------------------------------------------------------------------------------- rotor
// ModuleRoboticServoRotor.cs:7
class ModuleRoboticServoRotor : public BaseServo {
public:
    explicit ModuleRoboticServoRotor(const ModuleClassInfo& cls) : BaseServo(cls) {}

    float rpmLimit() const { return Get<float>("rpmLimit"); }                          // :11
    float currentRPM() const { return Get<float>("currentRPM"); }                      // :14
    bool rotateCounterClockwise() const { return Get<bool>("rotateCounterClockwise"); }  // :18
    bool inverted() const { return Get<bool>("inverted"); }                            // :22
    int ratcheted() const { return Get<int32_t>("ratcheted"); }                        // :26
    float brakePercentage() const { return Get<float>("brakePercentage"); }            // :30
    std::string angularDriveMode() const { return Get<std::string>("angularDriveMode"); }  // :57

    SoftJointLimitSpring xSpringLimit;         // :60
    SoftJointLimit lowLimits, highLimits;      // :63 :66
    JointDrive xDrive;                         // :69
    Quaternionf editorRotation = Quaternionf::identity();  // :72
    bool initComplete = false;                 // :74
    BaseAxisField* rpmLimitAxisField = nullptr;  // :82
    bool brakingMode = false;                  // :86
    float freespinCurrentRotation = 0.0f;      // :88
    float freespinPreviousRotation = 0.0f;     // :90
    Quaternionf ratchetLockRotation = Quaternionf::identity();  // :92
    float ratchetTolerance = 0.2f;             // :95 [SerializeField]
    bool resourceStateChange = false;          // :97
    float positiveAngularVelocity = 0.0f;      // :99
    float brakingTorque = 0.0f;                // :101
    float totalTorque = 0.0f;                  // :103
    float tempAngularVelocity = 0.0f;          // :105
    float workingRPM = 0.0f;                   // :107
    float loadMass = 0.0f;                     // :109
    float rpmRefreshSeconds = 0.3f;            // :112 [SerializeField]
    float timeSinceRPMRefresh = 0.0f;          // :114
    bool refreshRPMReadouts = false;           // :116
    float rotationDiff = 0.0f;                 // :118

    float normalizedOutput() const { return F32Div(transformRateOfMotion, rpmLimit()); }  // :120
    void OnStart(StartState state) override;       // :122
    void OnDestroy() override;                     // :153
    void ModifyRPMLimits(const ModuleValue* field);  // :165
    void ModifyDirection(const ModuleValue* field);  // :179
    void OnUpdate() override;                      // :187
    void BrakeAction(KSPActionParam& kPar);        // :201
    void MotorPowerAction(KSPActionParam&) { ToggleMotorPower(); }      // :225
    void MotorDirectionAction(KSPActionParam&) { ToggleMotorDirection(); }  // :231
    void ToggleMotorPower();                       // :236
    void ToggleMotorDirection();                   // :256
    void OnJointInit(bool goodSetup) override;     // :269
    void OnPostStartJointInit() override;          // :316
    void OnPreModifyServo() override {}            // :326
    void OnVisualizeServo(bool rotateBase) override;  // :330
    void OnModifyServo() override;                 // :346
    void OnServoLockApplied() override { lockAngle = currentTransformAngle(); }  // :407
    void OnServoLockRemoved() override {}          // :412
    void OnServoMotorEngaged() override { motorManualDisengaged = false; }  // :416
    void OnServoMotorDisengaged() override { motorManualDisengaged = true; }  // :421
    void ServoFixedUpdate() override;              // :426
    void RatchetLock();                            // :502
    bool IsMoving() override { return transformRateOfMotion > 0.5f; }  // :568
    float GetFrameDisplacement() override;         // :573
    void SetInitialDisplacement() override { previousDisplacement = currentTransformAngle(); }  // :588
    void OnSaveShip() override;                    // :593
    void ResetLaunchPosition() override;           // :599
    void OnVesselChange(Vessel* vsl);              // :612
    void UpdateLoadMass();                         // :620
    void SetJointDrive(float torque);              // :630
    Quaternionf RotateOnMainAxis(const Quaternionf& originalRotation, float rotationAngle);  // :653
    void UpdatePAWUI(UI_Scene currentScene) override;  // :664 (state side only)
    void OnResourceNonemptyEmpty(PartResource* resource);  // :721
    void InitAxisFieldLimits() override {}         // :736
    void UpdateAxisFieldHardLimit(const std::string&, const Vector2f&) override {}  // :740
    void UpdateAxisFieldSoftLimit(const std::string&, const Vector2f&) override {}  // :744

    static float F32Div(float a, float b) { return f32store(fx(a) / fx(b)); }
};

// ---------------------------------------------------------------------------------------------------------------- rotation servo
// ModuleRoboticRotationServo.cs:7
class ModuleRoboticRotationServo : public BaseServo {
public:
    explicit ModuleRoboticRotationServo(const ModuleClassInfo& cls) : BaseServo(cls) {}

    Vector2f softMinMaxAngles() const { return Get<Vector2f>("softMinMaxAngles"); }   // :11
    bool allowFullRotation() const { return Get<bool>("allowFullRotation"); }         // :19
    float targetAngle() const { return Get<float>("targetAngle"); }                   // :24
    float currentAngle() const { return Get<float>("currentAngle"); }                 // :27
    bool inverted() const { return Get<bool>("inverted"); }                           // :31
    bool mirrorRotation() const { return Get<bool>("mirrorRotation"); }               // :34
    float traverseVelocity() const { return Get<float>("traverseVelocity"); }         // :42
    float hingeDamping() const { return Get<float>("hingeDamping"); }                 // :49

    Vector2f invertedSoftMinMaxAngles{0.0f, -177.0f};   // :13
    bool checkSymmetry = false;                 // :36
    float workingJointTargetAngle = 0.0f;       // :38
    std::string pistonTransforms = "Bar1,Bar2,TopPlateKnuckle,BasePlateKnuckle";  // :82
    SoftJointLimit upperLimit, lowerLimit;      // :85 :88
    SoftJointLimitSpring xSpringLimit;          // :91
    JointDrive xDrive;                          // :94
    Quaternionf editorRotation = Quaternionf::identity();  // :97
    bool initComplete = false;                  // :99
    BaseAxisField* targetAngleAxisField = nullptr, *traverseVelocityAxisField = nullptr;  // :103 :107
    float maxAnglePerFrame = 0.0f;              // :111 public
    float driveTargetAngle = 0.0f;              // :113 public
    float angleDiff = 0.0f;                     // :115
    Vector2f configHardMinMaxLimits{0, 0};      // :117
    Vector2f prevSoftMinMaxAngles{0, 0};        // :119

    float JointTargetAngle();                   // :121
    void MaximumAngleAction(KSPActionParam&) { SetMaximumAngle(); }  // :136
    void MinimumAngleAction(KSPActionParam&) { SetMinimumAngle(); }  // :142
    void ToggleServoAction(KSPActionParam&);    // :148
    void SetMinimumAngle();                     // :160
    void SetMaximumAngle();                     // :173
    void CheckSymmetry();                       // :186
    void OnEditorPartEvent(ConstructionEventType evt, Part* p);  // :212
    void OnStart(StartState state) override;    // :235
    void OnDestroy() override;                  // :298
    void OnCopy(PartModule& fromModule) override;  // :316
    void OnJointInit(bool goodSetup) override;  // :325
    void SetJointHighLowLimits();               // :371
    void OnPostStartJointInit() override;       // :391
    void OnPreModifyServo() override;           // :404
    void OnVisualizeServo(bool rotateBase) override;  // :418
    void OnModifyServo() override;              // :440
    void ServoFixedUpdate() override;           // :485
    bool IsMoving() override { return transformRateOfMotion > 0.049f; }  // :520
    float GetFrameDisplacement() override;      // :525
    void SetInitialDisplacement() override { previousDisplacement = currentTransformAngle(); }  // :540
    void UpdatePAWUI(UI_Scene currentScene) override;  // :545 (state side only)
    void OnServoLockApplied() override { lockAngle = currentTransformAngle(); }  // :572
    void OnServoLockRemoved() override { driveTargetAngle = currentTransformAngle(); }  // :577
    void OnServoMotorEngaged() override;        // :582
    void OnServoMotorDisengaged() override { motorManualDisengaged = true; }  // :588
    void ResetLaunchPosition() override;        // :593
    void OnSaveShip() override;                 // :599
    void ModifyTargetAngle(const ModuleValue* field);   // :612
    void ModifyLimitsMode(const ModuleValue* field);    // :629
    void ModifyLimits(const ModuleValue* field);        // :653
    void ModifyInverted(const ModuleValue* field);      // :678
    void ModifyTraverseLimits(const ModuleValue* field);  // :686
    Quaternionf SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation);  // :700
    void OnResourceNonemptyEmpty(PartResource* resource);  // :710
    void InitAxisFieldLimits() override;        // :726
    void UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :736
    void UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) override;  // :745
};

// ---------------------------------------------------------------------------------------------------------------- the reference plant
// Moves the moving transform of one servo under its ConfigurableJointData for `dt` seconds (servo_joint.hpp: a documented model of the D6 drive,
// NOT PhysX). One plant per servo. SYNTHETIC parameters (inertia).
class HeadlessServoPlant {
public:
    explicit HeadlessServoPlant(BaseServo& servo, ServoPlantParams params = {}) : servo_(servo), params_(params) {}
    // Reads the servo joint and its moving node, steps the physical coordinate, writes the node's localRotation (revolute: about the main axis) or
    // localPosition (prismatic) back. Hinge / rotation servo: position drive; rotor: velocity drive; piston: linear position drive.
    void Step(double dt);
    ServoPlantState state;
    void SetInitial(double position) { state.position = position; state.velocity = 0.0; }

private:
    BaseServo& servo_;
    ServoPlantParams params_;
};

void RegisterServoModules();  // serenity/servo_register.cpp: the four module classes (called from RegisterBuiltinModules)

}  // namespace ksp::Expansions::Serenity
