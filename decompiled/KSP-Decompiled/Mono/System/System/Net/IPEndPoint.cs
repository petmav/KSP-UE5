using System.Globalization;
using System.Net.Sockets;

namespace System.Net;

[Serializable]
public class IPEndPoint : EndPoint
{
	public const int MinPort = 0;

	public const int MaxPort = 65535;

	private IPAddress m_Address;

	private int m_Port;

	internal const int AnyPort = 0;

	internal static IPEndPoint Any = new IPEndPoint(IPAddress.Any, 0);

	internal static IPEndPoint IPv6Any = new IPEndPoint(IPAddress.IPv6Any, 0);

	public override AddressFamily AddressFamily => m_Address.AddressFamily;

	public IPAddress Address
	{
		get
		{
			return m_Address;
		}
		set
		{
			m_Address = value;
		}
	}

	public int Port
	{
		get
		{
			return m_Port;
		}
		set
		{
			if (!ValidationHelper.ValidateTcpPort(value))
			{
				throw new ArgumentOutOfRangeException("value");
			}
			m_Port = value;
		}
	}

	public IPEndPoint(long address, int port)
	{
		if (!ValidationHelper.ValidateTcpPort(port))
		{
			throw new ArgumentOutOfRangeException("port");
		}
		m_Port = port;
		m_Address = new IPAddress(address);
	}

	public IPEndPoint(IPAddress address, int port)
	{
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (!ValidationHelper.ValidateTcpPort(port))
		{
			throw new ArgumentOutOfRangeException("port");
		}
		m_Port = port;
		m_Address = address;
	}

	public override string ToString()
	{
		string format = ((m_Address.AddressFamily != AddressFamily.InterNetworkV6) ? "{0}:{1}" : "[{0}]:{1}");
		return string.Format(format, m_Address.ToString(), Port.ToString(NumberFormatInfo.InvariantInfo));
	}

	public override SocketAddress Serialize()
	{
		return new SocketAddress(Address, Port);
	}

	public override EndPoint Create(SocketAddress socketAddress)
	{
		if (socketAddress.Family != AddressFamily)
		{
			throw new ArgumentException(global::SR.GetString("The AddressFamily {0} is not valid for the {1} end point, use {2} instead.", socketAddress.Family.ToString(), GetType().FullName, AddressFamily.ToString()), "socketAddress");
		}
		if (socketAddress.Size < 8)
		{
			throw new ArgumentException(global::SR.GetString("The supplied {0} is an invalid size for the {1} end point.", socketAddress.GetType().FullName, GetType().FullName), "socketAddress");
		}
		return socketAddress.GetIPEndPoint();
	}

	public override bool Equals(object comparand)
	{
		if (!(comparand is IPEndPoint))
		{
			return false;
		}
		if (((IPEndPoint)comparand).m_Address.Equals(m_Address))
		{
			return ((IPEndPoint)comparand).m_Port == m_Port;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return m_Address.GetHashCode() ^ m_Port;
	}

	internal IPEndPoint Snapshot()
	{
		return new IPEndPoint(Address.Snapshot(), Port);
	}
}
