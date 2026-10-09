using System.Collections.Generic;
using System.IO;
using System.Net.NetworkInformation.MacOsStructs;
using System.Runtime.InteropServices;

namespace System.Net.NetworkInformation;

internal abstract class NetworkInterfaceFactory
{
	internal abstract class UnixNetworkInterfaceAPI : NetworkInterfaceFactory
	{
		[DllImport("libc")]
		public static extern int if_nametoindex(string ifname);

		[DllImport("libc")]
		protected static extern int getifaddrs(out IntPtr ifap);

		[DllImport("libc")]
		protected static extern void freeifaddrs(IntPtr ifap);
	}

	private class MacOsNetworkInterfaceAPI : UnixNetworkInterfaceAPI
	{
		private const int AF_INET = 2;

		private const int AF_INET6 = 30;

		private const int AF_LINK = 18;

		public override NetworkInterface[] GetAllNetworkInterfaces()
		{
			Dictionary<string, MacOsNetworkInterface> dictionary = new Dictionary<string, MacOsNetworkInterface>();
			if (UnixNetworkInterfaceAPI.getifaddrs(out var ifap) != 0)
			{
				throw new SystemException("getifaddrs() failed");
			}
			try
			{
				IntPtr intPtr = ifap;
				while (intPtr != IntPtr.Zero)
				{
					System.Net.NetworkInformation.MacOsStructs.ifaddrs ifaddrs2 = (System.Net.NetworkInformation.MacOsStructs.ifaddrs)Marshal.PtrToStructure(intPtr, typeof(System.Net.NetworkInformation.MacOsStructs.ifaddrs));
					IPAddress iPAddress = IPAddress.None;
					string ifa_name = ifaddrs2.ifa_name;
					int index = -1;
					byte[] array = null;
					NetworkInterfaceType networkInterfaceType = NetworkInterfaceType.Unknown;
					if (ifaddrs2.ifa_addr != IntPtr.Zero)
					{
						sockaddr sockaddr = (sockaddr)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(sockaddr));
						if (sockaddr.sa_family == 30)
						{
							System.Net.NetworkInformation.MacOsStructs.sockaddr_in6 sockaddr_in7 = (System.Net.NetworkInformation.MacOsStructs.sockaddr_in6)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(System.Net.NetworkInformation.MacOsStructs.sockaddr_in6));
							iPAddress = new IPAddress(sockaddr_in7.sin6_addr.u6_addr8, sockaddr_in7.sin6_scope_id);
						}
						else if (sockaddr.sa_family == 2)
						{
							iPAddress = new IPAddress(((System.Net.NetworkInformation.MacOsStructs.sockaddr_in)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(System.Net.NetworkInformation.MacOsStructs.sockaddr_in))).sin_addr);
						}
						else if (sockaddr.sa_family == 18)
						{
							sockaddr_dl sockaddr_dl = default(sockaddr_dl);
							sockaddr_dl.Read(ifaddrs2.ifa_addr);
							array = new byte[sockaddr_dl.sdl_alen];
							Array.Copy(sockaddr_dl.sdl_data, sockaddr_dl.sdl_nlen, array, 0, Math.Min(array.Length, sockaddr_dl.sdl_data.Length - sockaddr_dl.sdl_nlen));
							index = sockaddr_dl.sdl_index;
							int sdl_type = sockaddr_dl.sdl_type;
							if (Enum.IsDefined(typeof(MacOsArpHardware), sdl_type))
							{
								switch ((MacOsArpHardware)sdl_type)
								{
								case MacOsArpHardware.ETHER:
									networkInterfaceType = NetworkInterfaceType.Ethernet;
									break;
								case MacOsArpHardware.ATM:
									networkInterfaceType = NetworkInterfaceType.Atm;
									break;
								case MacOsArpHardware.SLIP:
									networkInterfaceType = NetworkInterfaceType.Slip;
									break;
								case MacOsArpHardware.PPP:
									networkInterfaceType = NetworkInterfaceType.Ppp;
									break;
								case MacOsArpHardware.LOOPBACK:
									networkInterfaceType = NetworkInterfaceType.Loopback;
									array = null;
									break;
								case MacOsArpHardware.FDDI:
									networkInterfaceType = NetworkInterfaceType.Fddi;
									break;
								}
							}
						}
					}
					MacOsNetworkInterface value = null;
					if (!dictionary.TryGetValue(ifa_name, out value))
					{
						value = new MacOsNetworkInterface(ifa_name, ifaddrs2.ifa_flags);
						dictionary.Add(ifa_name, value);
					}
					if (!iPAddress.Equals(IPAddress.None))
					{
						value.AddAddress(iPAddress);
					}
					if (array != null || networkInterfaceType == NetworkInterfaceType.Loopback)
					{
						value.SetLinkLayerInfo(index, array, networkInterfaceType);
					}
					intPtr = ifaddrs2.ifa_next;
				}
			}
			finally
			{
				UnixNetworkInterfaceAPI.freeifaddrs(ifap);
			}
			NetworkInterface[] array2 = new NetworkInterface[dictionary.Count];
			int num = 0;
			foreach (MacOsNetworkInterface value2 in dictionary.Values)
			{
				array2[num] = value2;
				num++;
			}
			return array2;
		}

		public override int GetLoopbackInterfaceIndex()
		{
			return UnixNetworkInterfaceAPI.if_nametoindex("lo0");
		}

		public override IPAddress GetNetMask(IPAddress address)
		{
			if (UnixNetworkInterfaceAPI.getifaddrs(out var ifap) != 0)
			{
				throw new SystemException("getifaddrs() failed");
			}
			try
			{
				IntPtr intPtr = ifap;
				while (intPtr != IntPtr.Zero)
				{
					System.Net.NetworkInformation.MacOsStructs.ifaddrs ifaddrs2 = (System.Net.NetworkInformation.MacOsStructs.ifaddrs)Marshal.PtrToStructure(intPtr, typeof(System.Net.NetworkInformation.MacOsStructs.ifaddrs));
					if (ifaddrs2.ifa_addr != IntPtr.Zero && ((sockaddr)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(sockaddr))).sa_family == 2)
					{
						IPAddress obj = new IPAddress(((System.Net.NetworkInformation.MacOsStructs.sockaddr_in)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(System.Net.NetworkInformation.MacOsStructs.sockaddr_in))).sin_addr);
						if (address.Equals(obj))
						{
							return new IPAddress(((sockaddr_in)Marshal.PtrToStructure(ifaddrs2.ifa_netmask, typeof(sockaddr_in))).sin_addr);
						}
					}
					intPtr = ifaddrs2.ifa_next;
				}
			}
			finally
			{
				UnixNetworkInterfaceAPI.freeifaddrs(ifap);
			}
			return null;
		}
	}

	private class LinuxNetworkInterfaceAPI : UnixNetworkInterfaceAPI
	{
		private const int AF_INET = 2;

		private const int AF_INET6 = 10;

		private const int AF_PACKET = 17;

		private static void FreeInterfaceAddresses(IntPtr ifap)
		{
			UnixNetworkInterfaceAPI.freeifaddrs(ifap);
		}

		private static int GetInterfaceAddresses(out IntPtr ifap)
		{
			return UnixNetworkInterfaceAPI.getifaddrs(out ifap);
		}

		public override NetworkInterface[] GetAllNetworkInterfaces()
		{
			Dictionary<string, LinuxNetworkInterface> dictionary = new Dictionary<string, LinuxNetworkInterface>();
			if (GetInterfaceAddresses(out var ifap) != 0)
			{
				throw new SystemException("getifaddrs() failed");
			}
			try
			{
				IntPtr intPtr = ifap;
				while (intPtr != IntPtr.Zero)
				{
					ifaddrs ifaddrs2 = (ifaddrs)Marshal.PtrToStructure(intPtr, typeof(ifaddrs));
					IPAddress iPAddress = IPAddress.None;
					string text = ifaddrs2.ifa_name;
					int index = -1;
					byte[] array = null;
					NetworkInterfaceType networkInterfaceType = NetworkInterfaceType.Unknown;
					int num = 0;
					if (ifaddrs2.ifa_addr != IntPtr.Zero)
					{
						sockaddr_in sockaddr_in7 = (sockaddr_in)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(sockaddr_in));
						if (sockaddr_in7.sin_family == 10)
						{
							sockaddr_in6 sockaddr_in8 = (sockaddr_in6)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(sockaddr_in6));
							iPAddress = new IPAddress(sockaddr_in8.sin6_addr.u6_addr8, sockaddr_in8.sin6_scope_id);
						}
						else if (sockaddr_in7.sin_family == 2)
						{
							iPAddress = new IPAddress(sockaddr_in7.sin_addr);
						}
						else if (sockaddr_in7.sin_family == 17)
						{
							sockaddr_ll sockaddr_ll2 = (sockaddr_ll)Marshal.PtrToStructure(ifaddrs2.ifa_addr, typeof(sockaddr_ll));
							if (sockaddr_ll2.sll_halen > sockaddr_ll2.sll_addr.Length)
							{
								intPtr = ifaddrs2.ifa_next;
								continue;
							}
							array = new byte[sockaddr_ll2.sll_halen];
							Array.Copy(sockaddr_ll2.sll_addr, 0, array, 0, array.Length);
							index = sockaddr_ll2.sll_ifindex;
							int sll_hatype = sockaddr_ll2.sll_hatype;
							if (Enum.IsDefined(typeof(LinuxArpHardware), sll_hatype))
							{
								switch ((LinuxArpHardware)sll_hatype)
								{
								case LinuxArpHardware.ETHER:
								case LinuxArpHardware.EETHER:
									networkInterfaceType = NetworkInterfaceType.Ethernet;
									break;
								case LinuxArpHardware.PRONET:
									networkInterfaceType = NetworkInterfaceType.TokenRing;
									break;
								case LinuxArpHardware.ATM:
									networkInterfaceType = NetworkInterfaceType.Atm;
									break;
								case LinuxArpHardware.SLIP:
								case LinuxArpHardware.CSLIP:
								case LinuxArpHardware.SLIP6:
								case LinuxArpHardware.CSLIP6:
									networkInterfaceType = NetworkInterfaceType.Slip;
									break;
								case LinuxArpHardware.PPP:
									networkInterfaceType = NetworkInterfaceType.Ppp;
									break;
								case LinuxArpHardware.LOOPBACK:
									networkInterfaceType = NetworkInterfaceType.Loopback;
									array = null;
									break;
								case LinuxArpHardware.FDDI:
									networkInterfaceType = NetworkInterfaceType.Fddi;
									break;
								case LinuxArpHardware.TUNNEL:
								case LinuxArpHardware.TUNNEL6:
								case LinuxArpHardware.SIT:
								case LinuxArpHardware.IPDDP:
								case LinuxArpHardware.IPGRE:
								case LinuxArpHardware.IP6GRE:
									networkInterfaceType = NetworkInterfaceType.Tunnel;
									break;
								}
							}
						}
					}
					LinuxNetworkInterface value = null;
					if (string.IsNullOrEmpty(text))
					{
						int num2 = num + 1;
						num = num2;
						text = "\0" + num2;
					}
					if (!dictionary.TryGetValue(text, out value))
					{
						value = new LinuxNetworkInterface(text);
						dictionary.Add(text, value);
					}
					if (!iPAddress.Equals(IPAddress.None))
					{
						value.AddAddress(iPAddress);
					}
					if (array != null || networkInterfaceType == NetworkInterfaceType.Loopback)
					{
						if (networkInterfaceType == NetworkInterfaceType.Ethernet && Directory.Exists(value.IfacePath + "wireless"))
						{
							networkInterfaceType = NetworkInterfaceType.Wireless80211;
						}
						value.SetLinkLayerInfo(index, array, networkInterfaceType);
					}
					intPtr = ifaddrs2.ifa_next;
				}
			}
			finally
			{
				FreeInterfaceAddresses(ifap);
			}
			NetworkInterface[] array2 = new NetworkInterface[dictionary.Count];
			int num3 = 0;
			foreach (LinuxNetworkInterface value2 in dictionary.Values)
			{
				array2[num3] = value2;
				num3++;
			}
			return array2;
		}

		public override int GetLoopbackInterfaceIndex()
		{
			return UnixNetworkInterfaceAPI.if_nametoindex("lo");
		}

		public override IPAddress GetNetMask(IPAddress address)
		{
			foreach (ifaddrs networkInterface in GetNetworkInterfaces())
			{
				if (!(networkInterface.ifa_addr == IntPtr.Zero))
				{
					sockaddr_in sockaddr_in7 = (sockaddr_in)Marshal.PtrToStructure(networkInterface.ifa_addr, typeof(sockaddr_in));
					if (sockaddr_in7.sin_family == 2 && address.Equals(new IPAddress(sockaddr_in7.sin_addr)))
					{
						return new IPAddress(((sockaddr_in)Marshal.PtrToStructure(networkInterface.ifa_netmask, typeof(sockaddr_in))).sin_addr);
					}
				}
			}
			return null;
		}

		private static IEnumerable<ifaddrs> GetNetworkInterfaces()
		{
			IntPtr ifap = IntPtr.Zero;
			try
			{
				if (GetInterfaceAddresses(out ifap) == 0)
				{
					IntPtr intPtr = ifap;
					while (intPtr != IntPtr.Zero)
					{
						ifaddrs addr = (ifaddrs)Marshal.PtrToStructure(intPtr, typeof(ifaddrs));
						yield return addr;
						intPtr = addr.ifa_next;
					}
				}
			}
			finally
			{
				if (ifap != IntPtr.Zero)
				{
					FreeInterfaceAddresses(ifap);
				}
			}
		}
	}

	private class Win32NetworkInterfaceAPI : NetworkInterfaceFactory
	{
		private const string IPHLPAPI = "iphlpapi.dll";

		[DllImport("iphlpapi.dll", SetLastError = true)]
		private static extern int GetAdaptersAddresses(uint family, uint flags, IntPtr reserved, IntPtr info, ref int size);

		[DllImport("iphlpapi.dll")]
		private static extern uint GetBestInterfaceEx(byte[] ipAddress, out int index);

		private static Win32_IP_ADAPTER_ADDRESSES[] GetAdaptersAddresses()
		{
			IntPtr zero = IntPtr.Zero;
			int size = 0;
			uint flags = 192u;
			GetAdaptersAddresses(0u, flags, IntPtr.Zero, zero, ref size);
			if (Marshal.SizeOf(typeof(Win32_IP_ADAPTER_ADDRESSES)) > size)
			{
				throw new NetworkInformationException();
			}
			zero = Marshal.AllocHGlobal(size);
			int adaptersAddresses = GetAdaptersAddresses(0u, flags, IntPtr.Zero, zero, ref size);
			if (adaptersAddresses != 0)
			{
				throw new NetworkInformationException(adaptersAddresses);
			}
			List<Win32_IP_ADAPTER_ADDRESSES> list = new List<Win32_IP_ADAPTER_ADDRESSES>();
			IntPtr intPtr = zero;
			while (intPtr != IntPtr.Zero)
			{
				Win32_IP_ADAPTER_ADDRESSES item = Marshal.PtrToStructure<Win32_IP_ADAPTER_ADDRESSES>(intPtr);
				list.Add(item);
				intPtr = item.Next;
			}
			return list.ToArray();
		}

		public override NetworkInterface[] GetAllNetworkInterfaces()
		{
			Win32_IP_ADAPTER_ADDRESSES[] adaptersAddresses = GetAdaptersAddresses();
			NetworkInterface[] array = new NetworkInterface[adaptersAddresses.Length];
			for (int i = 0; i < array.Length; i++)
			{
				array[i] = new Win32NetworkInterface2(adaptersAddresses[i]);
			}
			return array;
		}

		private static int GetBestInterfaceForAddress(IPAddress addr)
		{
			int index;
			int bestInterfaceEx = (int)GetBestInterfaceEx(new SocketAddress(addr).m_Buffer, out index);
			if (bestInterfaceEx != 0)
			{
				throw new NetworkInformationException(bestInterfaceEx);
			}
			return index;
		}

		public override int GetLoopbackInterfaceIndex()
		{
			return GetBestInterfaceForAddress(IPAddress.Loopback);
		}

		public override IPAddress GetNetMask(IPAddress address)
		{
			throw new NotImplementedException();
		}
	}

	public abstract NetworkInterface[] GetAllNetworkInterfaces();

	public abstract int GetLoopbackInterfaceIndex();

	public abstract IPAddress GetNetMask(IPAddress address);

	public static NetworkInterfaceFactory Create()
	{
		if (Environment.OSVersion.Platform == PlatformID.Unix)
		{
			if (Platform.IsMacOS || Platform.IsFreeBSD)
			{
				return new MacOsNetworkInterfaceAPI();
			}
			return new LinuxNetworkInterfaceAPI();
		}
		Version version = new Version(5, 1);
		if (Environment.OSVersion.Version >= version)
		{
			return new Win32NetworkInterfaceAPI();
		}
		throw new NotImplementedException();
	}
}
