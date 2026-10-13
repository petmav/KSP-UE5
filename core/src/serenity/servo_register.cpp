// WP-52: registration of the Breaking Ground robotics module classes (ModuleFactory), called from RegisterBuiltinModules.
#include "ksp/modules/part_module.hpp"
#include "ksp/serenity/robotic_controller.hpp"
#include "ksp/serenity/servo_modules.hpp"

namespace ksp {
namespace detail {
void RegisterRoboticsModules() {
    using namespace Expansions::Serenity;
    const auto reg = [](const char* name, ModuleFactory::Creator c) { ModuleFactory::Register(name, std::move(c)); };
    reg("ModuleRoboticServoHinge", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRoboticServoHinge>(c)); });
    reg("ModuleRoboticServoPiston", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRoboticServoPiston>(c)); });
    reg("ModuleRoboticServoRotor", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRoboticServoRotor>(c)); });
    reg("ModuleRoboticRotationServo", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRoboticRotationServo>(c)); });
    reg("ModuleRoboticController", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleRoboticController>(c)); });
}
}  // namespace detail
}  // namespace ksp
