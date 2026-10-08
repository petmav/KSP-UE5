using System.Reflection;

namespace KSP.UI.Screens.Settings.Controls
{
	public class FieldAccessor : AccessorBase
	{
		public FieldInfo field => member as FieldInfo;

		public override object Value
		{
			get
			{
				return ((FieldInfo)member).GetValue(obj);
			}
			set
			{
				((FieldInfo)member).SetValue(obj, value);
			}
		}

		public FieldAccessor(object obj, MemberInfo member)
			: base(obj, member)
		{
		}
	}
}
