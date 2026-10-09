using Unity;

namespace System.IO.Ports;

public class SerialDataReceivedEventArgs : EventArgs
{
	private SerialData eventType;

	public SerialData EventType => eventType;

	internal SerialDataReceivedEventArgs(SerialData eventType)
	{
		this.eventType = eventType;
	}

	internal SerialDataReceivedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
