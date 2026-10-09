using System;
using System.Globalization;
using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class XPathScanner
{
	public enum LexKind
	{
		Comma = 44,
		Slash = 47,
		At = 64,
		Dot = 46,
		LParens = 40,
		RParens = 41,
		LBracket = 91,
		RBracket = 93,
		Star = 42,
		Plus = 43,
		Minus = 45,
		Eq = 61,
		Lt = 60,
		Gt = 62,
		Bang = 33,
		Dollar = 36,
		Apos = 39,
		Quote = 34,
		Union = 124,
		Ne = 78,
		Le = 76,
		Ge = 71,
		And = 65,
		Or = 79,
		DotDot = 68,
		SlashSlash = 83,
		Name = 110,
		String = 115,
		Number = 100,
		Axe = 97,
		Eof = 69
	}

	private string xpathExpr;

	private int xpathExprIndex;

	private LexKind kind;

	private char currentChar;

	private string name;

	private string prefix;

	private string stringValue;

	private double numberValue = double.NaN;

	private bool canBeFunction;

	private XmlCharType xmlCharType = XmlCharType.Instance;

	public string SourceText => xpathExpr;

	private char CurerntChar => currentChar;

	public LexKind Kind => kind;

	public string Name => name;

	public string Prefix => prefix;

	public string StringValue => stringValue;

	public double NumberValue => numberValue;

	public bool CanBeFunction => canBeFunction;

	public XPathScanner(string xpathExpr)
	{
		if (xpathExpr == null)
		{
			throw XPathException.Create("'{0}' is an invalid expression.", string.Empty);
		}
		this.xpathExpr = xpathExpr;
		NextChar();
		NextLex();
	}

	private bool NextChar()
	{
		if (xpathExprIndex < xpathExpr.Length)
		{
			currentChar = xpathExpr[xpathExprIndex++];
			return true;
		}
		currentChar = '\0';
		return false;
	}

	private void SkipSpace()
	{
		while (xmlCharType.IsWhiteSpace(CurerntChar) && NextChar())
		{
		}
	}

	public bool NextLex()
	{
		SkipSpace();
		switch (CurerntChar)
		{
		case '\0':
			kind = LexKind.Eof;
			return false;
		case '#':
		case '$':
		case '(':
		case ')':
		case '*':
		case '+':
		case ',':
		case '-':
		case '=':
		case '@':
		case '[':
		case ']':
		case '|':
			kind = (LexKind)Convert.ToInt32(CurerntChar, CultureInfo.InvariantCulture);
			NextChar();
			break;
		case '<':
			kind = LexKind.Lt;
			NextChar();
			if (CurerntChar == '=')
			{
				kind = LexKind.Le;
				NextChar();
			}
			break;
		case '>':
			kind = LexKind.Gt;
			NextChar();
			if (CurerntChar == '=')
			{
				kind = LexKind.Ge;
				NextChar();
			}
			break;
		case '!':
			kind = LexKind.Bang;
			NextChar();
			if (CurerntChar == '=')
			{
				kind = LexKind.Ne;
				NextChar();
			}
			break;
		case '.':
			kind = LexKind.Dot;
			NextChar();
			if (CurerntChar == '.')
			{
				kind = LexKind.DotDot;
				NextChar();
			}
			else if (XmlCharType.IsDigit(CurerntChar))
			{
				kind = LexKind.Number;
				numberValue = ScanFraction();
			}
			break;
		case '/':
			kind = LexKind.Slash;
			NextChar();
			if (CurerntChar == '/')
			{
				kind = LexKind.SlashSlash;
				NextChar();
			}
			break;
		case '"':
		case '\'':
			kind = LexKind.String;
			stringValue = ScanString();
			break;
		default:
			if (XmlCharType.IsDigit(CurerntChar))
			{
				kind = LexKind.Number;
				numberValue = ScanNumber();
				break;
			}
			if (xmlCharType.IsStartNCNameSingleChar(CurerntChar))
			{
				kind = LexKind.Name;
				name = ScanName();
				prefix = string.Empty;
				if (CurerntChar == ':')
				{
					NextChar();
					if (CurerntChar == ':')
					{
						NextChar();
						kind = LexKind.Axe;
					}
					else
					{
						prefix = name;
						if (CurerntChar == '*')
						{
							NextChar();
							name = "*";
						}
						else
						{
							if (!xmlCharType.IsStartNCNameSingleChar(CurerntChar))
							{
								throw XPathException.Create("'{0}' has an invalid qualified name.", SourceText);
							}
							name = ScanName();
						}
					}
				}
				else
				{
					SkipSpace();
					if (CurerntChar == ':')
					{
						NextChar();
						if (CurerntChar != ':')
						{
							throw XPathException.Create("'{0}' has an invalid qualified name.", SourceText);
						}
						NextChar();
						kind = LexKind.Axe;
					}
				}
				SkipSpace();
				canBeFunction = CurerntChar == '(';
				break;
			}
			throw XPathException.Create("'{0}' has an invalid token.", SourceText);
		}
		return true;
	}

	private double ScanNumber()
	{
		int startIndex = xpathExprIndex - 1;
		int num = 0;
		while (XmlCharType.IsDigit(CurerntChar))
		{
			NextChar();
			num++;
		}
		if (CurerntChar == '.')
		{
			NextChar();
			num++;
			while (XmlCharType.IsDigit(CurerntChar))
			{
				NextChar();
				num++;
			}
		}
		return XmlConvert.ToXPathDouble(xpathExpr.Substring(startIndex, num));
	}

	private double ScanFraction()
	{
		int startIndex = xpathExprIndex - 2;
		int num = 1;
		while (XmlCharType.IsDigit(CurerntChar))
		{
			NextChar();
			num++;
		}
		return XmlConvert.ToXPathDouble(xpathExpr.Substring(startIndex, num));
	}

	private string ScanString()
	{
		char curerntChar = CurerntChar;
		NextChar();
		int startIndex = xpathExprIndex - 1;
		int num = 0;
		while (CurerntChar != curerntChar)
		{
			if (!NextChar())
			{
				throw XPathException.Create("This is an unclosed string.");
			}
			num++;
		}
		NextChar();
		return xpathExpr.Substring(startIndex, num);
	}

	private string ScanName()
	{
		int startIndex = xpathExprIndex - 1;
		int num = 0;
		while (xmlCharType.IsNCNameSingleChar(CurerntChar))
		{
			NextChar();
			num++;
		}
		return xpathExpr.Substring(startIndex, num);
	}
}
