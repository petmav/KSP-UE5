using System.Xml.Schema;

namespace System.Xml.Serialization;

internal class AttributeAccessor : Accessor
{
	private bool isSpecial;

	private bool isList;

	internal bool IsSpecialXmlNamespace => isSpecial;

	internal bool IsList
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

	internal void CheckSpecial()
	{
		if (Name.LastIndexOf(':') >= 0)
		{
			if (!Name.StartsWith("xml:", StringComparison.Ordinal))
			{
				throw new InvalidOperationException(Res.GetString("Invalid name character in '{0}'.", Name));
			}
			Name = Name.Substring("xml:".Length);
			base.Namespace = "http://www.w3.org/XML/1998/namespace";
			isSpecial = true;
		}
		else if (base.Namespace == "http://www.w3.org/XML/1998/namespace")
		{
			isSpecial = true;
		}
		else
		{
			isSpecial = false;
		}
		if (isSpecial)
		{
			base.Form = XmlSchemaForm.Qualified;
		}
	}
}
