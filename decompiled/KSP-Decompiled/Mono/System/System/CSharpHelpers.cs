using System.Collections.Generic;
using System.Globalization;

namespace System;

internal abstract class CSharpHelpers
{
	private static Dictionary<string, object> s_fixedStringLookup;

	private static readonly string[][] s_keywords;

	static CSharpHelpers()
	{
		s_keywords = new string[10][]
		{
			null,
			new string[5] { "as", "do", "if", "in", "is" },
			new string[6] { "for", "int", "new", "out", "ref", "try" },
			new string[15]
			{
				"base", "bool", "byte", "case", "char", "else", "enum", "goto", "lock", "long",
				"null", "this", "true", "uint", "void"
			},
			new string[16]
			{
				"break", "catch", "class", "const", "event", "false", "fixed", "float", "sbyte", "short",
				"throw", "ulong", "using", "where", "while", "yield"
			},
			new string[15]
			{
				"double", "extern", "object", "params", "public", "return", "sealed", "sizeof", "static", "string",
				"struct", "switch", "typeof", "unsafe", "ushort"
			},
			new string[8] { "checked", "decimal", "default", "finally", "foreach", "partial", "private", "virtual" },
			new string[10] { "abstract", "continue", "delegate", "explicit", "implicit", "internal", "operator", "override", "readonly", "volatile" },
			new string[7] { "__arglist", "__makeref", "__reftype", "interface", "namespace", "protected", "unchecked" },
			new string[2] { "__refvalue", "stackalloc" }
		};
		s_fixedStringLookup = new Dictionary<string, object>();
		for (int i = 0; i < s_keywords.Length; i++)
		{
			string[] array = s_keywords[i];
			if (array != null)
			{
				for (int j = 0; j < array.Length; j++)
				{
					s_fixedStringLookup.Add(array[j], null);
				}
			}
		}
	}

	public static string CreateEscapedIdentifier(string name)
	{
		if (IsKeyword(name) || IsPrefixTwoUnderscore(name))
		{
			return "@" + name;
		}
		return name;
	}

	public static bool IsValidLanguageIndependentIdentifier(string value)
	{
		return IsValidTypeNameOrIdentifier(value, isTypeName: false);
	}

	internal static bool IsKeyword(string value)
	{
		return s_fixedStringLookup.ContainsKey(value);
	}

	internal static bool IsPrefixTwoUnderscore(string value)
	{
		if (value.Length < 3)
		{
			return false;
		}
		if (value[0] == '_' && value[1] == '_')
		{
			return value[2] != '_';
		}
		return false;
	}

	internal static bool IsValidTypeNameOrIdentifier(string value, bool isTypeName)
	{
		bool nextMustBeStartChar = true;
		if (value.Length == 0)
		{
			return false;
		}
		foreach (char c in value)
		{
			switch (CharUnicodeInfo.GetUnicodeCategory(c))
			{
			case UnicodeCategory.UppercaseLetter:
			case UnicodeCategory.LowercaseLetter:
			case UnicodeCategory.TitlecaseLetter:
			case UnicodeCategory.ModifierLetter:
			case UnicodeCategory.OtherLetter:
			case UnicodeCategory.LetterNumber:
				nextMustBeStartChar = false;
				break;
			case UnicodeCategory.NonSpacingMark:
			case UnicodeCategory.SpacingCombiningMark:
			case UnicodeCategory.DecimalDigitNumber:
			case UnicodeCategory.ConnectorPunctuation:
				if (nextMustBeStartChar && c != '_')
				{
					return false;
				}
				nextMustBeStartChar = false;
				break;
			default:
				if (!isTypeName || !IsSpecialTypeChar(c, ref nextMustBeStartChar))
				{
					return false;
				}
				break;
			}
		}
		return true;
	}

	internal static bool IsSpecialTypeChar(char ch, ref bool nextMustBeStartChar)
	{
		switch (ch)
		{
		case '$':
		case '&':
		case '*':
		case '+':
		case ',':
		case '-':
		case '.':
		case ':':
		case '<':
		case '>':
		case '[':
		case ']':
			nextMustBeStartChar = true;
			return true;
		case '`':
			return true;
		default:
			return false;
		}
	}
}
