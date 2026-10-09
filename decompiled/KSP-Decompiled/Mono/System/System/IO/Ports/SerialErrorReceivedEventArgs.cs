using Unity;

namespace System.IO.Ports;

public class SerialErrorReceivedEventArgs : EventArgs
{
	private SerialError eventType;

	public SerialError EventType => eventType;

	internal SerialErrorReceivedEventArgs(SerialError eventType)
	{
		this.eventType = eventType;
	}

	internal SerialErrorReceivedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
