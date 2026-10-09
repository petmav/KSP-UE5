namespace System.Xml.Serialization;

internal class PrimitiveMapping : TypeMapping
{
	private bool isList;

	internal override bool IsList
	{
		get
		{
			return isList;
		}
		set
		{
			isList = value;
		}
	}
}
