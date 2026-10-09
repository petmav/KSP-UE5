using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace UnityEngine.Networking;

[StructLayout(LayoutKind.Sequential)]
[NativeConditional("ENABLE_NETWORK && ENABLE_UNET", true)]
[NativeHeader("Modules/UNET/UNETConfiguration.h")]
[NativeHeader("Modules/UNET/UNetTypes.h")]
[NativeHeader("Modules/UNET/UNETManager.h")]
internal class ConnectionConfigInternal : IDisposable
{
	public IntPtr m_Ptr;

	[NativeProperty("m_ProtocolRequired.m_FragmentSize", TargetType.Field)]
	private extern ushort FragmentSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_ResendTimeout", TargetType.Field)]
	private extern uint ResendTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_DisconnectTimeout", TargetType.Field)]
	private extern uint DisconnectTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_ConnectTimeout", TargetType.Field)]
	private extern uint ConnectTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_MinUpdateTimeout", TargetType.Field)]
	private extern uint MinUpdateTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_PingTimeout", TargetType.Field)]
	private extern uint PingTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_ReducedPingTimeout", TargetType.Field)]
	private extern uint ReducedPingTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_AllCostTimeout", TargetType.Field)]
	private extern uint AllCostTimeout
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_NetworkDropThreshold", TargetType.Field)]
	private extern byte NetworkDropThreshold
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_OverflowDropThreshold", TargetType.Field)]
	private extern byte OverflowDropThreshold
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_MaxConnectionAttempt", TargetType.Field)]
	private extern byte MaxConnectionAttempt
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_AckDelay", TargetType.Field)]
	private extern uint AckDelay
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_SendDelay", TargetType.Field)]
	private extern uint SendDelay
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_MaxCombinedReliableMessageSize", TargetType.Field)]
	private extern ushort MaxCombinedReliableMessageSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_MaxCombinedReliableMessageAmount", TargetType.Field)]
	private extern ushort MaxCombinedReliableMessageCount
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_MaxSentMessageQueueSize", TargetType.Field)]
	private extern ushort MaxSentMessageQueueSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_AcksType", TargetType.Field)]
	private extern byte AcksType
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolRequired.m_UsePlatformSpecificProtocols", TargetType.Field)]
	private extern bool UsePlatformSpecificProtocols
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_InitialBandwidth", TargetType.Field)]
	private extern uint InitialBandwidth
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_BandwidthPeakFactor", TargetType.Field)]
	private extern float BandwidthPeakFactor
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_WebSocketReceiveBufferMaxSize", TargetType.Field)]
	private extern ushort WebSocketReceiveBufferMaxSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeProperty("m_ProtocolOptional.m_UdpSocketReceiveBufferMaxSize", TargetType.Field)]
	private extern uint UdpSocketReceiveBufferMaxSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	public ConnectionConfigInternal(ConnectionConfig config)
	{
		if (config == null)
		{
			throw new NullReferenceException("config is not defined");
		}
		m_Ptr = InternalCreate();
		if (!SetPacketSize(config.PacketSize))
		{
			throw new ArgumentOutOfRangeException("PacketSize is too small");
		}
		FragmentSize = config.FragmentSize;
		ResendTimeout = config.ResendTimeout;
		DisconnectTimeout = config.DisconnectTimeout;
		ConnectTimeout = config.ConnectTimeout;
		MinUpdateTimeout = config.MinUpdateTimeout;
		PingTimeout = config.PingTimeout;
		ReducedPingTimeout = config.ReducedPingTimeout;
		AllCostTimeout = config.AllCostTimeout;
		NetworkDropThreshold = config.NetworkDropThreshold;
		OverflowDropThreshold = config.OverflowDropThreshold;
		MaxConnectionAttempt = config.MaxConnectionAttempt;
		AckDelay = config.AckDelay;
		SendDelay = config.SendDelay;
		MaxCombinedReliableMessageSize = config.MaxCombinedReliableMessageSize;
		MaxCombinedReliableMessageCount = config.MaxCombinedReliableMessageCount;
		MaxSentMessageQueueSize = config.MaxSentMessageQueueSize;
		AcksType = (byte)config.AcksType;
		UsePlatformSpecificProtocols = config.UsePlatformSpecificProtocols;
		InitialBandwidth = config.InitialBandwidth;
		BandwidthPeakFactor = config.BandwidthPeakFactor;
		WebSocketReceiveBufferMaxSize = config.WebSocketReceiveBufferMaxSize;
		UdpSocketReceiveBufferMaxSize = config.UdpSocketReceiveBufferMaxSize;
		if (config.SSLCertFilePath != null)
		{
			int num = SetSSLCertFilePath(config.SSLCertFilePath);
			if (num != 0)
			{
				throw new ArgumentOutOfRangeException("SSLCertFilePath cannot be > than " + num);
			}
		}
		if (config.SSLPrivateKeyFilePath != null)
		{
			int num2 = SetSSLPrivateKeyFilePath(config.SSLPrivateKeyFilePath);
			if (num2 != 0)
			{
				throw new ArgumentOutOfRangeException("SSLPrivateKeyFilePath cannot be > than " + num2);
			}
		}
		if (config.SSLCAFilePath != null)
		{
			int num3 = SetSSLCAFilePath(config.SSLCAFilePath);
			if (num3 != 0)
			{
				throw new ArgumentOutOfRangeException("SSLCAFilePath cannot be > than " + num3);
			}
		}
		for (byte b = 0; b < config.ChannelCount; b++)
		{
			AddChannel((byte)config.GetChannel(b));
		}
		for (byte b2 = 0; b2 < config.SharedOrderChannelCount; b2++)
		{
			IList<byte> sharedOrderChannels = config.GetSharedOrderChannels(b2);
			byte[] array = new byte[sharedOrderChannels.Count];
			sharedOrderChannels.CopyTo(array, 0);
			MakeChannelsSharedOrder(array);
		}
	}

	protected virtual void Dispose(bool disposing)
	{
		if (m_Ptr != IntPtr.Zero)
		{
			InternalDestroy(m_Ptr);
			m_Ptr = IntPtr.Zero;
		}
	}

	~ConnectionConfigInternal()
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
	private static extern IntPtr InternalCreate();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private static extern void InternalDestroy(IntPtr ptr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern byte AddChannel(int value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern bool SetPacketSize(ushort value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("SetSSLCertFilePath")]
	public extern int SetSSLCertFilePath(string value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("SetSSLPrivateKeyFilePath")]
	public extern int SetSSLPrivateKeyFilePath(string value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("SetSSLCAFilePath")]
	public extern int SetSSLCAFilePath(string value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("MakeChannelsSharedOrder")]
	private extern bool MakeChannelsSharedOrder(byte[] values);
}
