#include "Spike/SpikeMath.h"

namespace KspSpike
{
FVector3d CamSpaceExact(const FVector3d& V, const FVector3d& C, const FQuat4d& CamRot)
{
	return CamRot.UnrotateVector(V - C);
}

FVector3d CamSpaceF32World(const FVector3d& V, const FVector3d& C, const FVector3d& O, const FQuat4d& CamRot)
{
	const FVector3f Vf(V - O); // world-space vertex as float
	const FVector3f Cf(C - O); // camera position as float
	const FVector3f D = Vf - Cf;
	const FQuat4f Rf(CamRot);
	return FVector3d(Rf.UnrotateVector(D));
}

FVector3d CamSpaceLwc(const FVector3d& V, const FVector3d& P, const FVector3d& C, const FQuat4d& CamRot)
{
	// Mirrors the UE 5.8 fast-math DF path (Shaders/Private/DoubleFloat.ush DFTransformLocalToTranslatedWorld and
	// DoubleFloatOperations.ush DFFastLocalSubtractDemote; CPU split in Core/Public/Math/DoubleFloat.h and
	// PrimitiveUniformShaderParametersBuilder.h: PositionHigh = float(P), LocalToRelativeWorld.T = float(P - High)).
	const FVector3f L(V - P);                  // vertex buffer (ProceduralMesh stores positions relative to the component)
	const FVector3f PH(P);                     // Primitive.PositionHigh
	const FVector3f PL(P - FVector3d(PH));     // LocalToRelativeWorld translation
	const FVector3f CH(C);                     // -PreViewTranslation, DF high
	const FVector3f CL(C - FVector3d(CH));     // -PreViewTranslation, DF low
	const FVector3f High = PH - CH;            // exact only if PH and CH are within 2x of each other (Sterbenz)
	const FVector3f Sum = High + (FVector3f::ZeroVector - CL);
	const FVector3f OriginTW = Sum + PL;
	const FVector3f TW = L + OriginTW;         // identity primitive rotation in this spike
	const FQuat4f Rf(CamRot);
	return FVector3d(Rf.UnrotateVector(TW));
}

double FJitterTracker::Add(const FVector3d& Emulated, const FVector3d& Exact)
{
	const FVector3d Err = Emulated - Exact;
	MaxAbsErr = FMath::Max(MaxAbsErr, Err.Length());
	double J = 0.0;
	if (bHasPrev)
	{
		J = (Err - PrevErr).Length();
		MaxJitter = FMath::Max(MaxJitter, J);
		SumSqJitter += J * J;
		++Count;
	}
	PrevErr = Err;
	bHasPrev = true;
	return J;
}

void FaceBasis(int32 Face, FVector3d& OutN, FVector3d& OutT1, FVector3d& OutT2)
{
	switch (Face)
	{
	case 0: OutN = FVector3d(1, 0, 0);  OutT1 = FVector3d(0, 1, 0); OutT2 = FVector3d(0, 0, 1); break;
	case 1: OutN = FVector3d(-1, 0, 0); OutT1 = FVector3d(0, 0, 1); OutT2 = FVector3d(0, 1, 0); break;
	case 2: OutN = FVector3d(0, 1, 0);  OutT1 = FVector3d(0, 0, 1); OutT2 = FVector3d(1, 0, 0); break;
	case 3: OutN = FVector3d(0, -1, 0); OutT1 = FVector3d(1, 0, 0); OutT2 = FVector3d(0, 0, 1); break;
	case 4: OutN = FVector3d(0, 0, 1);  OutT1 = FVector3d(1, 0, 0); OutT2 = FVector3d(0, 1, 0); break;
	default: OutN = FVector3d(0, 0, -1); OutT1 = FVector3d(0, 1, 0); OutT2 = FVector3d(1, 0, 0); break;
	}
}

FVector3d CubeToSphere(const FVector3d& C)
{
	const double X2 = C.X * C.X, Y2 = C.Y * C.Y, Z2 = C.Z * C.Z;
	const FVector3d S(
		C.X * FMath::Sqrt(FMath::Max(0.0, 1.0 - Y2 * 0.5 - Z2 * 0.5 + Y2 * Z2 / 3.0)),
		C.Y * FMath::Sqrt(FMath::Max(0.0, 1.0 - Z2 * 0.5 - X2 * 0.5 + Z2 * X2 / 3.0)),
		C.Z * FMath::Sqrt(FMath::Max(0.0, 1.0 - X2 * 0.5 - Y2 * 0.5 + X2 * Y2 / 3.0)));
	return S.GetSafeNormal();
}

FVector3d FaceUVToDir(int32 Face, double U, double V)
{
	FVector3d N, T1, T2;
	FaceBasis(Face, N, T1, T2);
	return CubeToSphere(N + T1 * U + T2 * V);
}

double PatchEdgeLength(int32 Level, double Radius)
{
	return Radius * (UE_DOUBLE_PI * 0.5) / FMath::Pow(2.0, static_cast<double>(Level));
}

FVector3d PatchCentre(int32 Face, int32 Level, int32 IX, int32 IY, double Radius)
{
	const double Size = 2.0 / FMath::Pow(2.0, static_cast<double>(Level));
	return FaceUVToDir(Face, -1.0 + Size * (IX + 0.5), -1.0 + Size * (IY + 0.5)) * Radius;
}

void AddTriFacing(TArray<int32>& T, const TArray<FVector>& V, int32 A, int32 B, int32 C, const FVector3d& Outward, bool bFlip)
{
	// UE front face: cross(B-A, C-A) points AWAY from the viewer (see GenerateBoxMesh: +Z face gives a -Z cross).
	const FVector3d Cr = FVector3d::CrossProduct(FVector3d(V[B]) - FVector3d(V[A]), FVector3d(V[C]) - FVector3d(V[A]));
	bool bSwap = FVector3d::DotProduct(Cr, Outward) > 0.0;
	if (bFlip)
	{
		bSwap = !bSwap;
	}
	T.Add(A);
	T.Add(bSwap ? C : B);
	T.Add(bSwap ? B : C);
}

FColor SurfaceColor(const FVector3d& Dir)
{
	const double LatDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Dir.Z, -1.0, 1.0)));
	if (FMath::Abs(LatDeg) > 72.0)
	{
		return FColor(215, 222, 235);
	}
	// Cheap smooth "continents"; a forced continent around (1,1,1) (the spike's test site) so the site is land.
	double Nz = FMath::Sin(3.1 * Dir.X + 1.7 * FMath::Sin(2.3 * Dir.Y)) + FMath::Sin(2.7 * Dir.Y + 1.3 * Dir.Z) * 0.8 + FMath::Sin(4.1 * Dir.Z - 0.7 * Dir.X) * 0.5;
	Nz += 2.0 * FMath::Exp(-(Dir - FVector3d(1, 1, 1).GetSafeNormal()).SizeSquared() / 0.05);
	if (Nz < 0.15)
	{
		return FColor(18, 40, 85); // ocean
	}
	// Land: dark green with a faint high-frequency tint so motion is visible in screenshots.
	const double Tint = 0.5 + 0.5 * FMath::Sin(60.0 * Dir.Y) * FMath::Sin(60.0 * Dir.Z);
	return FColor(static_cast<uint8>(40 + 20 * Tint), static_cast<uint8>(70 + 20 * Tint), static_cast<uint8>(30 + 10 * Tint));
}

void BuildPatch(int32 Face, int32 Level, int32 IX, int32 IY, int32 N, double Radius, bool bPlanetCentred,
	bool bFlipWinding, EPatchColor ColorMode, const FColor& FlatColor, FPatchMesh& Out)
{
	const double Size = 2.0 / FMath::Pow(2.0, static_cast<double>(Level));
	const double U0 = -1.0 + Size * IX;
	const double V0 = -1.0 + Size * IY;
	const FVector3d CentreDir = FaceUVToDir(Face, U0 + Size * 0.5, V0 + Size * 0.5);
	Out.Origin = bPlanetCentred ? FVector3d::ZeroVector : CentreDir * Radius;
	Out.Verts.Reset();
	Out.Tris.Reset();
	Out.Normals.Reset();
	Out.Colors.Reset();

	const int32 S = N + 1;
	auto ColorFor = [&](const FVector3d& Dir) { return ColorMode == EPatchColor::Flat ? FlatColor : SurfaceColor(Dir); };

	TArray<FVector3d> Dirs;
	Dirs.Reserve(S * S);
	for (int32 J = 0; J <= N; ++J)
	{
		for (int32 I = 0; I <= N; ++I)
		{
			const FVector3d Dir = FaceUVToDir(Face, U0 + Size * I / N, V0 + Size * J / N);
			Dirs.Add(Dir);
			Out.Verts.Add(FVector(Dir * Radius - Out.Origin));
			Out.Normals.Add(FVector(Dir));
			Out.Colors.Add(ColorFor(Dir));
		}
	}
	auto Idx = [S](int32 I, int32 J) { return J * S + I; };
	for (int32 J = 0; J < N; ++J)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const int32 A = Idx(I, J), B = Idx(I + 1, J), C = Idx(I + 1, J + 1), D = Idx(I, J + 1);
			AddTriFacing(Out.Tris, Out.Verts, A, B, C, Dirs[A], bFlipWinding);
			AddTriFacing(Out.Tris, Out.Verts, A, C, D, Dirs[A], bFlipWinding);
		}
	}

	// Skirts hide T-junction cracks between LOD levels. Emitted double-sided (cheap: 4*N*4 tris).
	const double SkirtDepth = FMath::Max(PatchEdgeLength(Level, Radius) * 0.03, 50.0);
	auto AddSkirtEdge = [&](TFunctionRef<int32(int32)> EdgeIndex)
	{
		const int32 First = Out.Verts.Num();
		for (int32 K = 0; K <= N; ++K)
		{
			const FVector3d& Dir = Dirs[EdgeIndex(K)];
			Out.Verts.Add(FVector(Dir * (Radius - SkirtDepth) - Out.Origin));
			Out.Normals.Add(FVector(Dir));
			Out.Colors.Add(ColorFor(Dir));
		}
		for (int32 K = 0; K < N; ++K)
		{
			const int32 E0 = EdgeIndex(K), E1 = EdgeIndex(K + 1), S0 = First + K, S1 = First + K + 1;
			Out.Tris.Append({ E0, E1, S0, S0, E1, S1, E0, S0, E1, S0, S1, E1 });
		}
	};
	AddSkirtEdge([&](int32 K) { return Idx(K, 0); });
	AddSkirtEdge([&](int32 K) { return Idx(K, N); });
	AddSkirtEdge([&](int32 K) { return Idx(0, K); });
	AddSkirtEdge([&](int32 K) { return Idx(N, K); });
}

void BuildCube(const FVector3d& Centre, const FVector3d& Ax, const FVector3d& Ay, const FVector3d& Az, double H,
	const FColor& Color, TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FColor>& C)
{
	V.Reset(); T.Reset(); N.Reset(); C.Reset();
	const FVector3d Axes[3] = { Ax, Ay, Az };
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (int32 Sign = -1; Sign <= 1; Sign += 2)
		{
			const FVector3d Nrm = Axes[Axis] * Sign;
			const FVector3d U = Axes[(Axis + 1) % 3];
			const FVector3d W = Axes[(Axis + 2) % 3];
			const int32 Base = V.Num();
			const FVector3d FaceC = Centre + Nrm * H;
			V.Add(FVector(FaceC - U * H - W * H));
			V.Add(FVector(FaceC + U * H - W * H));
			V.Add(FVector(FaceC + U * H + W * H));
			V.Add(FVector(FaceC - U * H + W * H));
			for (int32 K = 0; K < 4; ++K)
			{
				N.Add(FVector(Nrm));
				C.Add(Color);
			}
			AddTriFacing(T, V, Base + 0, Base + 1, Base + 2, Nrm, false);
			AddTriFacing(T, V, Base + 0, Base + 2, Base + 3, Nrm, false);
		}
	}
}
} // namespace KspSpike
