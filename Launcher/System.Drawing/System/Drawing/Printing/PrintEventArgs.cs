using System.ComponentModel;
using Unity;

namespace System.Drawing.Printing;

public class PrintEventArgs : CancelEventArgs
{
	public PrintAction PrintAction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PrintAction);
		}
	}

	public PrintEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
