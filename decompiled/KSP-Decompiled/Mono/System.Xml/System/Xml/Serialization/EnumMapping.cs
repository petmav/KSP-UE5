namespace System.Xml.Serialization;

internal class EnumMapping : PrimitiveMapping
{
	private ConstantMapping[] constants;

	private bool isFlags;

	internal bool IsFlags
	{
		get
		{
			return isFlags;
		}
		set
		{
			isFlags = value;
		}
	}

	internal ConstantMapping[] Constants
	{
		get
		{
			return constants;
		}
		set
		{
			constants = value;
		}
	}
}
