using System.Collections;
using System.Collections.Generic;

namespace System.Net;

internal class WebConnectionGroup
{
	private class ConnectionState : IWebConnectionState
	{
		private bool busy;

		private DateTime idleSince;

		public WebConnection Connection { get; private set; }

		public WebConnectionGroup Group { get; private set; }

		public ServicePoint ServicePoint => Group.sPoint;

		public bool Busy => busy;

		public DateTime IdleSince => idleSince;

		public bool TrySetBusy()
		{
			lock (ServicePoint)
			{
				if (busy)
				{
					return false;
				}
				busy = true;
				idleSince = DateTime.UtcNow + TimeSpan.FromDays(3650.0);
				return true;
			}
		}

		public void SetIdle()
		{
			lock (ServicePoint)
			{
				busy = false;
				idleSince = DateTime.UtcNow;
			}
		}

		public ConnectionState(WebConnectionGroup group)
		{
			Group = group;
			idleSince = DateTime.UtcNow;
			Connection = new WebConnection(this, group.sPoint);
		}
	}

	private ServicePoint sPoint;

	private string name;

	private LinkedList<ConnectionState> connections;

	private Queue queue;

	private bool closing;

	public string Name => name;

	internal Queue Queue => queue;

	public event EventHandler ConnectionClosed;

	public WebConnectionGroup(ServicePoint sPoint, string name)
	{
		this.sPoint = sPoint;
		this.name = name;
		connections = new LinkedList<ConnectionState>();
		queue = new Queue();
	}

	private void OnConnectionClosed()
	{
		if (this.ConnectionClosed != null)
		{
			this.ConnectionClosed(this, null);
		}
	}

	public void Close()
	{
		List<WebConnection> list = null;
		lock (sPoint)
		{
			closing = true;
			LinkedListNode<ConnectionState> linkedListNode = connections.First;
			while (linkedListNode != null)
			{
				WebConnection connection = linkedListNode.Value.Connection;
				LinkedListNode<ConnectionState> node = linkedListNode;
				linkedListNode = linkedListNode.Next;
				if (list == null)
				{
					list = new List<WebConnection>();
				}
				list.Add(connection);
				connections.Remove(node);
			}
		}
		if (list == null)
		{
			return;
		}
		foreach (WebConnection item in list)
		{
			item.Close(sendNext: false);
			OnConnectionClosed();
		}
	}

	public WebConnection GetConnection(HttpWebRequest request, out bool created)
	{
		lock (sPoint)
		{
			return CreateOrReuseConnection(request, out created);
		}
	}

	private static void PrepareSharingNtlm(WebConnection cnc, HttpWebRequest request)
	{
		if (cnc.NtlmAuthenticated)
		{
			bool flag = false;
			NetworkCredential ntlmCredential = cnc.NtlmCredential;
			NetworkCredential networkCredential = ((request.Proxy == null || request.Proxy.IsBypassed(request.RequestUri)) ? request.Credentials : request.Proxy.Credentials)?.GetCredential(request.RequestUri, "NTLM");
			if (ntlmCredential == null || networkCredential == null || ntlmCredential.Domain != networkCredential.Domain || ntlmCredential.UserName != networkCredential.UserName || ntlmCredential.Password != networkCredential.Password)
			{
				flag = true;
			}
			if (!flag)
			{
				bool unsafeAuthenticatedConnectionSharing = request.UnsafeAuthenticatedConnectionSharing;
				bool unsafeAuthenticatedConnectionSharing2 = cnc.UnsafeAuthenticatedConnectionSharing;
				flag = !unsafeAuthenticatedConnectionSharing || unsafeAuthenticatedConnectionSharing != unsafeAuthenticatedConnectionSharing2;
			}
			if (flag)
			{
				cnc.Close(sendNext: false);
				cnc.ResetNtlm();
			}
		}
	}

	private ConnectionState FindIdleConnection()
	{
		foreach (ConnectionState connection in connections)
		{
			if (!connection.Busy)
			{
				connections.Remove(connection);
				connections.AddFirst(connection);
				return connection;
			}
		}
		return null;
	}

	private WebConnection CreateOrReuseConnection(HttpWebRequest request, out bool created)
	{
		ConnectionState connectionState = FindIdleConnection();
		if (connectionState != null)
		{
			created = false;
			PrepareSharingNtlm(connectionState.Connection, request);
			return connectionState.Connection;
		}
		if (sPoint.ConnectionLimit > connections.Count || connections.Count == 0)
		{
			created = true;
			connectionState = new ConnectionState(this);
			connections.AddFirst(connectionState);
			return connectionState.Connection;
		}
		created = false;
		connectionState = connections.Last.Value;
		connections.Remove(connectionState);
		connections.AddFirst(connectionState);
		return connectionState.Connection;
	}

	internal bool TryRecycle(TimeSpan maxIdleTime, ref DateTime idleSince)
	{
		DateTime utcNow = DateTime.UtcNow;
		bool result;
		while (true)
		{
			List<WebConnection> list = null;
			lock (sPoint)
			{
				if (closing)
				{
					idleSince = DateTime.MinValue;
					return true;
				}
				int num = 0;
				LinkedListNode<ConnectionState> linkedListNode = connections.First;
				while (linkedListNode != null)
				{
					ConnectionState value = linkedListNode.Value;
					LinkedListNode<ConnectionState> node = linkedListNode;
					linkedListNode = linkedListNode.Next;
					num++;
					if (value.Busy)
					{
						continue;
					}
					if (num <= sPoint.ConnectionLimit && utcNow - value.IdleSince < maxIdleTime)
					{
						if (value.IdleSince > idleSince)
						{
							idleSince = value.IdleSince;
						}
						continue;
					}
					if (list == null)
					{
						list = new List<WebConnection>();
					}
					list.Add(value.Connection);
					connections.Remove(node);
				}
				result = connections.Count == 0;
			}
			if (list == null)
			{
				break;
			}
			foreach (WebConnection item in list)
			{
				item.Close(sendNext: false);
			}
		}
		return result;
	}
}
