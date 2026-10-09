using UnityEngine.Bindings;

namespace UnityEngine;

[NativeHeader("Modules/Vehicles/WheelCollider.h")]
public struct WheelHit
{
	[NativeName("point")]
	private Vector3 m_Point;

	[NativeName("normal")]
	private Vector3 m_Normal;

	[NativeName("forwardDir")]
	private Vector3 m_ForwardDir;

	[NativeName("sidewaysDir")]
	private Vector3 m_SidewaysDir;

	[NativeName("force")]
	private float m_Force;

	[NativeName("forwardSlip")]
	private float m_ForwardSlip;

	[NativeName("sidewaysSlip")]
	private float m_SidewaysSlip;

	[NativeName("collider")]
	private Collider m_Collider;

	public Collider collider
	{
		get
		{
			return m_Collider;
		}
		set
		{
			m_Collider = value;
		}
	}

	public Vector3 point
	{
		get
		{
			return m_Point;
		}
		set
		{
			m_Point = value;
		}
	}

	public Vector3 normal
	{
		get
		{
			return m_Normal;
		}
		set
		{
			m_Normal = value;
		}
	}

	public Vector3 forwardDir
	{
		get
		{
			return m_ForwardDir;
		}
		set
		{
			m_ForwardDir = value;
		}
	}

	public Vector3 sidewaysDir
	{
		get
		{
			return m_SidewaysDir;
		}
		set
		{
			m_SidewaysDir = value;
		}
	}

	public float force
	{
		get
		{
			return m_Force;
		}
		set
		{
			m_Force = value;
		}
	}

	public float forwardSlip
	{
		get
		{
			return m_ForwardSlip;
		}
		set
		{
			m_ForwardSlip = value;
		}
	}

	public float sidewaysSlip
	{
		get
		{
			return m_SidewaysSlip;
		}
		set
		{
			m_SidewaysSlip = value;
		}
	}
}
