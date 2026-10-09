namespace System.Xml.Xsl.Runtime;

internal class WhitespaceRuleReader : XmlWrappingReader
{
	private WhitespaceRuleLookup wsRules;

	private BitStack stkStrip;

	private bool shouldStrip;

	private bool preserveAdjacent;

	private string val;

	private XmlCharType xmlCharType = XmlCharType.Instance;

	public override string Value
	{
		get
		{
			if (val != null)
			{
				return val;
			}
			return base.Value;
		}
	}

	public static XmlReader CreateReader(XmlReader baseReader, WhitespaceRuleLookup wsRules)
	{
		if (wsRules == null)
		{
			return baseReader;
		}
		XmlReaderSettings settings = baseReader.Settings;
		if (settings != null)
		{
			if (settings.IgnoreWhitespace)
			{
				return baseReader;
			}
		}
		else
		{
			if (baseReader is XmlTextReader { WhitespaceHandling: WhitespaceHandling.None })
			{
				return baseReader;
			}
			if (baseReader is XmlTextReaderImpl { WhitespaceHandling: WhitespaceHandling.None })
			{
				return baseReader;
			}
		}
		return new WhitespaceRuleReader(baseReader, wsRules);
	}

	private WhitespaceRuleReader(XmlReader baseReader, WhitespaceRuleLookup wsRules)
		: base(baseReader)
	{
		val = null;
		stkStrip = new BitStack();
		shouldStrip = false;
		preserveAdjacent = false;
		this.wsRules = wsRules;
		this.wsRules.Atomize(baseReader.NameTable);
	}

	public override bool Read()
	{
		XmlCharType instance = XmlCharType.Instance;
		string text = null;
		val = null;
		while (base.Read())
		{
			switch (base.NodeType)
			{
			case XmlNodeType.Element:
				if (!base.IsEmptyElement)
				{
					stkStrip.PushBit(shouldStrip);
					shouldStrip = wsRules.ShouldStripSpace(base.LocalName, base.NamespaceURI) && base.XmlSpace != XmlSpace.Preserve;
				}
				break;
			case XmlNodeType.EndElement:
				shouldStrip = stkStrip.PopBit();
				break;
			case XmlNodeType.Text:
			case XmlNodeType.CDATA:
				if (preserveAdjacent)
				{
					return true;
				}
				if (!shouldStrip)
				{
					break;
				}
				if (!instance.IsOnlyWhitespace(base.Value))
				{
					if (text != null)
					{
						val = text + base.Value;
					}
					preserveAdjacent = true;
					return true;
				}
				goto case XmlNodeType.Whitespace;
			case XmlNodeType.Whitespace:
			case XmlNodeType.SignificantWhitespace:
				if (preserveAdjacent)
				{
					return true;
				}
				if (shouldStrip)
				{
					text = ((text != null) ? (text + base.Value) : base.Value);
					continue;
				}
				break;
			case XmlNodeType.EndEntity:
				continue;
			}
			preserveAdjacent = false;
			return true;
		}
		return false;
	}
}
