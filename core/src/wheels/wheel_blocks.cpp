// WP-29a: VehiclePhysics/Block.cs, Wheel.cs, DirectDrive.cs, Solver.cs (the wheel-spin block chain and its Euler / RK4 integrator).
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp).
#include <algorithm>
#include <typeinfo>

#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::VehiclePhysics {

// ---- Block -----------------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/Block.cs:69
Block::Block() { Block::Initialize(); }  // C#'s constructor calls the virtual Initialize; derived classes repeat it in their own constructor

// VehiclePhysics/Block.cs:74
void Block::Initialize() {
    SetInputs(0);
    SetOutputs(0);
}

// VehiclePhysics/Block.cs:80
bool Block::CheckConnections() { return true; }
// VehiclePhysics/Block.cs:85
void Block::PreStep() {}
// VehiclePhysics/Block.cs:89
void Block::GetState(State&) {}
// VehiclePhysics/Block.cs:93
void Block::SetSubstepState(const State&) {}
// VehiclePhysics/Block.cs:97
void Block::ComputeStateUpstream() {}
// VehiclePhysics/Block.cs:101
void Block::EvaluateTorqueDownstream() {}
// VehiclePhysics/Block.cs:105
void Block::GetSubstepDerivative(Derivative&) {}
// VehiclePhysics/Block.cs:109
void Block::SetState(const State&) {}

// VehiclePhysics/Block.cs:113
void Block::SetInputs(int count) {
    count = Mathf::Max(count, 0);
    m_inputs.assign(static_cast<size_t>(count), nullptr);
}

// VehiclePhysics/Block.cs:119
void Block::SetOutputs(int count) {
    count = Mathf::Max(count, 0);
    m_outputs.assign(static_cast<size_t>(count), nullptr);
}

// VehiclePhysics/Block.cs:125
bool Block::Connect(Block& inputUnit, int inputSlot, Block& outputUnit, int outputSlot) {
    if (inputUnit.hasInputs() && inputSlot < static_cast<int>(inputUnit.inputs().size()) && outputUnit.hasOutputs() &&
        outputSlot < static_cast<int>(outputUnit.outputs().size())) {
        outputUnit.DisconnectOutput(outputSlot);
        inputUnit.DisconnectInput(inputSlot);
        auto connection = std::make_shared<Connection>();
        connection->input = &inputUnit;
        connection->inputSlot = inputSlot;
        connection->output = &outputUnit;
        connection->outputSlot = outputSlot;
        inputUnit.inputs()[static_cast<size_t>(inputSlot)] = connection;
        outputUnit.outputs()[static_cast<size_t>(outputSlot)] = connection;
        inputUnit.CountConnections();
        outputUnit.CountConnections();
        return true;
    }
    return false;
}

// VehiclePhysics/Block.cs:145
bool Block::Connect(Block& inputUnit, Block& outputUnit) { return Connect(inputUnit, 0, outputUnit, 0); }

// VehiclePhysics/Block.cs:178
bool Block::DisconnectInput(int inputSlot) {
    if (hasInputs() && inputSlot < static_cast<int>(inputs().size())) {
        std::shared_ptr<Connection> connection = inputs()[static_cast<size_t>(inputSlot)];
        if (connection != nullptr) {
            inputs()[static_cast<size_t>(inputSlot)] = nullptr;
            CountConnections();
            connection->output->DisconnectOutput(connection->outputSlot);
        }
        return true;
    }
    return false;
}

// VehiclePhysics/Block.cs:194
bool Block::DisconnectOutput(int outputSlot) {
    if (hasOutputs() && outputSlot < static_cast<int>(outputs().size())) {
        std::shared_ptr<Connection> connection = outputs()[static_cast<size_t>(outputSlot)];
        if (connection != nullptr) {
            outputs()[static_cast<size_t>(outputSlot)] = nullptr;
            CountConnections();
            connection->input->DisconnectInput(connection->inputSlot);
        }
        return true;
    }
    return false;
}

// VehiclePhysics/Block.cs:210
void Block::CountConnections() {
    m_connectedInputs = 0;
    for (const auto& c : m_inputs)
        if (c != nullptr) m_connectedInputs++;
    m_connectedOutputs = 0;
    for (const auto& c : m_outputs)
        if (c != nullptr) m_connectedOutputs++;
}

// ---- Wheel -----------------------------------------------------------------------------------------------------------------------------
Wheel::Wheel() { Wheel::Initialize(); }

// VehiclePhysics/Wheel.cs:76
void Wheel::RecalculateConstants() {
    I = f32store(fx(mass) * fx(radius) * fx(radius) * 0.5);
    if (I < 0.01f) I = 0.01f;
    invI = f32store(1.0 / fx(I));
}

// VehiclePhysics/Wheel.cs:86
float Wheel::AddBrakeTorque(float brakeTorque) {
    m_brakeTorque = f32store(fx(m_brakeTorque) + fx(brakeTorque));
    if (m_brakeTorque < 0.0f) m_brakeTorque = 0.0f;
    return m_brakeTorque;
}

// VehiclePhysics/Wheel.cs:96
void Wheel::ResetBrakeTorque() { m_brakeTorque = 0.0f; }

// VehiclePhysics/Wheel.cs:101
void Wheel::RecalculateVars() { w = f32store(fx(L) * fx(invI)); }

// VehiclePhysics/Wheel.cs:106
void Wheel::Initialize() {
    SetInputs(1);
    SetOutputs(0);
    tireFriction = std::make_shared<TireFriction>();
}

// VehiclePhysics/Wheel.cs:113
void Wheel::GetState(State& S) {
    S.L = L;
    S.P = Vector2f::zero();
    S.Lr = 0.0f;
    isResting = true;
}

// VehiclePhysics/Wheel.cs:121
void Wheel::SetSubstepState(const State& S) {
    L = S.L;
    P = S.P;
    RecalculateVars();
}

// VehiclePhysics/Wheel.cs:128
void Wheel::ComputeStateUpstream() {
    m_forwardSlip = f32store(fx(w) * fx(radius) - fx(V.y));
    Vector2f slip(V.x, m_forwardSlip);
    m_contact.load = load;
    m_contact.slip = slip;
    m_contact.groundGrip = grip;
    m_tireForce = tireFriction->GetForce(m_contact);
    m_Ty = f32store(fx(m_tireForce.y) * fx(radius));
    m_isAdherent = tireFriction->IsAdherentSlip(m_contact);
    if (m_isAdherent) {
        m_adherentSlip.x = slip.x;
        m_adherentSlip.y = tireFriction->GetAdherentSlipForward(m_contact);
        m_contact.slip = m_adherentSlip;
        m_contact.slip.y = f32store(fx(m_contact.slip.y) * fx(Mathf::Sign(slip.y)));
        m_adherentForce = tireFriction->GetAdherentForce(m_contact);
        const float num = f32store(fx(m_adherentForce.y) * fx(radius));
        if (MathUtility::FastAbs(m_Ty) < MathUtility::FastAbs(num)) m_Ty = num;
    } else {
        m_adherentSlip = Vector2f::zero();
        m_adherentForce = Vector2f::zero();
    }
    std::shared_ptr<Connection> connection = inputs()[0];
    if (connection != nullptr) {
        connection->L = L;
        connection->I = I;
        connection->Tr = Tr;
    }
}

// VehiclePhysics/Wheel.cs:165
void Wheel::EvaluateTorqueDownstream() {
    m_inTd = inputs()[0] != nullptr ? inputs()[0]->outTd : 0.0f;
    m_inTb = m_brakeTorque;
    const float num = f32store(fx(m_inTb) * fx(Mathf::Sign(f32store(0.0 - fx(L)))));
    float tsum = f32store(fx(m_inTd) + fx(m_Ty) + fx(num));
    const float num2 = f32store((0.0 - fx(L)) / fx(Solver::deltaTime()));
    const float num3 = f32store(fx(f32store(fx(V.y) / fx(radius) - fx(w))) * fx(I) / fx(Solver::deltaTime()));
    const float num4 = MathUtility::FastAbs(num2);
    const float num5 = MathUtility::FastAbs(num3);
    if (!(num4 < num5) && MathUtility::FastAbs(f32store(fx(num5) - fx(num4))) >= 1E-06f) {
        tsum = SolveSymmetricConstraint(tsum, m_Ty, num3);
        tsum = SolveSymmetricConstraint(tsum, num, num2);
    } else {
        tsum = SolveSymmetricConstraint(tsum, num, num2);
        tsum = SolveSymmetricConstraint(tsum, m_Ty, num3);
    }
    T = tsum;
    Tr = f32store(fx(T) - fx(m_inTd));
    float num6 = f32store(fx(Mathf::Clamp(f32store(fx(Fext.y) * fx(radius) - fx(m_inTd)), f32store(0.0 - fx(m_inTb)), m_inTb)) + fx(m_inTd));
    float num9;
    if (num3 <= 0.0f) {
        const float num7 = Mathf::Max(m_Ty, num3);
        if (num6 < 0.0f) {
            const float num8 = f32store(fx(m_Ty) * fx(Mathf::InverseLerp(f32store(fx(m_adherentSlip.y) * fx(Mathf::Sign(m_forwardSlip))), 0.0f, m_forwardSlip)));
            if (num6 < num8) num6 = num8;
            num9 = f32store(fx(num6) - fx(num7));
        } else {
            num9 = Mathf::Clamp(num6, f32store(0.0 - fx(num7)), f32store(0.0 - fx(m_Ty)));
        }
    } else {
        const float num7 = Mathf::Min(m_Ty, num3);
        if (num6 > 0.0f) {
            const float num10 = f32store(fx(m_Ty) * fx(Mathf::InverseLerp(f32store(fx(m_adherentSlip.y) * fx(Mathf::Sign(m_forwardSlip))), 0.0f, m_forwardSlip)));
            if (num6 > num10) num6 = num10;
            num9 = f32store(fx(num6) - fx(num7));
        } else {
            num9 = Mathf::Clamp(num6, f32store(0.0 - fx(m_Ty)), f32store(0.0 - fx(num7)));
        }
    }
    F.y = f32store(fx(num9) / fx(radius));
    float x;
    if (m_isAdherent) {
        m_adherentSlip.y = f32store(fx(m_adherentSlip.y) * fx(F.y) / fx(m_adherentForce.y));
        m_contact.slip = m_adherentSlip;
        m_adherentSlip.x = tireFriction->GetAdherentSlipSideways(m_contact);
        m_contact.slip = m_adherentSlip;
        m_adherentForce = tireFriction->GetAdherentForce(m_contact);
        x = m_adherentForce.x;
    } else {
        x = m_tireForce.x;
    }
    x = MathUtility::FastAbs(x);
    F.x = Mathf::Clamp(Fext.x, f32store(0.0 - fx(x)), x);
    isResting = isResting && (x > MathUtility::FastAbs(Fext.x));
    isResting = isResting && (m_inTb > MathUtility::FastAbs(f32store(fx(Fext.y) * fx(radius) - fx(m_inTd))));
}

// VehiclePhysics/Wheel.cs:244
void Wheel::GetSubstepDerivative(Derivative& D) {
    D.T = T;
    D.F = F;
    D.Tr = Tr;
}

// VehiclePhysics/Wheel.cs:251
void Wheel::SetState(const State& S) {
    L = S.L;
    F = S.P / Solver::deltaTime();
    Tr = f32store(fx(S.Lr) / fx(Solver::deltaTime()));
    RecalculateVars();
    m_brakeTorque = 0.0f;
}

// VehiclePhysics/Wheel.cs:260
float Wheel::SolveSymmetricConstraint(float Tsum, float T, float Tmax) {
    const bool flag = Tsum >= 0.0f;
    const bool flag2 = Tmax >= 0.0f;
    const float num = flag ? Tsum : f32store(0.0 - fx(Tsum));
    const float num2 = flag2 ? Tmax : f32store(0.0 - fx(Tmax));
    if (Tmax == 0.0f || (flag == flag2 && num > num2)) {
        const float t = Mathf::InverseLerp(0.0f, f32store(fx(Tsum) * 0.5), T);
        Tsum = MathUtility::UnclampedLerp(Tsum, Tmax, t);
    }
    return Tsum;
}

// ---- DirectDrive -----------------------------------------------------------------------------------------------------------------------
DirectDrive::DirectDrive() { DirectDrive::Initialize(); }

// VehiclePhysics/DirectDrive.cs:18
float DirectDrive::angularVelocity() const { return f32store(fx(m_output->L) / fx(m_output->I)); }

// VehiclePhysics/DirectDrive.cs:20
float DirectDrive::torque() const { return m_output->outTd; }

// VehiclePhysics/DirectDrive.cs:22
void DirectDrive::Initialize() {
    SetInputs(0);
    SetOutputs(1);
}

// VehiclePhysics/DirectDrive.cs:28
bool DirectDrive::CheckConnections() {
    m_outputHold = outputs()[0];
    m_output = m_outputHold.get();
    return m_output != nullptr;
}

// VehiclePhysics/DirectDrive.cs:34
void DirectDrive::EvaluateTorqueDownstream() {
    const float value = f32store(fx(motorInput) * fx(maxMotorTorque));
    const float num = f32store(fx(m_output->I) * fx(MathUtility::FastAbs(maxRpm)) * fx(Block::RpmToW));
    const float max = f32store((fx(num) - fx(m_output->L)) / fx(Solver::deltaTime()) - fx(m_output->Tr) * fx(damping));
    const float min = f32store((0.0 - fx(num) - fx(m_output->L)) / fx(Solver::deltaTime()) - fx(m_output->Tr) * fx(damping));
    m_output->outTd = Mathf::Clamp(value, min, max);
}

// ---- Solver ----------------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/Solver.cs:62
bool Solver::Initialize(const std::vector<Wheel*>& wheels, bool enableRK4) {
    std::vector<Block*> list;
    bool flag = true;
    m_resultMessage = "Initializing | ";
    for (Wheel* item : wheels) list.push_back(item);
    m_wheelCount = static_cast<int>(wheels.size());
    size_t num = 0;
    while (num < list.size()) {
        const size_t count = list.size();
        for (size_t j = num; j < count; j++) {
            for (const auto& connection : list[j]->inputs()) {
                if (connection != nullptr && std::find(list.begin(), list.end(), connection->output) == list.end()) {
                    Block* output = connection->output;
                    list.push_back(output);
                    const bool flag2 = output->CheckConnections();
                    flag = flag && flag2;
                    if (!flag2) m_resultMessage += "Failed: " + std::string(typeid(*output).name()) + "@" + std::to_string(j) + " | ";
                }
            }
        }
        num = count;
    }
    if (!flag) {
        list.clear();
        for (Wheel* item : wheels) list.push_back(item);
    }
    m_blocks = list;
    if (enableRK4) {
        m_states.assign(2, StateVector());
        m_derivatives.assign(5, DerivativeVector());
    } else {
        m_states.assign(1, StateVector());
        m_derivatives.assign(1, DerivativeVector());
    }
    m_RK4enabled = enableRK4;
    for (auto& s : m_states) s.assign(m_blocks.size(), Block::State());
    for (auto& d : m_derivatives) d.assign(m_blocks.size(), Block::Derivative());
    m_resultMessage += flag ? "Ok." : "Aborted.";
    return flag;
}

// VehiclePhysics/Solver.cs:132
void Solver::Integrate(float t, float dt, int steps, bool useRK4) {
    if (m_RK4enabled && useRK4) {
        IntegrateRK4(m_states[0], m_states[1], m_derivatives[0], m_derivatives[1], m_derivatives[2], m_derivatives[3], m_derivatives[4], t, dt);
    } else {
        IntegrateEuler(m_states[0], m_derivatives[0], t, dt, steps);
    }
}

// VehiclePhysics/Solver.cs:154
void Solver::EulerStep(const Block::State& In, const Block::Derivative& Der, Block::State& Out) {
    Out.L = f32store(fx(In.L) + fx(Der.T) * fx(s_deltaTime));
    Out.P = In.P + Der.F * s_deltaTime;
    Out.Lr = f32store(fx(In.Lr) + fx(Der.Tr) * fx(s_deltaTime));
}

// VehiclePhysics/Solver.cs:161
void Solver::EulerStepL(const Block::State& In, const Block::Derivative& Der, Block::State& Out) {
    Out.L = f32store(fx(In.L) + fx(Der.T) * fx(s_deltaTime));
    Out.Lr = f32store(fx(In.Lr) + fx(Der.Tr) * fx(s_deltaTime));
}

// VehiclePhysics/Solver.cs:167
void Solver::EulerStep(const StateVector& In, const DerivativeVector& Der, StateVector& Out) {
    for (int i = 0; i < m_wheelCount; i++) EulerStep(In[static_cast<size_t>(i)], Der[static_cast<size_t>(i)], Out[static_cast<size_t>(i)]);
    for (size_t j = static_cast<size_t>(m_wheelCount); j < In.size(); j++) EulerStepL(In[j], Der[j], Out[j]);
}

// VehiclePhysics/Solver.cs:181
void Solver::IntegrateEuler(StateVector& S, DerivativeVector& D, float t, float dt, int subSteps) {
    s_time = t;
    s_deltaTime = dt;
    for (size_t i = 0; i < m_blocks.size(); i++) {
        m_blocks[i]->PreStep();
        m_blocks[i]->GetState(S[i]);
    }
    const float num2 = f32store(fx(dt) / static_cast<double>(static_cast<float>(subSteps)));
    for (int j = 0; j < subSteps; j++) {
        s_time = t;
        s_deltaTime = num2;
        ComputeDerivative(S, D);
        EulerStep(S, D, S);
        t = f32store(fx(t) + fx(num2));
    }
    s_time = f32store(fx(t) + fx(dt));
    s_deltaTime = dt;
    for (size_t k = 0; k < m_blocks.size(); k++) m_blocks[k]->SetState(S[k]);
}

// VehiclePhysics/Solver.cs:209
void Solver::EvaluateRK4(StateVector& S, StateVector& Stemp, DerivativeVector& DIn, float t, float dt, DerivativeVector& DOut) {
    s_time = t;
    s_deltaTime = dt;
    EulerStep(S, DIn, Stemp);
    s_time = f32store(fx(t) + fx(dt));
    ComputeDerivative(Stemp, DOut);
}

// VehiclePhysics/Solver.cs:218
void Solver::ComputeRK4Derivative(const Block::Derivative& A, const Block::Derivative& B, const Block::Derivative& C, const Block::Derivative& D, Block::Derivative& DOut) {
    const double sixth = fx(f32store(1.0 / 6.0));  // 1f / 6f is a compile-time float constant
    DOut.T = f32store(sixth * (fx(A.T) + 2.0 * (fx(B.T) + fx(C.T)) + fx(D.T)));
    DOut.F = f32store(sixth) * (A.F + 2.0f * (B.F + C.F) + D.F);
    DOut.Tr = f32store(sixth * (fx(A.Tr) + 2.0 * (fx(B.Tr) + fx(C.Tr)) + fx(D.Tr)));
}

// VehiclePhysics/Solver.cs:225
void Solver::ComputeRK4DerivativeT(const Block::Derivative& A, const Block::Derivative& B, const Block::Derivative& C, const Block::Derivative& D, Block::Derivative& DOut) {
    const double sixth = fx(f32store(1.0 / 6.0));
    DOut.T = f32store(sixth * (fx(A.T) + 2.0 * (fx(B.T) + fx(C.T)) + fx(D.T)));
    DOut.Tr = f32store(sixth * (fx(A.Tr) + 2.0 * (fx(B.Tr) + fx(C.Tr)) + fx(D.Tr)));
}

// VehiclePhysics/Solver.cs:231
void Solver::ComputeRK4Derivative(const DerivativeVector& A, const DerivativeVector& B, const DerivativeVector& C, const DerivativeVector& D, DerivativeVector& DOut) {
    for (size_t i = 0; i < static_cast<size_t>(m_wheelCount); i++) ComputeRK4Derivative(A[i], B[i], C[i], D[i], DOut[i]);
    for (size_t j = static_cast<size_t>(m_wheelCount); j < A.size(); j++) ComputeRK4DerivativeT(A[j], B[j], C[j], D[j], DOut[j]);
}

// VehiclePhysics/Solver.cs:245
void Solver::IntegrateRK4(StateVector& S, StateVector& Stemp, DerivativeVector& D, DerivativeVector& Da, DerivativeVector& Db, DerivativeVector& Dc, DerivativeVector& Dd, float t, float dt) {
    s_time = t;
    s_deltaTime = dt;
    for (size_t i = 0; i < m_blocks.size(); i++) {
        m_blocks[i]->PreStep();
        m_blocks[i]->GetState(S[i]);
    }
    ComputeDerivative(S, Da);
    EvaluateRK4(S, Stemp, Da, t, f32store(fx(dt) * 0.5), Db);
    EvaluateRK4(S, Stemp, Db, t, f32store(fx(dt) * 0.5), Dc);
    EvaluateRK4(S, Stemp, Dc, t, dt, Dd);
    s_time = t;
    s_deltaTime = dt;
    ComputeRK4Derivative(Da, Db, Dc, Dd, D);
    EulerStep(S, D, S);
    s_time = f32store(fx(t) + fx(dt));
    for (size_t j = 0; j < m_blocks.size(); j++) m_blocks[j]->SetState(S[j]);
}

// VehiclePhysics/Solver.cs:271
void Solver::ComputeDerivative(StateVector& S, DerivativeVector& Out) {
    for (size_t i = 0; i < m_blocks.size(); i++) {
        m_blocks[i]->SetSubstepState(S[i]);
        m_blocks[i]->ComputeStateUpstream();
    }
    for (int num2 = static_cast<int>(m_blocks.size()) - 1; num2 >= 0; num2--) {
        m_blocks[static_cast<size_t>(num2)]->EvaluateTorqueDownstream();
        m_blocks[static_cast<size_t>(num2)]->GetSubstepDerivative(Out[static_cast<size_t>(num2)]);
    }
}

}  // namespace ksp::VehiclePhysics
