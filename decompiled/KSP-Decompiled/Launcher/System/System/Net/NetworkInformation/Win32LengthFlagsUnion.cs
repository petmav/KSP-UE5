namespace System.Net.NetworkInformation;

internal struct Win32LengthFlagsUnion
{
	private const int IP_ADAPTER_ADDRESS_DNS_ELIGIBLE = 1;

	private const int IP_ADAPTER_ADDRESS_TRANSIENT = 2;

	public uint Length;

	public uint Flags;

	public bool IsDnsEligible => (Flags & 1) != 0;

	public bool IsTransient => (Flags & 2) != 0;
}
