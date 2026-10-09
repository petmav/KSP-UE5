using Unity;

namespace System.Drawing.Printing;

public class QueryPageSettingsEventArgs : PrintEventArgs
{
	public PageSettings PageSettings
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

	public QueryPageSettingsEventArgs(PageSettings pageSettings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
