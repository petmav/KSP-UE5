using System.ComponentModel;

namespace System.Data;

[Obsolete("DataSysDescriptionAttribute has been deprecated.  http://go.microsoft.com/fwlink/?linkid=14202", false)]
[AttributeUsage(AttributeTargets.All)]
public class DataSysDescriptionAttribute : DescriptionAttribute
{
	private bool _replaced;

	public override string Description
	{
		get
		{
			if (!_replaced)
			{
				_replaced = true;
				base.DescriptionValue = base.Description;
			}
			return base.Description;
		}
	}

	[Obsolete("DataSysDescriptionAttribute has been deprecated.  http://go.microsoft.com/fwlink/?linkid=14202", false)]
	public DataSysDescriptionAttribute(string description)
		: base(description)
	{
	}
}
