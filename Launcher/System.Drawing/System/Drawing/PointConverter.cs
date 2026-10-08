using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class PointConverter : TypeConverter
{
	public PointConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
