using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class ColorConverter : TypeConverter
{
	public ColorConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
