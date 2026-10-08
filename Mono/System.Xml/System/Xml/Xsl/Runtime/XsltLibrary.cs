using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Xml.XPath;
using System.Xml.Xsl.Xslt;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XsltLibrary
{
	internal enum ComparisonOperator
	{
		Eq,
		Ne,
		Lt,
		Le,
		Gt,
		Ge
	}

	private XmlQueryRuntime runtime;

	private HybridDictionary functionsAvail;

	private Dictionary<XmlQualifiedName, DecimalFormat> decimalFormats;

	private List<DecimalFormatter> decimalFormatters;

	internal const int InvariantCultureLcid = 127;

	internal XsltLibrary(XmlQueryRuntime runtime)
	{
		this.runtime = runtime;
	}

	public string FormatMessage(string res, IList<string> args)
	{
		string[] array = new string[args.Count];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = args[i];
		}
		return XslTransformException.CreateMessage(res, array);
	}

	public int CheckScriptNamespace(string nsUri)
	{
		if (runtime.ExternalContext.GetLateBoundObject(nsUri) != null)
		{
			throw new XslTransformException("Cannot have both an extension object and a script implementing the same namespace '{0}'.", nsUri);
		}
		return 0;
	}

	public bool ElementAvailable(XmlQualifiedName name)
	{
		return QilGenerator.IsElementAvailable(name);
	}

	public bool FunctionAvailable(XmlQualifiedName name)
	{
		if (functionsAvail == null)
		{
			functionsAvail = new HybridDictionary();
		}
		else
		{
			object obj = functionsAvail[name];
			if (obj != null)
			{
				return (bool)obj;
			}
		}
		bool flag = FunctionAvailableHelper(name);
		functionsAvail[name] = flag;
		return flag;
	}

	private bool FunctionAvailableHelper(XmlQualifiedName name)
	{
		if (QilGenerator.IsFunctionAvailable(name.Name, name.Namespace))
		{
			return true;
		}
		if (name.Namespace.Length == 0 || name.Namespace == "http://www.w3.org/1999/XSL/Transform")
		{
			return false;
		}
		if (runtime.ExternalContext.LateBoundFunctionExists(name.Name, name.Namespace))
		{
			return true;
		}
		return runtime.EarlyBoundFunctionExists(name.Name, name.Namespace);
	}

	public int RegisterDecimalFormat(XmlQualifiedName name, string infinitySymbol, string nanSymbol, string characters)
	{
		if (decimalFormats == null)
		{
			decimalFormats = new Dictionary<XmlQualifiedName, DecimalFormat>();
		}
		decimalFormats.Add(name, CreateDecimalFormat(infinitySymbol, nanSymbol, characters));
		return 0;
	}

	private DecimalFormat CreateDecimalFormat(string infinitySymbol, string nanSymbol, string characters)
	{
		NumberFormatInfo numberFormatInfo = new NumberFormatInfo();
		numberFormatInfo.NumberDecimalSeparator = char.ToString(characters[0]);
		numberFormatInfo.NumberGroupSeparator = char.ToString(characters[1]);
		numberFormatInfo.PositiveInfinitySymbol = infinitySymbol;
		numberFormatInfo.NegativeSign = char.ToString(characters[7]);
		numberFormatInfo.NaNSymbol = nanSymbol;
		numberFormatInfo.PercentSymbol = char.ToString(characters[2]);
		numberFormatInfo.PerMilleSymbol = char.ToString(characters[3]);
		numberFormatInfo.NegativeInfinitySymbol = numberFormatInfo.NegativeSign + numberFormatInfo.PositiveInfinitySymbol;
		return new DecimalFormat(numberFormatInfo, characters[5], characters[4], characters[6]);
	}

	public double RegisterDecimalFormatter(string formatPicture, string infinitySymbol, string nanSymbol, string characters)
	{
		if (decimalFormatters == null)
		{
			decimalFormatters = new List<DecimalFormatter>();
		}
		decimalFormatters.Add(new DecimalFormatter(formatPicture, CreateDecimalFormat(infinitySymbol, nanSymbol, characters)));
		return decimalFormatters.Count - 1;
	}

	public string FormatNumberStatic(double value, double decimalFormatterIndex)
	{
		int index = (int)decimalFormatterIndex;
		return decimalFormatters[index].Format(value);
	}

	public string FormatNumberDynamic(double value, string formatPicture, XmlQualifiedName decimalFormatName, string errorMessageName)
	{
		if (decimalFormats == null || !decimalFormats.TryGetValue(decimalFormatName, out var value2))
		{
			throw new XslTransformException("Decimal format '{0}' is not defined.", errorMessageName);
		}
		return new DecimalFormatter(formatPicture, value2).Format(value);
	}

	public string NumberFormat(IList<XPathItem> value, string formatString, double lang, string letterValue, string groupingSeparator, double groupingSize)
	{
		return new NumberFormatter(formatString, (int)lang, letterValue, groupingSeparator, (int)groupingSize).FormatSequence(value);
	}

	public int LangToLcid(string lang, bool forwardCompatibility)
	{
		return LangToLcidInternal(lang, forwardCompatibility, null);
	}

	internal static int LangToLcidInternal(string lang, bool forwardCompatibility, IErrorHelper errorHelper)
	{
		int result = 127;
		if (lang != null)
		{
			if (lang.Length == 0)
			{
				if (!forwardCompatibility)
				{
					if (errorHelper == null)
					{
						throw new XslTransformException("'{1}' is an invalid value for the '{0}' attribute.", "lang", lang);
					}
					errorHelper.ReportError("'{1}' is an invalid value for the '{0}' attribute.", "lang", lang);
				}
			}
			else
			{
				try
				{
					result = new CultureInfo(lang).LCID;
				}
				catch (ArgumentException)
				{
					if (!forwardCompatibility)
					{
						if (errorHelper == null)
						{
							throw new XslTransformException("'{0}' is not a supported language identifier.", lang);
						}
						errorHelper.ReportError("'{0}' is not a supported language identifier.", lang);
					}
				}
			}
		}
		return result;
	}

	private static TypeCode GetTypeCode(XPathItem item)
	{
		Type valueType = item.ValueType;
		if (valueType == XsltConvert.StringType)
		{
			return TypeCode.String;
		}
		if (valueType == XsltConvert.DoubleType)
		{
			return TypeCode.Double;
		}
		return TypeCode.Boolean;
	}

	private static TypeCode WeakestTypeCode(TypeCode typeCode1, TypeCode typeCode2)
	{
		if (typeCode1 >= typeCode2)
		{
			return typeCode2;
		}
		return typeCode1;
	}

	private static bool CompareNumbers(ComparisonOperator op, double left, double right)
	{
		return op switch
		{
			ComparisonOperator.Eq => left == right, 
			ComparisonOperator.Ne => left != right, 
			ComparisonOperator.Lt => left < right, 
			ComparisonOperator.Le => left <= right, 
			ComparisonOperator.Gt => left > right, 
			_ => left >= right, 
		};
	}

	private static bool CompareValues(ComparisonOperator op, XPathItem left, XPathItem right, TypeCode compType)
	{
		return compType switch
		{
			TypeCode.Double => CompareNumbers(op, XsltConvert.ToDouble(left), XsltConvert.ToDouble(right)), 
			TypeCode.String => XsltConvert.ToString(left) == XsltConvert.ToString(right) == (op == ComparisonOperator.Eq), 
			_ => XsltConvert.ToBoolean(left) == XsltConvert.ToBoolean(right) == (op == ComparisonOperator.Eq), 
		};
	}

	private static bool CompareNodeSetAndValue(ComparisonOperator op, IList<XPathNavigator> nodeset, XPathItem val, TypeCode compType)
	{
		if (compType == TypeCode.Boolean)
		{
			return CompareNumbers(op, (nodeset.Count != 0) ? 1 : 0, XsltConvert.ToBoolean(val) ? 1 : 0);
		}
		int count = nodeset.Count;
		for (int i = 0; i < count; i++)
		{
			if (CompareValues(op, nodeset[i], val, compType))
			{
				return true;
			}
		}
		return false;
	}

	private static bool CompareNodeSetAndNodeSet(ComparisonOperator op, IList<XPathNavigator> left, IList<XPathNavigator> right, TypeCode compType)
	{
		int count = left.Count;
		int count2 = right.Count;
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < count2; j++)
			{
				if (CompareValues(op, left[i], right[j], compType))
				{
					return true;
				}
			}
		}
		return false;
	}

	public bool EqualityOperator(double opCode, IList<XPathItem> left, IList<XPathItem> right)
	{
		ComparisonOperator op = (ComparisonOperator)opCode;
		if (IsNodeSetOrRtf(left))
		{
			if (IsNodeSetOrRtf(right))
			{
				return CompareNodeSetAndNodeSet(op, ToNodeSetOrRtf(left), ToNodeSetOrRtf(right), TypeCode.String);
			}
			XPathItem xPathItem = right[0];
			return CompareNodeSetAndValue(op, ToNodeSetOrRtf(left), xPathItem, GetTypeCode(xPathItem));
		}
		if (IsNodeSetOrRtf(right))
		{
			XPathItem xPathItem2 = left[0];
			return CompareNodeSetAndValue(op, ToNodeSetOrRtf(right), xPathItem2, GetTypeCode(xPathItem2));
		}
		XPathItem xPathItem3 = left[0];
		XPathItem xPathItem4 = right[0];
		return CompareValues(op, xPathItem3, xPathItem4, WeakestTypeCode(GetTypeCode(xPathItem3), GetTypeCode(xPathItem4)));
	}

	private static ComparisonOperator InvertOperator(ComparisonOperator op)
	{
		return op switch
		{
			ComparisonOperator.Lt => ComparisonOperator.Gt, 
			ComparisonOperator.Le => ComparisonOperator.Ge, 
			ComparisonOperator.Gt => ComparisonOperator.Lt, 
			ComparisonOperator.Ge => ComparisonOperator.Le, 
			_ => op, 
		};
	}

	public bool RelationalOperator(double opCode, IList<XPathItem> left, IList<XPathItem> right)
	{
		ComparisonOperator op = (ComparisonOperator)opCode;
		if (IsNodeSetOrRtf(left))
		{
			if (IsNodeSetOrRtf(right))
			{
				return CompareNodeSetAndNodeSet(op, ToNodeSetOrRtf(left), ToNodeSetOrRtf(right), TypeCode.Double);
			}
			XPathItem xPathItem = right[0];
			return CompareNodeSetAndValue(op, ToNodeSetOrRtf(left), xPathItem, WeakestTypeCode(GetTypeCode(xPathItem), TypeCode.Double));
		}
		if (IsNodeSetOrRtf(right))
		{
			XPathItem xPathItem2 = left[0];
			op = InvertOperator(op);
			return CompareNodeSetAndValue(op, ToNodeSetOrRtf(right), xPathItem2, WeakestTypeCode(GetTypeCode(xPathItem2), TypeCode.Double));
		}
		XPathItem left2 = left[0];
		XPathItem right2 = right[0];
		return CompareValues(op, left2, right2, TypeCode.Double);
	}

	public bool IsSameNodeSort(XPathNavigator nav1, XPathNavigator nav2)
	{
		XPathNodeType nodeType = nav1.NodeType;
		XPathNodeType nodeType2 = nav2.NodeType;
		if (XPathNodeType.Text <= nodeType && nodeType <= XPathNodeType.Whitespace)
		{
			if (XPathNodeType.Text <= nodeType2)
			{
				return nodeType2 <= XPathNodeType.Whitespace;
			}
			return false;
		}
		if (nodeType == nodeType2 && Ref.Equal(nav1.LocalName, nav2.LocalName))
		{
			return Ref.Equal(nav1.NamespaceURI, nav2.NamespaceURI);
		}
		return false;
	}

	[Conditional("DEBUG")]
	internal static void CheckXsltValue(XPathItem item)
	{
	}

	[Conditional("DEBUG")]
	internal static void CheckXsltValue(IList<XPathItem> val)
	{
		if (val.Count == 1)
		{
			XsltFunctions.EXslObjectType(val);
			return;
		}
		int count = val.Count;
		for (int i = 0; i < count && val[i].IsNode; i++)
		{
			if (i == 1)
			{
				i += Math.Max(count - 4, 0);
			}
		}
	}

	private static bool IsNodeSetOrRtf(IList<XPathItem> val)
	{
		if (val.Count == 1)
		{
			return val[0].IsNode;
		}
		return true;
	}

	private static IList<XPathNavigator> ToNodeSetOrRtf(IList<XPathItem> val)
	{
		return XmlILStorageConverter.ItemsToNavigators(val);
	}
}
