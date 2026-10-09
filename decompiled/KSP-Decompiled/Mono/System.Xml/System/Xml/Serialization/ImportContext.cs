using System.Collections;
using System.Collections.Specialized;

namespace System.Xml.Serialization;

public class ImportContext
{
	private bool shareTypes;

	private SchemaObjectCache cache;

	private Hashtable mappings;

	private Hashtable elements;

	private CodeIdentifiers typeIdentifiers;

	internal SchemaObjectCache Cache
	{
		get
		{
			if (cache == null)
			{
				cache = new SchemaObjectCache();
			}
			return cache;
		}
	}

	internal Hashtable Elements
	{
		get
		{
			if (elements == null)
			{
				elements = new Hashtable();
			}
			return elements;
		}
	}

	internal Hashtable Mappings
	{
		get
		{
			if (mappings == null)
			{
				mappings = new Hashtable();
			}
			return mappings;
		}
	}

	public CodeIdentifiers TypeIdentifiers
	{
		get
		{
			if (typeIdentifiers == null)
			{
				typeIdentifiers = new CodeIdentifiers();
			}
			return typeIdentifiers;
		}
	}

	public bool ShareTypes => shareTypes;

	public StringCollection Warnings => Cache.Warnings;

	public ImportContext(CodeIdentifiers identifiers, bool shareTypes)
	{
		typeIdentifiers = identifiers;
		this.shareTypes = shareTypes;
	}

	internal ImportContext()
		: this(null, shareTypes: false)
	{
	}
}
