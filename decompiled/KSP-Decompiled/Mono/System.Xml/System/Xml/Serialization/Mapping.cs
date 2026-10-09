namespace System.Xml.Serialization;

internal abstract class Mapping
{
	private bool isSoap;

	internal bool IsSoap
	{
		get
		{
			return isSoap;
		}
		set
		{
			isSoap = value;
		}
	}

	internal Mapping()
	{
	}

	protected Mapping(Mapping mapping)
	{
		isSoap = mapping.isSoap;
	}
}
