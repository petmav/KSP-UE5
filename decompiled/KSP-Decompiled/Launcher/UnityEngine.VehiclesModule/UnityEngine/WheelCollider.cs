using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine;

[NativeHeader("Modules/Vehicles/WheelCollider.h")]
[NativeHeader("PhysicsScriptingClasses.h")]
public class WheelCollider : Collider
{
	public Vector3 center
	{
		get
		{
			get_center_Injected(out var ret);
			return ret;
		}
		set
		{
			set_center_Injected(ref value);
		}
	}

	public extern float radius
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float suspensionDistance
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public JointSpring suspensionSpring
	{
		get
		{
			get_suspensionSpring_Injected(out var ret);
			return ret;
		}
		set
		{
			set_suspensionSpring_Injected(ref value);
		}
	}

	public extern float forceAppPointDistance
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float mass
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float wheelDampingRate
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public WheelFrictionCurve forwardFriction
	{
		get
		{
			get_forwardFriction_Injected(out var ret);
			return ret;
		}
		set
		{
			set_forwardFriction_Injected(ref value);
		}
	}

	public WheelFrictionCurve sidewaysFriction
	{
		get
		{
			get_sidewaysFriction_Injected(out var ret);
			return ret;
		}
		set
		{
			set_sidewaysFriction_Injected(ref value);
		}
	}

	public extern float motorTorque
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float brakeTorque
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern float steerAngle
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public extern bool isGrounded
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("IsGrounded")]
		get;
	}

	public extern float rpm
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	public extern float sprungMass
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void ResetSprungMasses();

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void ConfigureVehicleSubsteps(float speedThreshold, int stepsBelowThreshold, int stepsAboveThreshold);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void GetWorldPose(out Vector3 pos, out Quaternion quat);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern bool GetGroundHit(out WheelHit hit);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_center_Injected(out Vector3 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_center_Injected(ref Vector3 value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_suspensionSpring_Injected(out JointSpring ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_suspensionSpring_Injected(ref JointSpring value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_forwardFriction_Injected(out WheelFrictionCurve ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_forwardFriction_Injected(ref WheelFrictionCurve value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void get_sidewaysFriction_Injected(out WheelFrictionCurve ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private extern void set_sidewaysFriction_Injected(ref WheelFrictionCurve value);
}
