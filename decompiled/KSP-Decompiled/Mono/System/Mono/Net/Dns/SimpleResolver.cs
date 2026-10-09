using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace Mono.Net.Dns;

internal sealed class SimpleResolver : IDisposable
{
	private static string[] EmptyStrings = new string[0];

	private static IPAddress[] EmptyAddresses = new IPAddress[0];

	private IPEndPoint[] endpoints;

	private Socket client;

	private Dictionary<int, SimpleResolverEventArgs> queries;

	private AsyncCallback receive_cb;

	private TimerCallback timeout_cb;

	private bool disposed;

	public SimpleResolver()
	{
		queries = new Dictionary<int, SimpleResolverEventArgs>();
		receive_cb = OnReceive;
		timeout_cb = OnTimeout;
		InitFromSystem();
		InitSocket();
	}

	void IDisposable.Dispose()
	{
		if (!disposed)
		{
			disposed = true;
			if (client != null)
			{
				client.Close();
				client = null;
			}
		}
	}

	public void Close()
	{
		((IDisposable)this).Dispose();
	}

	private void GetLocalHost(SimpleResolverEventArgs args)
	{
		IPHostEntry iPHostEntry = new IPHostEntry();
		iPHostEntry.HostName = "localhost";
		iPHostEntry.AddressList = new IPAddress[1] { IPAddress.Loopback };
		iPHostEntry.Aliases = EmptyStrings;
		args.ResolverError = ResolverError.NoError;
		args.HostEntry = iPHostEntry;
	}

	public bool GetHostAddressesAsync(SimpleResolverEventArgs args)
	{
		if (args == null)
		{
			throw new ArgumentNullException("args");
		}
		if (args.HostName == null)
		{
			throw new ArgumentNullException("args.HostName is null");
		}
		if (args.HostName.Length > 255)
		{
			throw new ArgumentException("args.HostName is too long");
		}
		args.Reset(ResolverAsyncOperation.GetHostAddresses);
		string hostName = args.HostName;
		if (hostName == "")
		{
			GetLocalHost(args);
			return false;
		}
		if (IPAddress.TryParse(hostName, out var address))
		{
			IPHostEntry iPHostEntry = new IPHostEntry();
			iPHostEntry.HostName = hostName;
			iPHostEntry.Aliases = EmptyStrings;
			iPHostEntry.AddressList = new IPAddress[1] { address };
			args.HostEntry = iPHostEntry;
			return false;
		}
		SendAQuery(args, add_it: true);
		return true;
	}

	public bool GetHostEntryAsync(SimpleResolverEventArgs args)
	{
		if (args == null)
		{
			throw new ArgumentNullException("args");
		}
		if (args.HostName == null)
		{
			throw new ArgumentNullException("args.HostName is null");
		}
		if (args.HostName.Length > 255)
		{
			throw new ArgumentException("args.HostName is too long");
		}
		args.Reset(ResolverAsyncOperation.GetHostEntry);
		string hostName = args.HostName;
		if (hostName == "")
		{
			GetLocalHost(args);
			return false;
		}
		if (IPAddress.TryParse(hostName, out var address))
		{
			IPHostEntry iPHostEntry = new IPHostEntry();
			iPHostEntry.HostName = hostName;
			iPHostEntry.Aliases = EmptyStrings;
			iPHostEntry.AddressList = new IPAddress[1] { address };
			args.HostEntry = iPHostEntry;
			args.PTRAddress = address;
			SendPTRQuery(args, add_it: true);
			return true;
		}
		SendAQuery(args, add_it: true);
		return true;
	}

	private bool AddQuery(DnsQuery query, SimpleResolverEventArgs args)
	{
		lock (queries)
		{
			if (queries.ContainsKey(query.Header.ID))
			{
				return false;
			}
			queries[query.Header.ID] = args;
		}
		return true;
	}

	private static DnsQuery GetQuery(string host, DnsQType q, DnsQClass c)
	{
		return new DnsQuery(host, q, c);
	}

	private void SendAQuery(SimpleResolverEventArgs args, bool add_it)
	{
		SendAQuery(args, args.HostName, add_it);
	}

	private void SendAQuery(SimpleResolverEventArgs args, string host, bool add_it)
	{
		DnsQuery query = GetQuery(host, DnsQType.A, DnsQClass.Internet);
		SendQuery(args, query, add_it);
	}

	private static string GetPTRName(IPAddress address)
	{
		byte[] addressBytes = address.GetAddressBytes();
		StringBuilder stringBuilder = new StringBuilder(28);
		for (int num = addressBytes.Length - 1; num >= 0; num--)
		{
			stringBuilder.AppendFormat("{0}.", addressBytes[num]);
		}
		stringBuilder.Append("in-addr.arpa");
		return stringBuilder.ToString();
	}

	private void SendPTRQuery(SimpleResolverEventArgs args, bool add_it)
	{
		DnsQuery query = GetQuery(GetPTRName(args.PTRAddress), DnsQType.PTR, DnsQClass.Internet);
		SendQuery(args, query, add_it);
	}

	private void SendQuery(SimpleResolverEventArgs args, DnsQuery query, bool add_it)
	{
		int num = 0;
		if (add_it)
		{
			do
			{
				query.Header.ID = (ushort)new Random().Next(1, 65534);
				if (num > 500)
				{
					throw new InvalidOperationException("Too many pending queries (or really bad luck)");
				}
			}
			while (!AddQuery(query, args));
			args.QueryID = query.Header.ID;
		}
		else
		{
			query.Header.ID = args.QueryID;
		}
		if (args.Timer == null)
		{
			args.Timer = new Timer(timeout_cb, args, 5000, -1);
		}
		else
		{
			args.Timer.Change(5000, -1);
		}
		client.BeginSend(query.Packet, 0, query.Length, SocketFlags.None, null, null);
	}

	private byte[] GetFreshBuffer()
	{
		return new byte[512];
	}

	private void FreeBuffer(byte[] buffer)
	{
	}

	private void InitSocket()
	{
		client = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
		client.Blocking = true;
		client.Bind(new IPEndPoint(IPAddress.Any, 0));
		client.Connect(endpoints[0]);
		BeginReceive();
	}

	private void BeginReceive()
	{
		byte[] freshBuffer = GetFreshBuffer();
		client.BeginReceive(freshBuffer, 0, freshBuffer.Length, SocketFlags.None, receive_cb, freshBuffer);
	}

	private void OnTimeout(object obj)
	{
		SimpleResolverEventArgs simpleResolverEventArgs = (SimpleResolverEventArgs)obj;
		lock (queries)
		{
			if (queries.TryGetValue(simpleResolverEventArgs.QueryID, out var value))
			{
				if (simpleResolverEventArgs != value)
				{
					throw new Exception("Should not happen: args != args2");
				}
				simpleResolverEventArgs.Retries++;
				if (simpleResolverEventArgs.Retries > 1)
				{
					simpleResolverEventArgs.ResolverError = ResolverError.Timeout;
					simpleResolverEventArgs.OnCompleted(this);
				}
				else
				{
					SendAQuery(simpleResolverEventArgs, add_it: false);
				}
			}
		}
	}

	private void OnReceive(IAsyncResult ares)
	{
		if (disposed)
		{
			return;
		}
		int num = 0;
		EndPoint remoteEndPoint = client.RemoteEndPoint;
		try
		{
			num = client.EndReceive(ares);
		}
		catch (Exception value)
		{
			Console.Error.WriteLine(value);
		}
		BeginReceive();
		byte[] buffer = (byte[])ares.AsyncState;
		if (num > 12)
		{
			DnsResponse dnsResponse = new DnsResponse(buffer, num);
			int iD = dnsResponse.Header.ID;
			SimpleResolverEventArgs value2 = null;
			lock (queries)
			{
				if (queries.TryGetValue(iD, out value2))
				{
					queries.Remove(iD);
				}
			}
			if (value2 != null)
			{
				value2.Timer?.Change(-1, -1);
				try
				{
					ProcessResponse(value2, dnsResponse, remoteEndPoint);
				}
				catch (Exception ex)
				{
					value2.ResolverError = (ResolverError)(-1);
					value2.ErrorMessage = ex.Message;
				}
				IPHostEntry hostEntry = value2.HostEntry;
				if (value2.ResolverError != 0 && value2.PTRAddress != null && hostEntry != null && hostEntry.HostName != null)
				{
					value2.PTRAddress = null;
					SendAQuery(value2, hostEntry.HostName, add_it: true);
					value2.Timer.Change(5000, -1);
				}
				else
				{
					value2.OnCompleted(this);
				}
			}
		}
		FreeBuffer(buffer);
	}

	private void ProcessResponse(SimpleResolverEventArgs args, DnsResponse response, EndPoint server_ep)
	{
		DnsRCode rCode = response.Header.RCode;
		if (rCode != 0)
		{
			if (args.PTRAddress == null)
			{
				args.ResolverError = (ResolverError)rCode;
			}
			return;
		}
		if (((IPEndPoint)server_ep).Port != 53)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "Port";
			return;
		}
		DnsHeader header = response.Header;
		if (!header.IsQuery)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "IsQuery";
			return;
		}
		if (header.QuestionCount > 1)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "QuestionCount";
			return;
		}
		ReadOnlyCollection<DnsQuestion> questions = response.GetQuestions();
		if (questions.Count != 1)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "QuestionCount 2";
			return;
		}
		DnsQuestion dnsQuestion = questions[0];
		DnsQType type = dnsQuestion.Type;
		if (type != DnsQType.A && type != DnsQType.AAAA && type != DnsQType.PTR)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "QType " + dnsQuestion.Type;
			return;
		}
		if (dnsQuestion.Class != DnsQClass.Internet)
		{
			args.ResolverError = ResolverError.ResponseHeaderError;
			args.ErrorMessage = "QClass " + dnsQuestion.Class;
			return;
		}
		ReadOnlyCollection<DnsResourceRecord> answers = response.GetAnswers();
		if (answers.Count == 0)
		{
			if (args.PTRAddress == null)
			{
				args.ResolverError = ResolverError.NameError;
				args.ErrorMessage = "NoAnswers";
			}
			return;
		}
		List<string> list = null;
		List<IPAddress> list2 = null;
		foreach (DnsResourceRecord item in answers)
		{
			if (item.Class != DnsClass.Internet)
			{
				continue;
			}
			if (item.Type == DnsType.A || item.Type == DnsType.AAAA)
			{
				if (list2 == null)
				{
					list2 = new List<IPAddress>();
				}
				list2.Add(((DnsResourceRecordIPAddress)item).Address);
			}
			else if (item.Type == DnsType.CNAME)
			{
				if (list == null)
				{
					list = new List<string>();
				}
				list.Add(((DnsResourceRecordCName)item).CName);
			}
			else if (item.Type == DnsType.PTR)
			{
				args.HostEntry.HostName = ((DnsResourceRecordPTR)item).DName;
				args.HostEntry.Aliases = ((list == null) ? EmptyStrings : list.ToArray());
				args.HostEntry.AddressList = EmptyAddresses;
				return;
			}
		}
		IPHostEntry iPHostEntry = args.HostEntry ?? new IPHostEntry();
		if (iPHostEntry.HostName == null && list != null && list.Count > 0)
		{
			iPHostEntry.HostName = list[0];
			list.RemoveAt(0);
		}
		iPHostEntry.Aliases = ((list == null) ? EmptyStrings : list.ToArray());
		iPHostEntry.AddressList = ((list2 == null) ? EmptyAddresses : list2.ToArray());
		args.HostEntry = iPHostEntry;
		if ((dnsQuestion.Type == DnsQType.A || dnsQuestion.Type == DnsQType.AAAA) && iPHostEntry.AddressList == EmptyAddresses)
		{
			args.ResolverError = ResolverError.NameError;
			args.ErrorMessage = "No addresses in response";
		}
		else if (dnsQuestion.Type == DnsQType.PTR && iPHostEntry.HostName == null)
		{
			args.ResolverError = ResolverError.NameError;
			args.ErrorMessage = "No PTR in response";
		}
	}

	private void InitFromSystem()
	{
		List<IPEndPoint> list = new List<IPEndPoint>();
		NetworkInterface[] allNetworkInterfaces = NetworkInterface.GetAllNetworkInterfaces();
		foreach (NetworkInterface networkInterface in allNetworkInterfaces)
		{
			if (NetworkInterfaceType.Loopback == networkInterface.NetworkInterfaceType)
			{
				continue;
			}
			foreach (IPAddress dnsAddress in networkInterface.GetIPProperties().DnsAddresses)
			{
				if (AddressFamily.InterNetworkV6 != dnsAddress.AddressFamily)
				{
					IPEndPoint item = new IPEndPoint(dnsAddress, 53);
					if (!list.Contains(item))
					{
						list.Add(item);
					}
				}
			}
		}
		endpoints = list.ToArray();
	}
}
