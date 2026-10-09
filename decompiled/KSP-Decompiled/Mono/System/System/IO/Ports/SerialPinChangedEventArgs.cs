using Unity;

namespace System.IO.Ports;

public class SerialPinChangedEventArgs : EventArgs
{
	private SerialPinChange eventType;

	public SerialPinChange EventType => eventType;

	internal SerialPinChangedEventArgs(SerialPinChange eventType)
	{
		this.eventType = eventType;
	}

	internal SerialPinChangedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
