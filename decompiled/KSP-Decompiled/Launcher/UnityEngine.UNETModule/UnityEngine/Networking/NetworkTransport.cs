using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;
using UnityEngine.Networking.Types;

namespace UnityEngine.Networking;

[NativeHeader("Modules/UNET/UNETManager.h")]
[NativeHeader("Modules/UNET/UNetTypes.h")]
[NativeHeader("Modules/UNET/UNETConfiguration.h")]
[NativeConditional("ENABLE_NETWORK && ENABLE_UNET", true)]
[Obsolete("The UNET transport will be removed in the future as soon a replacement is ready.")]
public sealed class NetworkTransport
{
	private static int s_nextSceneId = 1;

	public static bool IsStarted => IsStartedInternal();

	public static bool DoesEndPointUsePlatformProtocols(EndPoint endPoint)
	{
		if (endPoint.GetType().FullName == "UnityEngine.PS4.SceEndPoint")
		{
			SocketAddress socketAddress = endPoint.Serialize();
			if (socketAddress[8] != 0 || socketAddress[9] != 0)
			{
				return true;
			}
		}
		return false;
	}

	public static int ConnectEndPoint(int hostId, EndPoint endPoint, int exceptionConnectionId, out byte error)
	{
		error = 0;
		byte[] array = new byte[4] { 95, 36, 19, 246 };
		if (endPoint == null)
		{
			throw new NullReferenceException("Null EndPoint provided");
		}
		if (endPoint.GetType().FullName != "UnityEngine.XboxOne.XboxOneEndPoint" && endPoint.GetType().FullName != "UnityEngine.PS4.SceEndPoint" && endPoint.GetType().FullName != "UnityEngine.PSVita.SceEndPoint")
		{
			throw new ArgumentException("Endpoint of type XboxOneEndPoint or SceEndPoint  required");
		}
		if (endPoint.GetType().FullName == "UnityEngine.XboxOne.XboxOneEndPoint")
		{
			if (endPoint.AddressFamily != AddressFamily.InterNetworkV6)
			{
				throw new ArgumentException("XboxOneEndPoint has an invalid family");
			}
			SocketAddress socketAddress = endPoint.Serialize();
			if (socketAddress.Size != 14)
			{
				throw new ArgumentException("XboxOneEndPoint has an invalid size");
			}
			if (socketAddress[0] != 0 || socketAddress[1] != 0)
			{
				throw new ArgumentException("XboxOneEndPoint has an invalid family signature");
			}
			if (socketAddress[2] != array[0] || socketAddress[3] != array[1] || socketAddress[4] != array[2] || socketAddress[5] != array[3])
			{
				throw new ArgumentException("XboxOneEndPoint has an invalid signature");
			}
			byte[] array2 = new byte[8];
			for (int i = 0; i < array2.Length; i++)
			{
				array2[i] = socketAddress[6 + i];
			}
			IntPtr intPtr = new IntPtr(BitConverter.ToInt64(array2, 0));
			if (intPtr == IntPtr.Zero)
			{
				throw new ArgumentException("XboxOneEndPoint has an invalid SOCKET_STORAGE pointer");
			}
			byte[] array3 = new byte[2];
			Marshal.Copy(intPtr, array3, 0, array3.Length);
			AddressFamily addressFamily = (AddressFamily)((array3[1] << 8) + array3[0]);
			if (addressFamily != AddressFamily.InterNetworkV6)
			{
				throw new ArgumentException("XboxOneEndPoint has corrupt or invalid SOCKET_STORAGE pointer");
			}
			return Internal_ConnectEndPoint(hostId, array2, 128, exceptionConnectionId, out error);
		}
		SocketAddress socketAddress2 = endPoint.Serialize();
		if (socketAddress2.Size != 16)
		{
			throw new ArgumentException("EndPoint has an invalid size");
		}
		if (socketAddress2[0] != socketAddress2.Size)
		{
			throw new ArgumentException("EndPoint has an invalid size value");
		}
		if (socketAddress2[1] != 2)
		{
			throw new ArgumentException("EndPoint has an invalid family value");
		}
		byte[] array4 = new byte[16];
		for (int j = 0; j < array4.Length; j++)
		{
			array4[j] = socketAddress2[j];
		}
		return Internal_ConnectEndPoint(hostId, array4, 16, exceptionConnectionId, out error);
	}

	private NetworkTransport()
	{
	}

	public static void Init()
	{
		InitializeClass();
	}

	public static void Init(GlobalConfig config)
	{
		if (config.NetworkEventAvailable != null)
		{
			SetNetworkEventAvailableCallback(config.NetworkEventAvailable);
		}
		if (config.ConnectionReadyForSend != null)
		{
			SetConnectionReadyForSendCallback(config.ConnectionReadyForSend);
		}
		InitializeClassWithConfig(new GlobalConfigInternal(config));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::InitializeClass")]
	private static extern void InitializeClass();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::InitializeClassWithConfig")]
	private static extern void InitializeClassWithConfig(GlobalConfigInternal config);

	public static void Shutdown()
	{
		Cleanup();
	}

	[Obsolete("This function has been deprecated. Use AssetDatabase utilities instead.")]
	public static string GetAssetId(GameObject go)
	{
		return "";
	}

	public static void AddSceneId(int id)
	{
		if (id > s_nextSceneId)
		{
			s_nextSceneId = id + 1;
		}
	}

	public static int GetNextSceneId()
	{
		return s_nextSceneId++;
	}

	public static int AddHostWithSimulator(HostTopology topology, int minTimeout, int maxTimeout, int port, string ip)
	{
		if (topology == null)
		{
			throw new NullReferenceException("topology is not defined");
		}
		CheckTopology(topology);
		return AddHostInternal(new HostTopologyInternal(topology), ip, port, minTimeout, maxTimeout);
	}

	public static int AddHostWithSimulator(HostTopology topology, int minTimeout, int maxTimeout, int port)
	{
		return AddHostWithSimulator(topology, minTimeout, maxTimeout, port, null);
	}

	public static int AddHostWithSimulator(HostTopology topology, int minTimeout, int maxTimeout)
	{
		return AddHostWithSimulator(topology, minTimeout, maxTimeout, 0, null);
	}

	public static int AddHost(HostTopology topology, int port, string ip)
	{
		return AddHostWithSimulator(topology, 0, 0, port, ip);
	}

	public static int AddHost(HostTopology topology, int port)
	{
		return AddHost(topology, port, null);
	}

	public static int AddHost(HostTopology topology)
	{
		return AddHost(topology, 0, null);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->AddHost")]
	private static extern int AddHostInternal(HostTopologyInternal topologyInt, string ip, int port, int minTimeout, int maxTimeout);

	public static int AddWebsocketHost(HostTopology topology, int port, string ip)
	{
		if (port != 0 && IsPortOpen(ip, port))
		{
			throw new InvalidOperationException("Cannot open web socket on port " + port + " It has been already occupied.");
		}
		if (topology == null)
		{
			throw new NullReferenceException("topology is not defined");
		}
		CheckTopology(topology);
		return AddWsHostInternal(new HostTopologyInternal(topology), ip, port);
	}

	public static int AddWebsocketHost(HostTopology topology, int port)
	{
		return AddWebsocketHost(topology, port, null);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->AddWsHost")]
	private static extern int AddWsHostInternal(HostTopologyInternal topologyInt, string ip, int port);

	private static bool IsPortOpen(string ip, int port)
	{
		TimeSpan timeout = TimeSpan.FromMilliseconds(500.0);
		string host = ((ip == null) ? "127.0.0.1" : ip);
		try
		{
			using TcpClient tcpClient = new TcpClient();
			IAsyncResult asyncResult = tcpClient.BeginConnect(host, port, null, null);
			if (!asyncResult.AsyncWaitHandle.WaitOne(timeout))
			{
				return false;
			}
			tcpClient.EndConnect(asyncResult);
		}
		catch
		{
			return false;
		}
		return true;
	}

	public static void ConnectAsNetworkHost(int hostId, string address, int port, NetworkID network, SourceID source, NodeID node, out byte error)
	{
		ConnectAsNetworkHostInternal(hostId, address, port, (ulong)network, (ulong)source, (ushort)node, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->ConnectAsNetworkHost")]
	private static extern void ConnectAsNetworkHostInternal(int hostId, string address, int port, ulong network, ulong source, ushort node, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->DisconnectNetworkHost")]
	public static extern void DisconnectNetworkHost(int hostId, out byte error);

	public static NetworkEventType ReceiveRelayEventFromHost(int hostId, out byte error)
	{
		return (NetworkEventType)ReceiveRelayEventFromHostInternal(hostId, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->PopRelayHostData")]
	private static extern int ReceiveRelayEventFromHostInternal(int hostId, out byte error);

	public static int ConnectToNetworkPeer(int hostId, string address, int port, int exceptionConnectionId, int relaySlotId, NetworkID network, SourceID source, NodeID node, int bytesPerSec, float bucketSizeFactor, out byte error)
	{
		return ConnectToNetworkPeerInternal(hostId, address, port, exceptionConnectionId, relaySlotId, (ulong)network, (ulong)source, (ushort)node, bytesPerSec, bucketSizeFactor, out error);
	}

	public static int ConnectToNetworkPeer(int hostId, string address, int port, int exceptionConnectionId, int relaySlotId, NetworkID network, SourceID source, NodeID node, out byte error)
	{
		return ConnectToNetworkPeer(hostId, address, port, exceptionConnectionId, relaySlotId, network, source, node, 0, 0f, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->ConnectToNetworkPeer")]
	private static extern int ConnectToNetworkPeerInternal(int hostId, string address, int port, int exceptionConnectionId, int relaySlotId, ulong network, ulong source, ushort node, int bytesPerSec, float bucketSizeFactor, out byte error);

	[Obsolete("GetCurrentIncomingMessageAmount has been deprecated.")]
	public static int GetCurrentIncomingMessageAmount()
	{
		return 0;
	}

	[Obsolete("GetCurrentOutgoingMessageAmount has been deprecated.")]
	public static int GetCurrentOutgoingMessageAmount()
	{
		return 0;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetIncomingMessageQueueSize")]
	public static extern int GetIncomingMessageQueueSize(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingMessageQueueSize")]
	public static extern int GetOutgoingMessageQueueSize(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetCurrentRTT")]
	public static extern int GetCurrentRTT(int hostId, int connectionId, out byte error);

	[Obsolete("GetCurrentRtt() has been deprecated.")]
	public static int GetCurrentRtt(int hostId, int connectionId, out byte error)
	{
		return GetCurrentRTT(hostId, connectionId, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetIncomingPacketLossCount")]
	public static extern int GetIncomingPacketLossCount(int hostId, int connectionId, out byte error);

	[Obsolete("GetNetworkLostPacketNum() has been deprecated.")]
	public static int GetNetworkLostPacketNum(int hostId, int connectionId, out byte error)
	{
		return GetIncomingPacketLossCount(hostId, connectionId, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetIncomingPacketCount")]
	public static extern int GetIncomingPacketCount(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingPacketNetworkLossPercent")]
	public static extern int GetOutgoingPacketNetworkLossPercent(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingPacketOverflowLossPercent")]
	public static extern int GetOutgoingPacketOverflowLossPercent(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetMaxAllowedBandwidth")]
	public static extern int GetMaxAllowedBandwidth(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetAckBufferCount")]
	public static extern int GetAckBufferCount(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetIncomingPacketDropCountForAllHosts")]
	public static extern int GetIncomingPacketDropCountForAllHosts();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetIncomingPacketCountForAllHosts")]
	public static extern int GetIncomingPacketCountForAllHosts();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingPacketCount")]
	public static extern int GetOutgoingPacketCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingPacketCount")]
	public static extern int GetOutgoingPacketCountForHost(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingPacketCount")]
	public static extern int GetOutgoingPacketCountForConnection(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingMessageCount")]
	public static extern int GetOutgoingMessageCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingMessageCount")]
	public static extern int GetOutgoingMessageCountForHost(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingMessageCount")]
	public static extern int GetOutgoingMessageCountForConnection(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingUserBytesCount")]
	public static extern int GetOutgoingUserBytesCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingUserBytesCount")]
	public static extern int GetOutgoingUserBytesCountForHost(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingUserBytesCount")]
	public static extern int GetOutgoingUserBytesCountForConnection(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingSystemBytesCount")]
	public static extern int GetOutgoingSystemBytesCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingSystemBytesCount")]
	public static extern int GetOutgoingSystemBytesCountForHost(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingSystemBytesCount")]
	public static extern int GetOutgoingSystemBytesCountForConnection(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingFullBytesCount")]
	public static extern int GetOutgoingFullBytesCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingFullBytesCount")]
	public static extern int GetOutgoingFullBytesCountForHost(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetOutgoingFullBytesCount")]
	public static extern int GetOutgoingFullBytesCountForConnection(int hostId, int connectionId, out byte error);

	[Obsolete("GetPacketSentRate has been deprecated.")]
	public static int GetPacketSentRate(int hostId, int connectionId, out byte error)
	{
		error = 0;
		return 0;
	}

	[Obsolete("GetPacketReceivedRate has been deprecated.")]
	public static int GetPacketReceivedRate(int hostId, int connectionId, out byte error)
	{
		error = 0;
		return 0;
	}

	[Obsolete("GetRemotePacketReceivedRate has been deprecated.")]
	public static int GetRemotePacketReceivedRate(int hostId, int connectionId, out byte error)
	{
		error = 0;
		return 0;
	}

	[Obsolete("GetNetIOTimeuS has been deprecated.")]
	public static int GetNetIOTimeuS()
	{
		return 0;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetConnectionInfo")]
	public static extern string GetConnectionInfo(int hostId, int connectionId, out int port, out ulong network, out ushort dstNode, out byte error);

	public static void GetConnectionInfo(int hostId, int connectionId, out string address, out int port, out NetworkID network, out NodeID dstNode, out byte error)
	{
		address = GetConnectionInfo(hostId, connectionId, out port, out var network2, out var dstNode2, out error);
		network = (NetworkID)network2;
		dstNode = (NodeID)dstNode2;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetNetworkTimestamp")]
	public static extern int GetNetworkTimestamp();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetRemoteDelayTimeMS")]
	public static extern int GetRemoteDelayTimeMS(int hostId, int connectionId, int remoteTime, out byte error);

	public static bool StartSendMulticast(int hostId, int channelId, byte[] buffer, int size, out byte error)
	{
		return StartSendMulticastInternal(hostId, channelId, buffer, size, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->StartSendMulticast")]
	private static extern bool StartSendMulticastInternal(int hostId, int channelId, [Out] byte[] buffer, int size, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->SendMulticast")]
	public static extern bool SendMulticast(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->FinishSendMulticast")]
	public static extern bool FinishSendMulticast(int hostId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetMaxPacketSize")]
	private static extern int GetMaxPacketSize();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->RemoveHost")]
	public static extern bool RemoveHost(int hostId);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::IsStarted")]
	private static extern bool IsStartedInternal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->Connect")]
	public static extern int Connect(int hostId, string address, int port, int exeptionConnectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->ConnectWithSimulator")]
	private static extern int ConnectWithSimulatorInternal(int hostId, string address, int port, int exeptionConnectionId, out byte error, ConnectionSimulatorConfigInternal conf);

	public static int ConnectWithSimulator(int hostId, string address, int port, int exeptionConnectionId, out byte error, ConnectionSimulatorConfig conf)
	{
		return ConnectWithSimulatorInternal(hostId, address, port, exeptionConnectionId, out error, new ConnectionSimulatorConfigInternal(conf));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->Disconnect")]
	public static extern bool Disconnect(int hostId, int connectionId, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->ConnectSockAddr")]
	private static extern int Internal_ConnectEndPoint(int hostId, [Out] byte[] sockAddrStorage, int sockAddrStorageLen, int exceptionConnectionId, out byte error);

	public static bool Send(int hostId, int connectionId, int channelId, byte[] buffer, int size, out byte error)
	{
		if (buffer == null)
		{
			throw new NullReferenceException("send buffer is not initialized");
		}
		return SendWrapper(hostId, connectionId, channelId, buffer, size, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->Send")]
	private static extern bool SendWrapper(int hostId, int connectionId, int channelId, [Out] byte[] buffer, int size, out byte error);

	public static bool QueueMessageForSending(int hostId, int connectionId, int channelId, byte[] buffer, int size, out byte error)
	{
		if (buffer == null)
		{
			throw new NullReferenceException("send buffer is not initialized");
		}
		return QueueMessageForSendingWrapper(hostId, connectionId, channelId, buffer, size, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->QueueMessageForSending")]
	private static extern bool QueueMessageForSendingWrapper(int hostId, int connectionId, int channelId, [Out] byte[] buffer, int size, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->SendQueuedMessages")]
	public static extern bool SendQueuedMessages(int hostId, int connectionId, out byte error);

	public static NetworkEventType Receive(out int hostId, out int connectionId, out int channelId, byte[] buffer, int bufferSize, out int receivedSize, out byte error)
	{
		return (NetworkEventType)PopData(out hostId, out connectionId, out channelId, buffer, bufferSize, out receivedSize, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->PopData")]
	private static extern int PopData(out int hostId, out int connectionId, out int channelId, [Out] byte[] buffer, int bufferSize, out int receivedSize, out byte error);

	public static NetworkEventType ReceiveFromHost(int hostId, out int connectionId, out int channelId, byte[] buffer, int bufferSize, out int receivedSize, out byte error)
	{
		return (NetworkEventType)PopDataFromHost(hostId, out connectionId, out channelId, buffer, bufferSize, out receivedSize, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->PopDataFromHost")]
	private static extern int PopDataFromHost(int hostId, out int connectionId, out int channelId, [Out] byte[] buffer, int bufferSize, out int receivedSize, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->SetPacketStat")]
	public static extern void SetPacketStat(int direction, int packetStatId, int numMsgs, int numBytes);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeThrows]
	[FreeFunction("UNETManager::SetNetworkEventAvailableCallback")]
	private static extern void SetNetworkEventAvailableCallback(Action<int> callback);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Cleanup")]
	private static extern void Cleanup();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::SetConnectionReadyForSendCallback")]
	[NativeThrows]
	private static extern void SetConnectionReadyForSendCallback(Action<int, int> callback);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->NotifyWhenConnectionReadyForSend")]
	public static extern bool NotifyWhenConnectionReadyForSend(int hostId, int connectionId, int notificationLevel, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetHostPort")]
	public static extern int GetHostPort(int hostId);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->StartBroadcastDiscoveryWithData")]
	private static extern bool StartBroadcastDiscoveryWithData(int hostId, int broadcastPort, int key, int version, int subversion, [Out] byte[] buffer, int size, int timeout, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->StartBroadcastDiscoveryWithoutData")]
	private static extern bool StartBroadcastDiscoveryWithoutData(int hostId, int broadcastPort, int key, int version, int subversion, int timeout, out byte error);

	public static bool StartBroadcastDiscovery(int hostId, int broadcastPort, int key, int version, int subversion, byte[] buffer, int size, int timeout, out byte error)
	{
		if (buffer != null)
		{
			if (buffer.Length < size)
			{
				throw new ArgumentOutOfRangeException("Size: " + size + " > buffer.Length " + buffer.Length);
			}
			if (size == 0)
			{
				throw new ArgumentOutOfRangeException("Size is zero while buffer exists, please pass null and 0 as buffer and size parameters");
			}
		}
		if (buffer == null)
		{
			return StartBroadcastDiscoveryWithoutData(hostId, broadcastPort, key, version, subversion, timeout, out error);
		}
		return StartBroadcastDiscoveryWithData(hostId, broadcastPort, key, version, subversion, buffer, size, timeout, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->StopBroadcastDiscovery")]
	public static extern void StopBroadcastDiscovery();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->IsBroadcastDiscoveryRunning")]
	public static extern bool IsBroadcastDiscoveryRunning();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->SetBroadcastCredentials")]
	public static extern void SetBroadcastCredentials(int hostId, int key, int version, int subversion, out byte error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetBroadcastConnectionInfoInternal")]
	public static extern string GetBroadcastConnectionInfo(int hostId, out int port, out byte error);

	public static void GetBroadcastConnectionInfo(int hostId, out string address, out int port, out byte error)
	{
		address = GetBroadcastConnectionInfo(hostId, out port, out error);
	}

	public static void GetBroadcastConnectionMessage(int hostId, byte[] buffer, int bufferSize, out int receivedSize, out byte error)
	{
		GetBroadcastConnectionMessageInternal(hostId, buffer, bufferSize, out receivedSize, out error);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::SetMulticastLock")]
	public static extern void SetMulticastLock(bool enabled);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetBroadcastConnectionMessage")]
	private static extern void GetBroadcastConnectionMessageInternal(int hostId, [Out] byte[] buffer, int bufferSize, out int receivedSize, out byte error);

	private static void CheckTopology(HostTopology topology)
	{
		int maxPacketSize = GetMaxPacketSize();
		if (topology.DefaultConfig.PacketSize > maxPacketSize)
		{
			throw new ArgumentOutOfRangeException("Default config: packet size should be less than packet size defined in global config: " + maxPacketSize);
		}
		for (int i = 0; i < topology.SpecialConnectionConfigs.Count; i++)
		{
			if (topology.SpecialConnectionConfigs[i].PacketSize > maxPacketSize)
			{
				throw new ArgumentOutOfRangeException("Special config " + i + ": packet size should be less than packet size defined in global config: " + maxPacketSize);
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->LoadEncryptionLibrary")]
	private static extern bool LoadEncryptionLibraryInternal(string libraryName);

	public static bool LoadEncryptionLibrary(string libraryName)
	{
		return LoadEncryptionLibraryInternal(libraryName);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->UnloadEncryptionLibrary")]
	private static extern void UnloadEncryptionLibraryInternal();

	public static void UnloadEncryptionLibrary()
	{
		UnloadEncryptionLibraryInternal();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->IsEncryptionActive")]
	private static extern bool IsEncryptionActiveInternal();

	public static bool IsEncryptionActive()
	{
		return IsEncryptionActiveInternal();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[FreeFunction("UNETManager::Get()->GetEncryptionSafeMaxPacketSize")]
	private static extern short GetEncryptionSafeMaxPacketSizeInternal(short maxPacketSize);

	public static short GetEncryptionSafeMaxPacketSize(short maxPacketSize)
	{
		return GetEncryptionSafeMaxPacketSizeInternal(maxPacketSize);
	}
}
