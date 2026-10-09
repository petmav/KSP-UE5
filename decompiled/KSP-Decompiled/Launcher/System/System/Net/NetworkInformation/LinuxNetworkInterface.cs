using System.Globalization;
using System.IO;

namespace System.Net.NetworkInformation;

internal sealed class LinuxNetworkInterface : UnixNetworkInterface
{
	private string iface_path;

	private string iface_operstate_path;

	private string iface_flags_path;

	internal string IfacePath => iface_path;

	public override OperationalStatus OperationalStatus
	{
		get
		{
			if (!Directory.Exists(iface_path))
			{
				return OperationalStatus.Unknown;
			}
			try
			{
				switch (ReadLine(iface_operstate_path))
				{
				case "unknown":
					return OperationalStatus.Unknown;
				case "notpresent":
					return OperationalStatus.NotPresent;
				case "down":
					return OperationalStatus.Down;
				case "lowerlayerdown":
					return OperationalStatus.LowerLayerDown;
				case "testing":
					return OperationalStatus.Testing;
				case "dormant":
					return OperationalStatus.Dormant;
				case "up":
					return OperationalStatus.Up;
				}
			}
			catch
			{
			}
			return OperationalStatus.Unknown;
		}
	}

	public override bool SupportsMulticast
	{
		get
		{
			if (!Directory.Exists(iface_path))
			{
				return false;
			}
			try
			{
				string text = ReadLine(iface_flags_path);
				if (text.Length > 2 && text[0] == '0' && text[1] == 'x')
				{
					text = text.Substring(2);
				}
				return (ulong.Parse(text, NumberStyles.HexNumber) & 0x1000) == 4096;
			}
			catch
			{
				return false;
			}
		}
	}

	internal LinuxNetworkInterface(string name)
		: base(name)
	{
		iface_path = "/sys/class/net/" + name + "/";
		iface_operstate_path = iface_path + "operstate";
		iface_flags_path = iface_path + "flags";
	}

	public override IPInterfaceProperties GetIPProperties()
	{
		if (ipproperties == null)
		{
			ipproperties = new LinuxIPInterfaceProperties(this, addresses);
		}
		return ipproperties;
	}

	public override IPv4InterfaceStatistics GetIPv4Statistics()
	{
		if (ipv4stats == null)
		{
			ipv4stats = new LinuxIPv4InterfaceStatistics(this);
		}
		return ipv4stats;
	}

	internal static string ReadLine(string path)
	{
		using FileStream stream = File.OpenRead(path);
		using StreamReader streamReader = new StreamReader(stream);
		return streamReader.ReadLine();
	}
}
