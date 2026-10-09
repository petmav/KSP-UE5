using System.Collections.Generic;
using System.Diagnostics;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal sealed class CompilerScopeManager<V>
{
	public enum ScopeFlags
	{
		BackwardCompatibility = 1,
		ForwardCompatibility = 2,
		CanHaveApplyImports = 4,
		NsDecl = 16,
		NsExcl = 32,
		Variable = 64,
		CompatibilityFlags = 3,
		InheritedFlags = 7,
		ExclusiveFlags = 112
	}

	public struct ScopeRecord
	{
		public int scopeCount;

		public ScopeFlags flags;

		public string ncName;

		public string nsUri;

		public V value;

		public bool IsVariable => (flags & ScopeFlags.Variable) != 0;

		public bool IsNamespace => (flags & ScopeFlags.NsDecl) != 0;
	}

	internal struct NamespaceEnumerator
	{
		private CompilerScopeManager<V> scope;

		private int lastRecord;

		private int currentRecord;

		public ScopeRecord Current => scope.records[currentRecord];

		public NamespaceEnumerator(CompilerScopeManager<V> scope)
		{
			this.scope = scope;
			lastRecord = scope.lastRecord;
			currentRecord = lastRecord + 1;
		}

		public void Reset()
		{
			currentRecord = lastRecord + 1;
		}

		public bool MoveNext()
		{
			while (0 < --currentRecord)
			{
				if (scope.records[currentRecord].IsNamespace && scope.LookupNamespace(scope.records[currentRecord].ncName, lastRecord, currentRecord + 1) == null)
				{
					return true;
				}
			}
			return false;
		}
	}

	private const int LastPredefRecord = 0;

	private ScopeRecord[] records = new ScopeRecord[32];

	private int lastRecord;

	private int lastScopes;

	public bool ForwardCompatibility
	{
		get
		{
			return (records[lastRecord].flags & ScopeFlags.ForwardCompatibility) != 0;
		}
		set
		{
			SetFlag(ScopeFlags.ForwardCompatibility, value);
		}
	}

	public bool BackwardCompatibility
	{
		get
		{
			return (records[lastRecord].flags & ScopeFlags.BackwardCompatibility) != 0;
		}
		set
		{
			SetFlag(ScopeFlags.BackwardCompatibility, value);
		}
	}

	public bool CanHaveApplyImports
	{
		get
		{
			return (records[lastRecord].flags & ScopeFlags.CanHaveApplyImports) != 0;
		}
		set
		{
			SetFlag(ScopeFlags.CanHaveApplyImports, value);
		}
	}

	public CompilerScopeManager()
	{
		records[0].flags = ScopeFlags.NsDecl;
		records[0].ncName = "xml";
		records[0].nsUri = "http://www.w3.org/XML/1998/namespace";
	}

	public CompilerScopeManager(KeywordsTable atoms)
	{
		records[0].flags = ScopeFlags.NsDecl;
		records[0].ncName = atoms.Xml;
		records[0].nsUri = atoms.UriXml;
	}

	public void EnterScope()
	{
		lastScopes++;
	}

	public void ExitScope()
	{
		if (0 < lastScopes)
		{
			lastScopes--;
			return;
		}
		while (records[--lastRecord].scopeCount == 0)
		{
		}
		lastScopes = records[lastRecord].scopeCount;
		lastScopes--;
	}

	[Conditional("DEBUG")]
	public void CheckEmpty()
	{
		ExitScope();
	}

	public bool EnterScope(NsDecl nsDecl)
	{
		lastScopes++;
		bool result = false;
		bool flag = false;
		while (nsDecl != null)
		{
			if (nsDecl.NsUri == null)
			{
				flag = true;
			}
			else if (nsDecl.Prefix == null)
			{
				AddExNamespace(nsDecl.NsUri);
			}
			else
			{
				result = true;
				AddNsDeclaration(nsDecl.Prefix, nsDecl.NsUri);
			}
			nsDecl = nsDecl.Prev;
		}
		if (flag)
		{
			AddExNamespace(null);
		}
		return result;
	}

	private void AddRecord()
	{
		records[lastRecord].scopeCount = lastScopes;
		if (++lastRecord == records.Length)
		{
			ScopeRecord[] destinationArray = new ScopeRecord[lastRecord * 2];
			Array.Copy(records, 0, destinationArray, 0, lastRecord);
			records = destinationArray;
		}
		lastScopes = 0;
	}

	private void AddRecord(ScopeFlags flag, string ncName, string uri, V value)
	{
		ScopeFlags scopeFlags = records[lastRecord].flags;
		if (lastScopes != 0 || (scopeFlags & ScopeFlags.ExclusiveFlags) != 0)
		{
			AddRecord();
			scopeFlags &= ScopeFlags.InheritedFlags;
		}
		records[lastRecord].flags = scopeFlags | flag;
		records[lastRecord].ncName = ncName;
		records[lastRecord].nsUri = uri;
		records[lastRecord].value = value;
	}

	private void SetFlag(ScopeFlags flag, bool value)
	{
		ScopeFlags scopeFlags = records[lastRecord].flags;
		if ((scopeFlags & flag) != 0 == value)
		{
			return;
		}
		if (lastScopes != 0)
		{
			AddRecord();
			scopeFlags &= ScopeFlags.InheritedFlags;
		}
		if (flag == ScopeFlags.CanHaveApplyImports)
		{
			scopeFlags ^= flag;
		}
		else
		{
			scopeFlags &= (ScopeFlags)(-4);
			if (value)
			{
				scopeFlags |= flag;
			}
		}
		records[lastRecord].flags = scopeFlags;
	}

	public void AddVariable(QilName varName, V value)
	{
		AddRecord(ScopeFlags.Variable, varName.LocalName, varName.NamespaceUri, value);
	}

	private string LookupNamespace(string prefix, int from, int to)
	{
		int num = from;
		while (to <= num)
		{
			if ((GetName(ref records[num], out var prefix2, out var nsUri) & ScopeFlags.NsDecl) != 0 && prefix2 == prefix)
			{
				return nsUri;
			}
			num--;
		}
		return null;
	}

	public string LookupNamespace(string prefix)
	{
		return LookupNamespace(prefix, lastRecord, 0);
	}

	private static ScopeFlags GetName(ref ScopeRecord re, out string prefix, out string nsUri)
	{
		prefix = re.ncName;
		nsUri = re.nsUri;
		return re.flags;
	}

	public void AddNsDeclaration(string prefix, string nsUri)
	{
		AddRecord(ScopeFlags.NsDecl, prefix, nsUri, default(V));
	}

	public void AddExNamespace(string nsUri)
	{
		AddRecord(ScopeFlags.NsExcl, null, nsUri, default(V));
	}

	public bool IsExNamespace(string nsUri)
	{
		int num = 0;
		int num2 = lastRecord;
		while (0 <= num2)
		{
			string prefix;
			string nsUri2;
			ScopeFlags name = GetName(ref records[num2], out prefix, out nsUri2);
			if ((name & ScopeFlags.NsExcl) != 0)
			{
				if (nsUri2 == nsUri)
				{
					return true;
				}
				if (nsUri2 == null)
				{
					num = num2;
				}
			}
			else if (num != 0 && (name & ScopeFlags.NsDecl) != 0 && nsUri2 == nsUri)
			{
				bool flag = false;
				for (int i = num2 + 1; i < num; i++)
				{
					GetName(ref records[i], out var prefix2, out var _);
					if ((name & ScopeFlags.NsDecl) != 0 && prefix2 == prefix)
					{
						flag = true;
						break;
					}
				}
				if (!flag)
				{
					return true;
				}
			}
			num2--;
		}
		return false;
	}

	private int SearchVariable(string localName, string uri)
	{
		int num = lastRecord;
		while (0 <= num)
		{
			if ((GetName(ref records[num], out var prefix, out var nsUri) & ScopeFlags.Variable) != 0 && prefix == localName && nsUri == uri)
			{
				return num;
			}
			num--;
		}
		return -1;
	}

	public V LookupVariable(string localName, string uri)
	{
		int num = SearchVariable(localName, uri);
		if (num >= 0)
		{
			return records[num].value;
		}
		return default(V);
	}

	public bool IsLocalVariable(string localName, string uri)
	{
		int num = SearchVariable(localName, uri);
		while (0 <= --num)
		{
			if (records[num].scopeCount != 0)
			{
				return true;
			}
		}
		return false;
	}

	internal IEnumerable<ScopeRecord> GetActiveRecords()
	{
		int currentRecord = lastRecord + 1;
		while (true)
		{
			int num = currentRecord - 1;
			currentRecord = num;
			if (0 < num)
			{
				if (!records[currentRecord].IsNamespace || LookupNamespace(records[currentRecord].ncName, lastRecord, currentRecord + 1) == null)
				{
					yield return records[currentRecord];
				}
				continue;
			}
			break;
		}
	}

	public NamespaceEnumerator GetEnumerator()
	{
		return new NamespaceEnumerator(this);
	}
}
