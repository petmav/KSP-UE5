using System.ComponentModel;
using Unity;

namespace System.Net.NetworkInformation;

public class PingCompletedEventArgs : AsyncCompletedEventArgs
{
	private PingReply reply;

	public PingReply Reply => reply;

	internal PingCompletedEventArgs(Exception ex, bool cancelled, object userState, PingReply reply)
		: base(ex, cancelled, userState)
	{
		this.reply = reply;
	}

	internal PingCompletedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
