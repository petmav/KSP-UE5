using Unity;

namespace System.Diagnostics.Eventing.Reader;

public class EventLogQuery
{
	public bool ReverseDirection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogSession Session
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool TolerateQueryErrors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogQuery(string path, PathType pathType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogQuery(string path, PathType pathType, string query)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
