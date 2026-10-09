using System;
using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.Networking;

[NativeConditional("ENABLE_NETWORK && ENABLE_UNET", true)]
[NativeHeader("Modules/UNET/UNETConfiguration.h")]
internal class HostTopologyInternal : IDisposable
{
	public IntPtr m_Ptr;

	[NativeProperty("m_ReceivedMessagePoolSize", TargetType.Field)]
	private extern ushort ReceivedMessagePoolSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_SentMessagePoolSize", TargetType.Field)]
	private extern ushort SentMessagePoolSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_MessagePoolSizeGrowthFactor", TargetType.Field)]
	private extern float MessagePoolSizeGrowthFactor
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public HostTopologyInternal(HostTopology topology)
	{
		ConnectionConfigInternal config = new ConnectionConfigInternal(topology.DefaultConfig);
		m_Ptr = InternalCreate(config, topology.MaxDefaultConnections);
		for (int i = 1; i <= topology.SpecialConnectionConfigsCount; i++)
		{
			ConnectionConfig specialConnectionConfig = topology.GetSpecialConnectionConfig(i);
			ConnectionConfigInternal config2 = new ConnectionConfigInternal(specialConnectionConfig);
			AddSpecialConnectionConfig(config2);
		}
		ReceivedMessagePoolSize = topology.ReceivedMessagePoolSize;
		SentMessagePoolSize = topology.SentMessagePoolSize;
		MessagePoolSizeGrowthFactor = topology.MessagePoolSizeGrowthFactor;
	}

	protected virtual void Dispose(bool disposing)
	{
		if (m_Ptr != IntPtr.Zero)
		{
			InternalDestroy(m_Ptr);
			m_Ptr = IntPtr.Zero;
		}
	}

	~HostTopologyInternal()
	{
		Dispose(disposing: false);
	}

	public void Dispose()
	{
		if (m_Ptr != IntPtr.Zero)
		{
			InternalDestroy(m_Ptr);
			m_Ptr = IntPtr.Zero;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr InternalCreate(ConnectionConfigInternal config, int maxDefaultConnections);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private static extern void InternalDestroy(IntPtr ptr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern ushort AddSpecialConnectionConfig(ConnectionConfigInternal config);
}
