using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class SizeConverter : TypeConverter
{
	public SizeConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
