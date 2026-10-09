using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class DataMisalignedException : SystemException
{
	public DataMisalignedException()
		: base(Environment.GetResourceString("A datatype misalignment was detected in a load or store instruction."))
	{
		SetErrorCode(-2146233023);
	}

	public DataMisalignedException(string message)
		: base(message)
	{
		SetErrorCode(-2146233023);
	}

	public DataMisalignedException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233023);
	}

	internal DataMisalignedException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}
