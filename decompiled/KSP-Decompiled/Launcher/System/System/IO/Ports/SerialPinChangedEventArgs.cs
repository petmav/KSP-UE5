using Unity;

namespace System.IO.Ports;

public class SerialPinChangedEventArgs : EventArgs
{
	public SerialPinChange EventType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SerialPinChange);
		}
	}

	internal SerialPinChangedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
