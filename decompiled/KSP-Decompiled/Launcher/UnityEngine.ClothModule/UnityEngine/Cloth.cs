using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine.Bindings;
using UnityEngine.Internal;

namespace UnityEngine;

[NativeClass("Unity::Cloth")]
[NativeHeader("Modules/Cloth/Cloth.h")]
[RequireComponent(typeof(Transform), typeof(SkinnedMeshRenderer))]
public sealed class Cloth : Component
{
	public extern Vector3[] vertices
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetPositions")]
		get;
	}

	public extern Vector3[] normals
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetNormals")]
		get;
	}

	public extern ClothSkinningCoefficient[] coefficients
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetCoefficients")]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("SetCoefficients")]
		set;
	}

	public extern CapsuleCollider[] capsuleColliders
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetCapsuleColliders")]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("SetCapsuleColliders")]
		set;
	}

	public extern ClothSphereColliderPair[] sphereColliders
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetSphereColliders")]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("SetSphereColliders")]
		set;
	}

	public extern float sleepThreshold
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float bendingStiffness
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float stretchingStiffness
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float damping
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public Vector3 externalAcceleration
	{
		get
		{
			get_externalAcceleration_Injected(out var ret);
			return ret;
		}
		set
		{
			set_externalAcceleration_Injected(ref value);
		}
	}

	public Vector3 randomAcceleration
	{
		get
		{
			get_randomAcceleration_Injected(out var ret);
			return ret;
		}
		set
		{
			set_randomAcceleration_Injected(ref value);
		}
	}

	public extern bool useGravity
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern bool enabled
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float friction
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float collisionMassScale
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern bool enableContinuousCollision
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float useVirtualParticles
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float worldVelocityScale
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float worldAccelerationScale
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float clothSolverFrequency
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[Obsolete("Parameter solverFrequency is obsolete and no longer supported. Please use clothSolverFrequency instead.")]
	public bool solverFrequency
	{
		get
		{
			return clothSolverFrequency > 0f;
		}
		set
		{
			clothSolverFrequency = (value ? 120f : 0f);
		}
	}

	public extern bool useTethers
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float stiffnessFrequency
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float selfCollisionDistance
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float selfCollisionStiffness
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[Obsolete("useContinuousCollision is no longer supported, use enableContinuousCollision instead")]
	public float useContinuousCollision { get; set; }

	[Obsolete("Deprecated.Cloth.selfCollisions is no longer supported since Unity 5.0.", true)]
	public bool selfCollision { get; }

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void ClearTransformMotion();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern uint[] GetSelfAndInterCollisionIndices();

	internal void Internal_GetSelfAndInterCollisionIndices(List<uint> indicesOutList)
	{
		uint[] selfAndInterCollisionIndices = GetSelfAndInterCollisionIndices();
		indicesOutList.Clear();
		indicesOutList.AddRange(selfAndInterCollisionIndices);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetSelfAndInterCollisionIndices(uint[] indicesIn);

	internal void Internal_SetSelfAndInterCollisionIndices(List<uint> indicesInList)
	{
		SetSelfAndInterCollisionIndices(indicesInList.ToArray());
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern uint[] GetVirtualParticleIndices();

	internal void Internal_GetVirtualParticleIndices(List<uint> indicesOutList)
	{
		uint[] virtualParticleIndices = GetVirtualParticleIndices();
		indicesOutList.Clear();
		indicesOutList.AddRange(virtualParticleIndices);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetVirtualParticleIndices(uint[] indicesIn);

	internal void Internal_SetVirtualParticleIndices(List<uint> indicesInList)
	{
		SetVirtualParticleIndices(indicesInList.ToArray());
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern Vector3[] GetVirtualParticleWeights();

	internal void Internal_GetVirtualParticleWeights(List<Vector3> weightsOutList)
	{
		Vector3[] virtualParticleWeights = GetVirtualParticleWeights();
		weightsOutList.Clear();
		weightsOutList.AddRange(virtualParticleWeights);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void SetVirtualParticleWeights(Vector3[] weightsIn);

	internal void Internal_SetVirtualParticleWeights(List<Vector3> weightsInList)
	{
		SetVirtualParticleWeights(weightsInList.ToArray());
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void SetEnabledFading(bool enabled, float interpolationTime);

	[ExcludeFromDocs]
	public void SetEnabledFading(bool enabled)
	{
		SetEnabledFading(enabled, 0.5f);
	}

	private RaycastHit Raycast(Ray ray, float maxDistance, ref bool hasHit)
	{
		Raycast_Injected(ref ray, maxDistance, ref hasHit, out var ret);
		return ret;
	}

	internal bool Raycast(Ray ray, out RaycastHit hitInfo, float maxDistance)
	{
		bool hasHit = false;
		hitInfo = Raycast(ray, maxDistance, ref hasHit);
		return hasHit;
	}

	public void GetVirtualParticleIndices(List<uint> indices)
	{
		if (indices == null)
		{
			throw new ArgumentNullException("indices");
		}
		Internal_GetVirtualParticleIndices(indices);
	}

	public void SetVirtualParticleIndices(List<uint> indices)
	{
		if (indices == null)
		{
			throw new ArgumentNullException("indices");
		}
		Internal_SetVirtualParticleIndices(indices);
	}

	public void GetVirtualParticleWeights(List<Vector3> weights)
	{
		if (weights == null)
		{
			throw new ArgumentNullException("weights");
		}
		Internal_GetVirtualParticleWeights(weights);
	}

	public void SetVirtualParticleWeights(List<Vector3> weights)
	{
		if (weights == null)
		{
			throw new ArgumentNullException("weights");
		}
		Internal_SetVirtualParticleWeights(weights);
	}

	public void GetSelfAndInterCollisionIndices(List<uint> indices)
	{
		if (indices == null)
		{
			throw new ArgumentNullException("indices");
		}
		Internal_GetSelfAndInterCollisionIndices(indices);
	}

	public void SetSelfAndInterCollisionIndices(List<uint> indices)
	{
		if (indices == null)
		{
			throw new ArgumentNullException("indices");
		}
		Internal_SetSelfAndInterCollisionIndices(indices);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_externalAcceleration_Injected(out Vector3 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_externalAcceleration_Injected(ref Vector3 value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_randomAcceleration_Injected(out Vector3 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_randomAcceleration_Injected(ref Vector3 value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void Raycast_Injected(ref Ray ray, float maxDistance, ref bool hasHit, out RaycastHit ret);
}
