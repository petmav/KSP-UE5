using System;
using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.Networking;

[NativeHeader("Modules/UNET/UNETConfiguration.h")]
[NativeConditional("ENABLE_NETWORK && ENABLE_UNET", true)]
internal class ConnectionSimulatorConfigInternal : IDisposable
{
	public IntPtr m_Ptr;

	public ConnectionSimulatorConfigInternal(ConnectionSimulatorConfig config)
	{
		m_Ptr = InternalCreate(config.m_OutMinDelay, config.m_OutAvgDelay, config.m_InMinDelay, config.m_InAvgDelay, config.m_PacketLossPercentage);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (m_Ptr != IntPtr.Zero)
		{
			InternalDestroy(m_Ptr);
			m_Ptr = IntPtr.Zero;
		}
		GC.SuppressFinalize(this);
	}

	~ConnectionSimulatorConfigInternal()
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
	private static extern IntPtr InternalCreate(int outMinDelay, int outAvgDelay, int inMinDelay, int inAvgDelay, float packetLossPercentage);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private static extern void InternalDestroy(IntPtr ptr);
}
