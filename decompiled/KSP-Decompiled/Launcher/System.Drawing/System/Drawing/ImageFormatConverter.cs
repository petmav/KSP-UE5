using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class ImageFormatConverter : TypeConverter
{
	public ImageFormatConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
