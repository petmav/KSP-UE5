using System.ComponentModel;
using System.Globalization;
using System.IO;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XmlCollation
{
	private struct Options
	{
		public const int FlagUpperFirst = 4096;

		public const int FlagEmptyGreatest = 8192;

		public const int FlagDescendingOrder = 16384;

		private const int Mask = 28672;

		private int value;

		public bool UpperFirst
		{
			get
			{
				return GetFlag(4096);
			}
			set
			{
				SetFlag(4096, value);
			}
		}

		public bool EmptyGreatest => GetFlag(8192);

		public bool DescendingOrder => GetFlag(16384);

		public bool IgnoreCase => GetFlag(1);

		public bool Ordinal => GetFlag(1073741824);

		public CompareOptions CompareOptions
		{
			get
			{
				return (CompareOptions)(value & -28673);
			}
			set
			{
				this.value = (this.value & 0x7000) | (int)value;
			}
		}

		public Options(int value)
		{
			this.value = value;
		}

		public bool GetFlag(int flag)
		{
			return (value & flag) != 0;
		}

		public void SetFlag(int flag, bool value)
		{
			if (value)
			{
				this.value |= flag;
			}
			else
			{
				this.value &= ~flag;
			}
		}

		public static implicit operator int(Options options)
		{
			return options.value;
		}
	}

	private const int deDE = 1031;

	private const int huHU = 1038;

	private const int jaJP = 1041;

	private const int kaGE = 1079;

	private const int koKR = 1042;

	private const int zhTW = 1028;

	private const int zhCN = 2052;

	private const int zhHK = 3076;

	private const int zhSG = 4100;

	private const int zhMO = 5124;

	private const int zhTWbopo = 197636;

	private const int deDEphon = 66567;

	private const int huHUtech = 66574;

	private const int kaGEmode = 66615;

	private CultureInfo cultInfo;

	private Options options;

	private CompareOptions compops;

	private static XmlCollation cp = new XmlCollation(CultureInfo.InvariantCulture, new Options(1073741824));

	private const int LOCALE_CURRENT = -1;

	internal static XmlCollation CodePointCollation => cp;

	internal bool UpperFirst => options.UpperFirst;

	internal bool EmptyGreatest => options.EmptyGreatest;

	internal bool DescendingOrder => options.DescendingOrder;

	internal CultureInfo Culture
	{
		get
		{
			if (cultInfo == null)
			{
				return CultureInfo.CurrentCulture;
			}
			return cultInfo;
		}
	}

	private XmlCollation(CultureInfo cultureInfo, Options options)
	{
		cultInfo = cultureInfo;
		this.options = options;
		compops = options.CompareOptions;
	}

	internal static XmlCollation Create(string collationLiteral)
	{
		return Create(collationLiteral, throwOnError: true);
	}

	internal static XmlCollation Create(string collationLiteral, bool throwOnError)
	{
		if (collationLiteral == "http://www.w3.org/2004/10/xpath-functions/collation/codepoint")
		{
			return CodePointCollation;
		}
		CultureInfo cultureInfo = null;
		Options options = default(Options);
		Uri result;
		if (throwOnError)
		{
			result = new Uri(collationLiteral);
		}
		else if (!Uri.TryCreate(collationLiteral, UriKind.Absolute, out result))
		{
			return null;
		}
		if (result.GetLeftPart(UriPartial.Authority) == "http://collations.microsoft.com")
		{
			string text = result.LocalPath.Substring(1);
			if (text.Length != 0)
			{
				try
				{
					cultureInfo = new CultureInfo(text);
				}
				catch (ArgumentException)
				{
					if (!throwOnError)
					{
						return null;
					}
					throw new XslTransformException("Collation language '{0}' is not supported.", text);
				}
			}
		}
		else
		{
			if (!result.IsBaseOf(new Uri("http://www.w3.org/2004/10/xpath-functions/collation/codepoint")))
			{
				if (!throwOnError)
				{
					return null;
				}
				throw new XslTransformException("The collation '{0}' is not supported.", collationLiteral);
			}
			options.CompareOptions = CompareOptions.Ordinal;
		}
		string query = result.Query;
		string text2 = null;
		if (query.Length != 0)
		{
			string[] array = query.Substring(1).Split('&');
			foreach (string text3 in array)
			{
				string[] array2 = text3.Split('=');
				if (array2.Length != 2)
				{
					if (!throwOnError)
					{
						return null;
					}
					throw new XslTransformException("Collation option '{0}' is invalid. Options must have the following format: <option-name>=<option-value>.", text3);
				}
				string text4 = array2[0].ToUpper(CultureInfo.InvariantCulture);
				string text5 = array2[1].ToUpper(CultureInfo.InvariantCulture);
				if (text4 == "SORT")
				{
					text2 = text5;
					continue;
				}
				int flag;
				switch (text4)
				{
				case "IGNORECASE":
					flag = 1;
					break;
				case "IGNORENONSPACE":
					flag = 2;
					break;
				case "IGNORESYMBOLS":
					flag = 4;
					break;
				case "IGNOREKANATYPE":
					flag = 8;
					break;
				case "IGNOREWIDTH":
					flag = 16;
					break;
				case "UPPERFIRST":
					flag = 4096;
					break;
				case "EMPTYGREATEST":
					flag = 8192;
					break;
				case "DESCENDINGORDER":
					flag = 16384;
					break;
				default:
					if (!throwOnError)
					{
						return null;
					}
					throw new XslTransformException("Unsupported option '{0}' in collation.", array2[0]);
				}
				switch (text5)
				{
				case "0":
				case "FALSE":
					options.SetFlag(flag, value: false);
					continue;
				case "1":
				case "TRUE":
					options.SetFlag(flag, value: true);
					continue;
				}
				if (!throwOnError)
				{
					return null;
				}
				throw new XslTransformException("Collation option '{0}' cannot have the value '{1}'.", array2[0], array2[1]);
			}
		}
		if (options.UpperFirst && options.IgnoreCase)
		{
			options.UpperFirst = false;
		}
		if (options.Ordinal)
		{
			options.CompareOptions = CompareOptions.Ordinal;
			options.UpperFirst = false;
		}
		if (text2 != null && cultureInfo != null)
		{
			int langID = GetLangID(cultureInfo.LCID);
			switch (text2)
			{
			case "bopo":
				if (langID == 1028)
				{
					cultureInfo = new CultureInfo(197636);
				}
				break;
			case "strk":
				if (langID == 2052 || langID == 3076 || langID == 4100 || langID == 5124)
				{
					cultureInfo = new CultureInfo(MakeLCID(cultureInfo.LCID, 2));
				}
				break;
			case "uni":
				if (langID == 1041 || langID == 1042)
				{
					cultureInfo = new CultureInfo(MakeLCID(cultureInfo.LCID, 1));
				}
				break;
			case "phn":
				if (langID == 1031)
				{
					cultureInfo = new CultureInfo(66567);
				}
				break;
			case "tech":
				if (langID == 1038)
				{
					cultureInfo = new CultureInfo(66574);
				}
				break;
			case "mod":
				if (langID == 1079)
				{
					cultureInfo = new CultureInfo(66615);
				}
				break;
			default:
				if (!throwOnError)
				{
					return null;
				}
				throw new XslTransformException("Unsupported sort option '{0}' in collation.", text2);
			case "pron":
			case "dict":
			case "trad":
				break;
			}
		}
		return new XmlCollation(cultureInfo, options);
	}

	public override bool Equals(object obj)
	{
		if (this == obj)
		{
			return true;
		}
		if (obj is XmlCollation xmlCollation && (int)options == (int)xmlCollation.options)
		{
			return object.Equals(cultInfo, xmlCollation.cultInfo);
		}
		return false;
	}

	public override int GetHashCode()
	{
		int num = options;
		if (cultInfo != null)
		{
			num ^= cultInfo.GetHashCode();
		}
		return num;
	}

	internal void GetObjectData(BinaryWriter writer)
	{
		writer.Write((cultInfo != null) ? cultInfo.LCID : (-1));
		writer.Write(options);
	}

	internal XmlCollation(BinaryReader reader)
	{
		int num = reader.ReadInt32();
		cultInfo = ((num != -1) ? new CultureInfo(num) : null);
		options = new Options(reader.ReadInt32());
		compops = options.CompareOptions;
	}

	internal XmlSortKey CreateSortKey(string s)
	{
		SortKey sortKey = Culture.CompareInfo.GetSortKey(s, compops);
		if (!UpperFirst)
		{
			return new XmlStringSortKey(sortKey, DescendingOrder);
		}
		byte[] keyData = sortKey.KeyData;
		if (UpperFirst && keyData.Length != 0)
		{
			int i;
			for (i = 0; keyData[i] != 1; i++)
			{
			}
			do
			{
				i++;
			}
			while (keyData[i] != 1);
			do
			{
				i++;
				keyData[i] ^= byte.MaxValue;
			}
			while (keyData[i] != 254);
		}
		return new XmlStringSortKey(keyData, DescendingOrder);
	}

	private static int MakeLCID(int langid, int sortid)
	{
		return (langid & 0xFFFF) | ((sortid & 0xF) << 16);
	}

	private static int GetLangID(int lcid)
	{
		return lcid & 0xFFFF;
	}
}
