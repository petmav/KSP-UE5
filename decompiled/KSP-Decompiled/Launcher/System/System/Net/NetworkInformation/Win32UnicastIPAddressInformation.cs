using System.Net.Sockets;

namespace System.Net.NetworkInformation;

internal class Win32UnicastIPAddressInformation : UnicastIPAddressInformation
{
	private Win32_IP_ADAPTER_UNICAST_ADDRESS info;

	private IPAddress ipv4Mask;

	public override IPAddress Address => info.Address.GetIPAddress();

	public override bool IsDnsEligible => info.LengthFlags.IsDnsEligible;

	public override bool IsTransient => info.LengthFlags.IsTransient;

	public override long AddressPreferredLifetime => info.PreferredLifetime;

	public override long AddressValidLifetime => info.ValidLifetime;

	public override long DhcpLeaseLifetime => info.LeaseLifetime;

	public override DuplicateAddressDetectionState DuplicateAddressDetectionState => info.DadState;

	public override IPAddress IPv4Mask
	{
		get
		{
			if (Address.AddressFamily != AddressFamily.InterNetwork)
			{
				return IPAddress.Any;
			}
			return ipv4Mask;
		}
	}

	public override PrefixOrigin PrefixOrigin => info.PrefixOrigin;

	public override SuffixOrigin SuffixOrigin => info.SuffixOrigin;

	public Win32UnicastIPAddressInformation(Win32_IP_ADAPTER_UNICAST_ADDRESS info)
	{
		this.info = info;
		IPAddress iPAddress = info.Address.GetIPAddress();
		if (iPAddress.AddressFamily == AddressFamily.InterNetwork)
		{
			ipv4Mask = PrefixLengthToSubnetMask(info.OnLinkPrefixLength, iPAddress.AddressFamily);
		}
	}

	private static IPAddress PrefixLengthToSubnetMask(byte prefixLength, AddressFamily family)
	{
		byte[] array = ((family != AddressFamily.InterNetwork) ? new byte[16] : new byte[4]);
		for (int i = 0; i < prefixLength; i++)
		{
			array[i / 8] |= (byte)(128 >> i % 8);
		}
		return new IPAddress(array);
	}
}
