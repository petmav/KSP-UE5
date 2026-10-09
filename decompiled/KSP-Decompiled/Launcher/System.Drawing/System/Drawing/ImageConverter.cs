using System.ComponentModel;
using Unity;

namespace System.Drawing;

public class ImageConverter : TypeConverter
{
	public ImageConverter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
