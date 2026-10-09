namespace System.Net;

internal struct SecurityStatusPal
{
	public readonly SecurityStatusPalErrorCode ErrorCode;

	public readonly Exception Exception;

	public SecurityStatusPal(SecurityStatusPalErrorCode errorCode, Exception exception = null)
	{
		ErrorCode = errorCode;
		Exception = exception;
	}

	public override string ToString()
	{
		if (Exception != null)
		{
			return string.Format("{0}={1}, {2}={3}", "ErrorCode", ErrorCode, "Exception", Exception);
		}
		return string.Format("{0}={1}", "ErrorCode", ErrorCode);
	}
}
