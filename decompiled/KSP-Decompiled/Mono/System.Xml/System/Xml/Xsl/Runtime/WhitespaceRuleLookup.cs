using System.Collections;
using System.Collections.Generic;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Runtime;

internal class WhitespaceRuleLookup
{
	private class InternalWhitespaceRule : WhitespaceRule
	{
		private int priority;

		private int hashCode;

		public int Priority => priority;

		public InternalWhitespaceRule()
		{
		}

		public InternalWhitespaceRule(string localName, string namespaceName, bool preserveSpace, int priority)
		{
			Init(localName, namespaceName, preserveSpace, priority);
		}

		public void Init(string localName, string namespaceName, bool preserveSpace, int priority)
		{
			Init(localName, namespaceName, preserveSpace);
			this.priority = priority;
			if (localName != null && namespaceName != null)
			{
				hashCode = localName.GetHashCode();
			}
		}

		public void Atomize(XmlNameTable nameTable)
		{
			if (base.LocalName != null)
			{
				base.LocalName = nameTable.Add(base.LocalName);
			}
			if (base.NamespaceName != null)
			{
				base.NamespaceName = nameTable.Add(base.NamespaceName);
			}
		}

		public override int GetHashCode()
		{
			return hashCode;
		}

		public override bool Equals(object obj)
		{
			InternalWhitespaceRule internalWhitespaceRule = obj as InternalWhitespaceRule;
			if (base.LocalName == internalWhitespaceRule.LocalName)
			{
				return base.NamespaceName == internalWhitespaceRule.NamespaceName;
			}
			return false;
		}
	}

	private Hashtable qnames;

	private ArrayList wildcards;

	private InternalWhitespaceRule ruleTemp;

	private XmlNameTable nameTable;

	public WhitespaceRuleLookup()
	{
		qnames = new Hashtable();
		wildcards = new ArrayList();
	}

	public WhitespaceRuleLookup(IList<WhitespaceRule> rules)
		: this()
	{
		for (int num = rules.Count - 1; num >= 0; num--)
		{
			WhitespaceRule whitespaceRule = rules[num];
			InternalWhitespaceRule internalWhitespaceRule = new InternalWhitespaceRule(whitespaceRule.LocalName, whitespaceRule.NamespaceName, whitespaceRule.PreserveSpace, -num);
			if (whitespaceRule.LocalName == null || whitespaceRule.NamespaceName == null)
			{
				wildcards.Add(internalWhitespaceRule);
			}
			else
			{
				qnames[internalWhitespaceRule] = internalWhitespaceRule;
			}
		}
		ruleTemp = new InternalWhitespaceRule();
	}

	public void Atomize(XmlNameTable nameTable)
	{
		if (nameTable == this.nameTable)
		{
			return;
		}
		this.nameTable = nameTable;
		foreach (InternalWhitespaceRule value in qnames.Values)
		{
			value.Atomize(nameTable);
		}
		foreach (InternalWhitespaceRule wildcard in wildcards)
		{
			wildcard.Atomize(nameTable);
		}
	}

	public bool ShouldStripSpace(string localName, string namespaceName)
	{
		ruleTemp.Init(localName, namespaceName, preserveSpace: false, 0);
		InternalWhitespaceRule internalWhitespaceRule = qnames[ruleTemp] as InternalWhitespaceRule;
		int count = wildcards.Count;
		while (count-- != 0)
		{
			InternalWhitespaceRule internalWhitespaceRule2 = wildcards[count] as InternalWhitespaceRule;
			if (internalWhitespaceRule != null)
			{
				if (internalWhitespaceRule.Priority > internalWhitespaceRule2.Priority)
				{
					return !internalWhitespaceRule.PreserveSpace;
				}
				if (internalWhitespaceRule.PreserveSpace == internalWhitespaceRule2.PreserveSpace)
				{
					continue;
				}
			}
			if ((internalWhitespaceRule2.LocalName == null || (object)internalWhitespaceRule2.LocalName == localName) && (internalWhitespaceRule2.NamespaceName == null || (object)internalWhitespaceRule2.NamespaceName == namespaceName))
			{
				return !internalWhitespaceRule2.PreserveSpace;
			}
		}
		if (internalWhitespaceRule != null)
		{
			return !internalWhitespaceRule.PreserveSpace;
		}
		return false;
	}
}
