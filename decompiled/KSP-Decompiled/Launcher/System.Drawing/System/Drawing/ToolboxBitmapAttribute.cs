using Unity;

namespace System.Drawing;

[AttributeUsage(AttributeTargets.Class)]
public class ToolboxBitmapAttribute : Attribute
{
	public static readonly ToolboxBitmapAttribute Default;

	public ToolboxBitmapAttribute(string imageFile)
	{
	}

	public ToolboxBitmapAttribute(Type t)
	{
	}

	public ToolboxBitmapAttribute(Type t, string name)
	{
	}

	public Image GetImage(object component)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Image GetImage(object component, bool large)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Image GetImage(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Image GetImage(Type type, bool large)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Image GetImage(Type type, string imgName, bool large)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Image GetImageFromResource(Type t, string imageName, bool large)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
