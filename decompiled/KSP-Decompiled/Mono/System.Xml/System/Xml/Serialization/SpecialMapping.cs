namespace System.Xml.Serialization;

internal class SpecialMapping : TypeMapping
{
	private bool namedAny;

	internal bool NamedAny
	{
		get
		{
			return namedAny;
		}
		set
		{
			namedAny = value;
		}
	}
}
