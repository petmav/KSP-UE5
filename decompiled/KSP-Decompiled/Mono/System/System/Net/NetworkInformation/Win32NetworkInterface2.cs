using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace System.Net.NetworkInformation;

internal class Win32NetworkInterface2 : NetworkInterface
{
	private Win32_IP_ADAPTER_ADDRESSES addr;

	private Win32_MIB_IFROW mib4;

	private Win32_MIB_IFROW mib6;

	private Win32IPv4InterfaceStatistics ip4stats;

	private IPInterfaceProperties ip_if_props;

	public override string Description => addr.Description;

	public override string Id => addr.AdapterName;

	public override bool IsReceiveOnly => addr.IsReceiveOnly;

	public override string Name => addr.FriendlyName;

	public override NetworkInterfaceType NetworkInterfaceType => addr.IfType;

	public override OperationalStatus OperationalStatus => addr.OperStatus;

	public override long Speed => (mib6.Index >= 0) ? mib6.Speed : mib4.Speed;

	public override bool SupportsMulticast => !addr.NoMulticast;

	[DllImport("iphlpapi.dll", SetLastError = true)]
	private static extern int GetAdaptersInfo(IntPtr info, ref int size);

	[DllImport("iphlpapi.dll", SetLastError = true)]
	private static extern int GetIfEntry(ref Win32_MIB_IFROW row);

	private static Win32_IP_ADAPTER_INFO[] GetAdaptersInfo()
	{
		int size = 0;
		GetAdaptersInfo(IntPtr.Zero, ref size);
		IntPtr intPtr = Marshal.AllocHGlobal(size);
		int adaptersInfo = GetAdaptersInfo(intPtr, ref size);
		if (adaptersInfo != 0)
		{
			throw new NetworkInformationException(adaptersInfo);
		}
		List<Win32_IP_ADAPTER_INFO> list = new List<Win32_IP_ADAPTER_INFO>();
		IntPtr intPtr2 = intPtr;
		while (intPtr2 != IntPtr.Zero)
		{
			Win32_IP_ADAPTER_INFO item = Marshal.PtrToStructure<Win32_IP_ADAPTER_INFO>(intPtr2);
			list.Add(item);
			intPtr2 = item.Next;
		}
		return list.ToArray();
	}

	internal Win32NetworkInterface2(Win32_IP_ADAPTER_ADDRESSES addr)
	{
		this.addr = addr;
		mib4 = default(Win32_MIB_IFROW);
		mib4.Index = addr.Alignment.IfIndex;
		if (GetIfEntry(ref mib4) != 0)
		{
			mib4.Index = -1;
		}
		mib6 = default(Win32_MIB_IFROW);
		mib6.Index = addr.Ipv6IfIndex;
		if (GetIfEntry(ref mib6) != 0)
		{
			mib6.Index = -1;
		}
		ip4stats = new Win32IPv4InterfaceStatistics(mib4);
		ip_if_props = new Win32IPInterfaceProperties2(addr, mib4, mib6);
	}

	public override IPInterfaceProperties GetIPProperties()
	{
		return ip_if_props;
	}

	public override IPv4InterfaceStatistics GetIPv4Statistics()
	{
		return ip4stats;
	}

	public override PhysicalAddress GetPhysicalAddress()
	{
		byte[] array = new byte[addr.PhysicalAddressLength];
		Array.Copy(addr.PhysicalAddress, 0, array, 0, array.Length);
		return new PhysicalAddress(array);
	}

	public override bool Supports(NetworkInterfaceComponent networkInterfaceComponent)
	{
		return networkInterfaceComponent switch
		{
			NetworkInterfaceComponent.IPv4 => mib4.Index >= 0, 
			NetworkInterfaceComponent.IPv6 => mib6.Index >= 0, 
			_ => false, 
		};
	}
}
