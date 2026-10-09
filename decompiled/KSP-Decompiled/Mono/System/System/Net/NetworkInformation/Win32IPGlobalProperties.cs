using System.Collections.Generic;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace System.Net.NetworkInformation;

internal class Win32IPGlobalProperties : IPGlobalProperties
{
	[StructLayout(LayoutKind.Explicit)]
	private struct Win32_IN6_ADDR
	{
		[FieldOffset(0)]
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
		public byte[] Bytes;
	}

	[StructLayout(LayoutKind.Sequential)]
	private class Win32_MIB_TCPROW
	{
		public TcpState State;

		public uint LocalAddr;

		public uint LocalPort;

		public uint RemoteAddr;

		public uint RemotePort;

		public IPEndPoint LocalEndPoint => new IPEndPoint(LocalAddr, ntohs((ushort)LocalPort));

		public IPEndPoint RemoteEndPoint => new IPEndPoint(RemoteAddr, ntohs((ushort)RemotePort));

		public TcpConnectionInformation TcpInfo => new SystemTcpConnectionInformation(LocalEndPoint, RemoteEndPoint, State);
	}

	[StructLayout(LayoutKind.Sequential)]
	private class Win32_MIB_TCP6ROW
	{
		public TcpState State;

		public Win32_IN6_ADDR LocalAddr;

		public uint LocalScopeId;

		public uint LocalPort;

		public Win32_IN6_ADDR RemoteAddr;

		public uint RemoteScopeId;

		public uint RemotePort;

		public IPEndPoint LocalEndPoint => new IPEndPoint(new IPAddress(LocalAddr.Bytes, LocalScopeId), ntohs((ushort)LocalPort));

		public IPEndPoint RemoteEndPoint => new IPEndPoint(new IPAddress(RemoteAddr.Bytes, RemoteScopeId), ntohs((ushort)RemotePort));

		public TcpConnectionInformation TcpInfo => new SystemTcpConnectionInformation(LocalEndPoint, RemoteEndPoint, State);
	}

	[StructLayout(LayoutKind.Sequential)]
	private class Win32_MIB_UDPROW
	{
		public uint LocalAddr;

		public uint LocalPort;

		public IPEndPoint LocalEndPoint => new IPEndPoint(LocalAddr, ntohs((ushort)LocalPort));
	}

	[StructLayout(LayoutKind.Sequential)]
	private class Win32_MIB_UDP6ROW
	{
		public Win32_IN6_ADDR LocalAddr;

		public uint LocalScopeId;

		public uint LocalPort;

		public IPEndPoint LocalEndPoint => new IPEndPoint(new IPAddress(LocalAddr.Bytes, LocalScopeId), ntohs((ushort)LocalPort));
	}

	public const int AF_INET = 2;

	public const int AF_INET6 = 23;

	public override string DhcpScopeName => Win32NetworkInterface.FixedInfo.ScopeId;

	public override string DomainName => Win32NetworkInterface.FixedInfo.DomainName;

	public override string HostName => Win32NetworkInterface.FixedInfo.HostName;

	public override bool IsWinsProxy => Win32NetworkInterface.FixedInfo.EnableProxy != 0;

	public override NetBiosNodeType NodeType => Win32NetworkInterface.FixedInfo.NodeType;

	private unsafe void FillTcpTable(out List<Win32_MIB_TCPROW> tab4, out List<Win32_MIB_TCP6ROW> tab6)
	{
		tab4 = new List<Win32_MIB_TCPROW>();
		int pdwSize = 0;
		GetTcpTable(null, ref pdwSize, bOrder: true);
		byte[] array = new byte[pdwSize];
		GetTcpTable(array, ref pdwSize, bOrder: true);
		int num = Marshal.SizeOf(typeof(Win32_MIB_TCPROW));
		fixed (byte* ptr = array)
		{
			int num2 = Marshal.ReadInt32((IntPtr)ptr);
			for (int i = 0; i < num2; i++)
			{
				Win32_MIB_TCPROW win32_MIB_TCPROW = new Win32_MIB_TCPROW();
				Marshal.PtrToStructure((IntPtr)(ptr + i * num + 4), win32_MIB_TCPROW);
				tab4.Add(win32_MIB_TCPROW);
			}
		}
		tab6 = new List<Win32_MIB_TCP6ROW>();
		if (Environment.OSVersion.Version.Major < 6)
		{
			return;
		}
		int SizePointer = 0;
		GetTcp6Table(null, ref SizePointer, Order: true);
		byte[] array2 = new byte[SizePointer];
		GetTcp6Table(array2, ref SizePointer, Order: true);
		int num3 = Marshal.SizeOf(typeof(Win32_MIB_TCP6ROW));
		fixed (byte* ptr2 = array2)
		{
			int num4 = Marshal.ReadInt32((IntPtr)ptr2);
			for (int j = 0; j < num4; j++)
			{
				Win32_MIB_TCP6ROW win32_MIB_TCP6ROW = new Win32_MIB_TCP6ROW();
				Marshal.PtrToStructure((IntPtr)(ptr2 + j * num3 + 4), win32_MIB_TCP6ROW);
				tab6.Add(win32_MIB_TCP6ROW);
			}
		}
	}

	private bool IsListenerState(TcpState state)
	{
		if ((uint)(state - 2) <= 1u || (uint)(state - 6) <= 2u)
		{
			return true;
		}
		return false;
	}

	public override TcpConnectionInformation[] GetActiveTcpConnections()
	{
		List<Win32_MIB_TCPROW> tab = null;
		List<Win32_MIB_TCP6ROW> tab2 = null;
		FillTcpTable(out tab, out tab2);
		int count = tab.Count;
		TcpConnectionInformation[] array = new TcpConnectionInformation[count + tab2.Count];
		for (int i = 0; i < count; i++)
		{
			array[i] = tab[i].TcpInfo;
		}
		for (int j = 0; j < tab2.Count; j++)
		{
			array[count + j] = tab2[j].TcpInfo;
		}
		return array;
	}

	public override IPEndPoint[] GetActiveTcpListeners()
	{
		List<Win32_MIB_TCPROW> tab = null;
		List<Win32_MIB_TCP6ROW> tab2 = null;
		FillTcpTable(out tab, out tab2);
		List<IPEndPoint> list = new List<IPEndPoint>();
		int i = 0;
		for (int count = tab.Count; i < count; i++)
		{
			if (IsListenerState(tab[i].State))
			{
				list.Add(tab[i].LocalEndPoint);
			}
		}
		int j = 0;
		for (int count2 = tab2.Count; j < count2; j++)
		{
			if (IsListenerState(tab2[j].State))
			{
				list.Add(tab2[j].LocalEndPoint);
			}
		}
		return list.ToArray();
	}

	public unsafe override IPEndPoint[] GetActiveUdpListeners()
	{
		List<IPEndPoint> list = new List<IPEndPoint>();
		int pdwSize = 0;
		GetUdpTable(null, ref pdwSize, bOrder: true);
		byte[] array = new byte[pdwSize];
		GetUdpTable(array, ref pdwSize, bOrder: true);
		int num = Marshal.SizeOf(typeof(Win32_MIB_UDPROW));
		fixed (byte* ptr = array)
		{
			int num2 = Marshal.ReadInt32((IntPtr)ptr);
			for (int i = 0; i < num2; i++)
			{
				Win32_MIB_UDPROW win32_MIB_UDPROW = new Win32_MIB_UDPROW();
				Marshal.PtrToStructure((IntPtr)(ptr + i * num + 4), win32_MIB_UDPROW);
				list.Add(win32_MIB_UDPROW.LocalEndPoint);
			}
		}
		if (Environment.OSVersion.Version.Major >= 6)
		{
			int SizePointer = 0;
			GetUdp6Table(null, ref SizePointer, Order: true);
			byte[] array2 = new byte[SizePointer];
			GetUdp6Table(array2, ref SizePointer, Order: true);
			int num3 = Marshal.SizeOf(typeof(Win32_MIB_UDP6ROW));
			fixed (byte* ptr2 = array2)
			{
				int num4 = Marshal.ReadInt32((IntPtr)ptr2);
				for (int j = 0; j < num4; j++)
				{
					Win32_MIB_UDP6ROW win32_MIB_UDP6ROW = new Win32_MIB_UDP6ROW();
					Marshal.PtrToStructure((IntPtr)(ptr2 + j * num3 + 4), win32_MIB_UDP6ROW);
					list.Add(win32_MIB_UDP6ROW.LocalEndPoint);
				}
			}
		}
		return list.ToArray();
	}

	public override IcmpV4Statistics GetIcmpV4Statistics()
	{
		if (!Socket.OSSupportsIPv4)
		{
			throw new NetworkInformationException();
		}
		GetIcmpStatistics(out var pStats, 2);
		return new Win32IcmpV4Statistics(pStats);
	}

	public override IcmpV6Statistics GetIcmpV6Statistics()
	{
		if (!Socket.OSSupportsIPv6)
		{
			throw new NetworkInformationException();
		}
		GetIcmpStatisticsEx(out var pStats, 23);
		return new Win32IcmpV6Statistics(pStats);
	}

	public override IPGlobalStatistics GetIPv4GlobalStatistics()
	{
		if (!Socket.OSSupportsIPv4)
		{
			throw new NetworkInformationException();
		}
		GetIpStatisticsEx(out var pStats, 2);
		return new Win32IPGlobalStatistics(pStats);
	}

	public override IPGlobalStatistics GetIPv6GlobalStatistics()
	{
		if (!Socket.OSSupportsIPv6)
		{
			throw new NetworkInformationException();
		}
		GetIpStatisticsEx(out var pStats, 23);
		return new Win32IPGlobalStatistics(pStats);
	}

	public override TcpStatistics GetTcpIPv4Statistics()
	{
		if (!Socket.OSSupportsIPv4)
		{
			throw new NetworkInformationException();
		}
		GetTcpStatisticsEx(out var pStats, 2);
		return new Win32TcpStatistics(pStats);
	}

	public override TcpStatistics GetTcpIPv6Statistics()
	{
		if (!Socket.OSSupportsIPv6)
		{
			throw new NetworkInformationException();
		}
		GetTcpStatisticsEx(out var pStats, 23);
		return new Win32TcpStatistics(pStats);
	}

	public override UdpStatistics GetUdpIPv4Statistics()
	{
		if (!Socket.OSSupportsIPv4)
		{
			throw new NetworkInformationException();
		}
		GetUdpStatisticsEx(out var pStats, 2);
		return new Win32UdpStatistics(pStats);
	}

	public override UdpStatistics GetUdpIPv6Statistics()
	{
		if (!Socket.OSSupportsIPv6)
		{
			throw new NetworkInformationException();
		}
		GetUdpStatisticsEx(out var pStats, 23);
		return new Win32UdpStatistics(pStats);
	}

	[DllImport("iphlpapi.dll")]
	private static extern int GetTcpTable(byte[] pTcpTable, ref int pdwSize, bool bOrder);

	[DllImport("iphlpapi.dll")]
	private static extern int GetTcp6Table(byte[] TcpTable, ref int SizePointer, bool Order);

	[DllImport("iphlpapi.dll")]
	private static extern int GetUdpTable(byte[] pUdpTable, ref int pdwSize, bool bOrder);

	[DllImport("iphlpapi.dll")]
	private static extern int GetUdp6Table(byte[] Udp6Table, ref int SizePointer, bool Order);

	[DllImport("iphlpapi.dll")]
	private static extern int GetTcpStatisticsEx(out Win32_MIB_TCPSTATS pStats, int dwFamily);

	[DllImport("iphlpapi.dll")]
	private static extern int GetUdpStatisticsEx(out Win32_MIB_UDPSTATS pStats, int dwFamily);

	[DllImport("iphlpapi.dll")]
	private static extern int GetIcmpStatistics(out Win32_MIBICMPINFO pStats, int dwFamily);

	[DllImport("iphlpapi.dll")]
	private static extern int GetIcmpStatisticsEx(out Win32_MIB_ICMP_EX pStats, int dwFamily);

	[DllImport("iphlpapi.dll")]
	private static extern int GetIpStatisticsEx(out Win32_MIB_IPSTATS pStats, int dwFamily);

	[DllImport("Ws2_32.dll")]
	private static extern ushort ntohs(ushort netshort);
}
