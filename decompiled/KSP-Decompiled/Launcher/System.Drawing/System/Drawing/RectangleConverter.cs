using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class RectangleConverter : TypeConverter
{
	public RectangleConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
