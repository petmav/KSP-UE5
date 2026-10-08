using System.Runtime.Serialization;
using Unity;

namespace System.Drawing.Printing;

[Serializable]
public class InvalidPrinterException : SystemException
{
	public InvalidPrinterException(PrinterSettings settings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected InvalidPrinterException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
