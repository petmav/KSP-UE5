using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text.RegularExpressions;

namespace System.Net.NetworkInformation;

internal abstract class UnixIPInterfaceProperties : IPInterfaceProperties
{
	protected IPv4InterfaceProperties ipv4iface_properties;

	protected UnixNetworkInterface iface;

	private List<IPAddress> addresses;

	private IPAddressCollection dns_servers;

	private static Regex ns = new Regex("\\s*nameserver\\s+(?<address>.*)");

	private static Regex search = new Regex("\\s*search\\s+(?<domain>.*)");

	private string dns_suffix;

	private DateTime last_parse;

	public override IPAddressInformationCollection AnycastAddresses
	{
		get
		{
			IPAddressInformationCollection iPAddressInformationCollection = new IPAddressInformationCollection();
			foreach (IPAddress address in addresses)
			{
				iPAddressInformationCollection.InternalAdd(new SystemIPAddressInformation(address, isDnsEligible: false, isTransient: false));
			}
			return iPAddressInformationCollection;
		}
	}

	[System.MonoTODO("Always returns an empty collection.")]
	public override IPAddressCollection DhcpServerAddresses => new IPAddressCollection();

	public override IPAddressCollection DnsAddresses
	{
		get
		{
			ParseResolvConf();
			return dns_servers;
		}
	}

	public override string DnsSuffix
	{
		get
		{
			ParseResolvConf();
			return dns_suffix;
		}
	}

	[System.MonoTODO("Always returns true")]
	public override bool IsDnsEnabled => true;

	[System.MonoTODO("Always returns false")]
	public override bool IsDynamicDnsEnabled => false;

	public override MulticastIPAddressInformationCollection MulticastAddresses
	{
		get
		{
			MulticastIPAddressInformationCollection multicastIPAddressInformationCollection = new MulticastIPAddressInformationCollection();
			foreach (IPAddress address in addresses)
			{
				byte[] addressBytes = address.GetAddressBytes();
				if (addressBytes[0] >= 224 && addressBytes[0] <= 239)
				{
					multicastIPAddressInformationCollection.InternalAdd(new SystemMulticastIPAddressInformation(new SystemIPAddressInformation(address, isDnsEligible: true, isTransient: false)));
				}
			}
			return multicastIPAddressInformationCollection;
		}
	}

	public override UnicastIPAddressInformationCollection UnicastAddresses
	{
		get
		{
			UnicastIPAddressInformationCollection unicastIPAddressInformationCollection = new UnicastIPAddressInformationCollection();
			foreach (IPAddress address in addresses)
			{
				switch (address.AddressFamily)
				{
				case AddressFamily.InterNetwork:
				{
					byte b = address.GetAddressBytes()[0];
					if (b < 224 || b > 239)
					{
						unicastIPAddressInformationCollection.InternalAdd(new LinuxUnicastIPAddressInformation(address));
					}
					break;
				}
				case AddressFamily.InterNetworkV6:
					if (!address.IsIPv6Multicast)
					{
						unicastIPAddressInformationCollection.InternalAdd(new LinuxUnicastIPAddressInformation(address));
					}
					break;
				}
			}
			return unicastIPAddressInformationCollection;
		}
	}

	[System.MonoTODO("Always returns an empty collection.")]
	public override IPAddressCollection WinsServersAddresses => new IPAddressCollection();

	public UnixIPInterfaceProperties(UnixNetworkInterface iface, List<IPAddress> addresses)
	{
		this.iface = iface;
		this.addresses = addresses;
	}

	public override IPv6InterfaceProperties GetIPv6Properties()
	{
		throw new NotImplementedException();
	}

	private void ParseResolvConf()
	{
		try
		{
			DateTime lastWriteTime = File.GetLastWriteTime("/etc/resolv.conf");
			if (lastWriteTime <= last_parse)
			{
				return;
			}
			last_parse = lastWriteTime;
			dns_suffix = "";
			dns_servers = new IPAddressCollection();
			using StreamReader streamReader = new StreamReader("/etc/resolv.conf");
			string text;
			while ((text = streamReader.ReadLine()) != null)
			{
				text = text.Trim();
				if (text.Length == 0 || text[0] == '#')
				{
					continue;
				}
				Match match = ns.Match(text);
				if (match.Success)
				{
					try
					{
						string value = match.Groups["address"].Value;
						value = value.Trim();
						dns_servers.InternalAdd(IPAddress.Parse(value));
					}
					catch
					{
					}
					continue;
				}
				match = search.Match(text);
				if (match.Success)
				{
					string value = match.Groups["domain"].Value;
					string[] array = value.Split(',');
					dns_suffix = array[0].Trim();
				}
			}
		}
		catch
		{
		}
	}
}
