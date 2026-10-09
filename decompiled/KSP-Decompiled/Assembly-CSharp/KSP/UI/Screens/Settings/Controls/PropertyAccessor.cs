using System.Reflection;

namespace KSP.UI.Screens.Settings.Controls
{
	public class PropertyAccessor : AccessorBase
	{
		protected object[] indexParam;

		public override object Value
		{
			get
			{
				return ((PropertyInfo)member).GetValue(obj, indexParam);
			}
			set
			{
				((PropertyInfo)member).SetValue(obj, value, indexParam);
			}
		}

		public PropertyAccessor(object obj, MemberInfo member, object[] indexParam)
			: base(obj, member)
		{
			this.indexParam = indexParam;
		}
	}
}
