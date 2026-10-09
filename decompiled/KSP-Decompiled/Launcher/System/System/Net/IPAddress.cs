using System.Globalization;
using System.Net.Sockets;

namespace System.Net;

[Serializable]
public class IPAddress
{
	public static readonly IPAddress Any = new IPAddress(0);

	public static readonly IPAddress Loopback = new IPAddress(16777343);

	public static readonly IPAddress Broadcast = new IPAddress(4294967295L);

	public static readonly IPAddress None = Broadcast;

	internal const long LoopbackMask = 255L;

	internal long m_Address;

	[NonSerialized]
	internal string m_ToString;

	public static readonly IPAddress IPv6Any = new IPAddress(new byte[16], 0L);

	public static readonly IPAddress IPv6Loopback = new IPAddress(new byte[16]
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1
	}, 0L);

	public static readonly IPAddress IPv6None = new IPAddress(new byte[16], 0L);

	private AddressFamily m_Family = AddressFamily.InterNetwork;

	private ushort[] m_Numbers = new ushort[8];

	private long m_ScopeId;

	private int m_HashCode;

	internal const int IPv4AddressBytes = 4;

	internal const int IPv6AddressBytes = 16;

	internal const int NumberOfLabels = 8;

	[Obsolete("This property has been deprecated. It is address family dependent. Please use IPAddress.Equals method to perform comparisons. http://go.microsoft.com/fwlink/?linkid=14202")]
	public long Address
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				throw new SocketException(SocketError.OperationNotSupported);
			}
			return m_Address;
		}
		set
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				throw new SocketException(SocketError.OperationNotSupported);
			}
			if (m_Address != value)
			{
				m_ToString = null;
				m_Address = value;
			}
		}
	}

	public AddressFamily AddressFamily => m_Family;

	public long ScopeId
	{
		get
		{
			if (m_Family == AddressFamily.InterNetwork)
			{
				throw new SocketException(SocketError.OperationNotSupported);
			}
			return m_ScopeId;
		}
		set
		{
			if (m_Family == AddressFamily.InterNetwork)
			{
				throw new SocketException(SocketError.OperationNotSupported);
			}
			if (value < 0 || value > uint.MaxValue)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			if (m_ScopeId != value)
			{
				m_Address = value;
				m_ScopeId = value;
			}
		}
	}

	internal bool IsBroadcast
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				return false;
			}
			return m_Address == Broadcast.m_Address;
		}
	}

	public bool IsIPv6Multicast
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				return (m_Numbers[0] & 0xFF00) == 65280;
			}
			return false;
		}
	}

	public bool IsIPv6LinkLocal
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				return (m_Numbers[0] & 0xFFC0) == 65152;
			}
			return false;
		}
	}

	public bool IsIPv6SiteLocal
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				return (m_Numbers[0] & 0xFFC0) == 65216;
			}
			return false;
		}
	}

	public bool IsIPv6Teredo
	{
		get
		{
			if (m_Family == AddressFamily.InterNetworkV6 && m_Numbers[0] == 8193)
			{
				return m_Numbers[1] == 0;
			}
			return false;
		}
	}

	public bool IsIPv4MappedToIPv6
	{
		get
		{
			if (AddressFamily != AddressFamily.InterNetworkV6)
			{
				return false;
			}
			for (int i = 0; i < 5; i++)
			{
				if (m_Numbers[i] != 0)
				{
					return false;
				}
			}
			return m_Numbers[5] == ushort.MaxValue;
		}
	}

	public IPAddress(long newAddress)
	{
		if (newAddress < 0 || newAddress > uint.MaxValue)
		{
			throw new ArgumentOutOfRangeException("newAddress");
		}
		m_Address = newAddress;
	}

	public IPAddress(byte[] address, long scopeid)
	{
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (address.Length != 16)
		{
			throw new ArgumentException(global::SR.GetString("An invalid IP address was specified."), "address");
		}
		m_Family = AddressFamily.InterNetworkV6;
		for (int i = 0; i < 8; i++)
		{
			m_Numbers[i] = (ushort)(address[i * 2] * 256 + address[i * 2 + 1]);
		}
		if (scopeid < 0 || scopeid > uint.MaxValue)
		{
			throw new ArgumentOutOfRangeException("scopeid");
		}
		m_ScopeId = scopeid;
	}

	private IPAddress(ushort[] address, uint scopeid)
	{
		m_Family = AddressFamily.InterNetworkV6;
		m_Numbers = address;
		m_ScopeId = scopeid;
	}

	public IPAddress(byte[] address)
	{
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (address.Length != 4 && address.Length != 16)
		{
			throw new ArgumentException(global::SR.GetString("An invalid IP address was specified."), "address");
		}
		if (address.Length == 4)
		{
			m_Family = AddressFamily.InterNetwork;
			m_Address = ((address[3] << 24) | (address[2] << 16) | (address[1] << 8) | address[0]) & 0xFFFFFFFFu;
			return;
		}
		m_Family = AddressFamily.InterNetworkV6;
		for (int i = 0; i < 8; i++)
		{
			m_Numbers[i] = (ushort)(address[i * 2] * 256 + address[i * 2 + 1]);
		}
	}

	internal IPAddress(int newAddress)
	{
		m_Address = newAddress & 0xFFFFFFFFu;
	}

	public static bool TryParse(string ipString, out IPAddress address)
	{
		address = InternalParse(ipString, tryParse: true);
		return address != null;
	}

	public static IPAddress Parse(string ipString)
	{
		return InternalParse(ipString, tryParse: false);
	}

	private unsafe static IPAddress InternalParse(string ipString, bool tryParse)
	{
		if (ipString == null)
		{
			if (tryParse)
			{
				return null;
			}
			throw new ArgumentNullException("ipString");
		}
		if (ipString.IndexOf(':') != -1)
		{
			SocketException ex = null;
			int start = 0;
			if (ipString[0] != '[')
			{
				ipString += "]";
			}
			else
			{
				start = 1;
			}
			int end = ipString.Length;
			fixed (char* name = ipString)
			{
				if (IPv6AddressHelper.IsValidStrict(name, start, ref end) || end != ipString.Length)
				{
					ushort[] array = new ushort[8];
					string scopeId = null;
					fixed (ushort* numbers = array)
					{
						IPv6AddressHelper.Parse(ipString, numbers, 0, ref scopeId);
					}
					if (scopeId == null || scopeId.Length == 0)
					{
						return new IPAddress(array, 0u);
					}
					scopeId = scopeId.Substring(1);
					if (uint.TryParse(scopeId, NumberStyles.None, null, out var result))
					{
						return new IPAddress(array, result);
					}
					return new IPAddress(array, 0u);
				}
			}
			if (tryParse)
			{
				return null;
			}
			ex = new SocketException(SocketError.InvalidArgument);
			throw new FormatException(global::SR.GetString("An invalid IP address was specified."), ex);
		}
		int end2 = ipString.Length;
		long num;
		fixed (char* name2 = ipString)
		{
			num = IPv4AddressHelper.ParseNonCanonical(name2, 0, ref end2, notImplicitFile: true);
		}
		if (num == -1 || end2 != ipString.Length)
		{
			if (tryParse)
			{
				return null;
			}
			throw new FormatException(global::SR.GetString("An invalid IP address was specified."));
		}
		num = ((num & 0xFF) << 24) | (((num & 0xFF00) << 8) | (((num & 0xFF0000) >> 8) | ((num & 0xFF000000u) >> 24)));
		return new IPAddress(num);
	}

	public byte[] GetAddressBytes()
	{
		byte[] array;
		if (m_Family != AddressFamily.InterNetworkV6)
		{
			array = new byte[4]
			{
				(byte)m_Address,
				(byte)(m_Address >> 8),
				(byte)(m_Address >> 16),
				(byte)(m_Address >> 24)
			};
		}
		else
		{
			array = new byte[16];
			int num = 0;
			for (int i = 0; i < 8; i++)
			{
				array[num++] = (byte)((uint)(m_Numbers[i] >> 8) & 0xFFu);
				array[num++] = (byte)(m_Numbers[i] & 0xFFu);
			}
		}
		return array;
	}

	public unsafe override string ToString()
	{
		if (m_ToString == null)
		{
			if (m_Family == AddressFamily.InterNetworkV6)
			{
				m_ToString = new IPv6AddressFormatter(m_Numbers, ScopeId).ToString();
			}
			else
			{
				int num = 15;
				char* ptr = stackalloc char[15];
				int num2 = (int)((m_Address >> 24) & 0xFF);
				do
				{
					ptr[--num] = (char)(48 + num2 % 10);
					num2 /= 10;
				}
				while (num2 > 0);
				ptr[--num] = '.';
				num2 = (int)((m_Address >> 16) & 0xFF);
				do
				{
					ptr[--num] = (char)(48 + num2 % 10);
					num2 /= 10;
				}
				while (num2 > 0);
				ptr[--num] = '.';
				num2 = (int)((m_Address >> 8) & 0xFF);
				do
				{
					ptr[--num] = (char)(48 + num2 % 10);
					num2 /= 10;
				}
				while (num2 > 0);
				ptr[--num] = '.';
				num2 = (int)(m_Address & 0xFF);
				do
				{
					ptr[--num] = (char)(48 + num2 % 10);
					num2 /= 10;
				}
				while (num2 > 0);
				m_ToString = new string(ptr, num, 15 - num);
			}
		}
		return m_ToString;
	}

	public static long HostToNetworkOrder(long host)
	{
		return ((HostToNetworkOrder((int)host) & 0xFFFFFFFFu) << 32) | (HostToNetworkOrder((int)(host >> 32)) & 0xFFFFFFFFu);
	}

	public static int HostToNetworkOrder(int host)
	{
		return ((HostToNetworkOrder((short)host) & 0xFFFF) << 16) | (HostToNetworkOrder((short)(host >> 16)) & 0xFFFF);
	}

	public static short HostToNetworkOrder(short host)
	{
		return (short)(((host & 0xFF) << 8) | ((host >> 8) & 0xFF));
	}

	public static long NetworkToHostOrder(long network)
	{
		return HostToNetworkOrder(network);
	}

	public static int NetworkToHostOrder(int network)
	{
		return HostToNetworkOrder(network);
	}

	public static short NetworkToHostOrder(short network)
	{
		return HostToNetworkOrder(network);
	}

	public static bool IsLoopback(IPAddress address)
	{
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (address.m_Family == AddressFamily.InterNetworkV6)
		{
			return address.Equals(IPv6Loopback);
		}
		return (address.m_Address & 0xFF) == (Loopback.m_Address & 0xFF);
	}

	internal bool Equals(object comparandObj, bool compareScopeId)
	{
		if (!(comparandObj is IPAddress iPAddress))
		{
			return false;
		}
		if (m_Family != iPAddress.m_Family)
		{
			return false;
		}
		if (m_Family == AddressFamily.InterNetworkV6)
		{
			for (int i = 0; i < 8; i++)
			{
				if (iPAddress.m_Numbers[i] != m_Numbers[i])
				{
					return false;
				}
			}
			if (iPAddress.m_ScopeId == m_ScopeId)
			{
				return true;
			}
			if (!compareScopeId)
			{
				return true;
			}
			return false;
		}
		return iPAddress.m_Address == m_Address;
	}

	public override bool Equals(object comparand)
	{
		return Equals(comparand, compareScopeId: true);
	}

	public override int GetHashCode()
	{
		if (m_Family == AddressFamily.InterNetworkV6)
		{
			if (m_HashCode == 0)
			{
				m_HashCode = StringComparer.InvariantCultureIgnoreCase.GetHashCode(ToString());
			}
			return m_HashCode;
		}
		return (int)m_Address;
	}

	internal IPAddress Snapshot()
	{
		return m_Family switch
		{
			AddressFamily.InterNetwork => new IPAddress(m_Address), 
			AddressFamily.InterNetworkV6 => new IPAddress(m_Numbers, (uint)m_ScopeId), 
			_ => throw new InternalException(), 
		};
	}

	public IPAddress MapToIPv6()
	{
		if (AddressFamily == AddressFamily.InterNetworkV6)
		{
			return this;
		}
		return new IPAddress(new ushort[8]
		{
			0,
			0,
			0,
			0,
			0,
			65535,
			(ushort)(((m_Address & 0xFF00) >> 8) | ((m_Address & 0xFF) << 8)),
			(ushort)(((m_Address & 0xFF000000u) >> 24) | ((m_Address & 0xFF0000) >> 8))
		}, 0u);
	}

	public IPAddress MapToIPv4()
	{
		if (AddressFamily == AddressFamily.InterNetwork)
		{
			return this;
		}
		return new IPAddress((uint)(((m_Numbers[6] & 0xFF00) >>> 8) | ((m_Numbers[6] & 0xFF) << 8) | ((((m_Numbers[7] & 0xFF00) >>> 8) | ((m_Numbers[7] & 0xFF) << 8)) << 16)));
	}
}
