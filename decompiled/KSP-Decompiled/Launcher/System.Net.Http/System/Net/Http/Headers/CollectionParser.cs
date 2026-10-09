using System.Collections.Generic;

namespace System.Net.Http.Headers;

internal static class CollectionParser
{
	public static bool TryParse<T>(string input, int minimalCount, ElementTryParser<T> parser, out List<T> result) where T : class
	{
		Lexer lexer = new Lexer(input);
		result = new List<T>();
		Token token;
		do
		{
			if (!parser(lexer, out var parsedValue, out token))
			{
				return false;
			}
			if (parsedValue != null)
			{
				result.Add(parsedValue);
			}
		}
		while ((Token.Type)token == Token.Type.SeparatorComma);
		if ((Token.Type)token == Token.Type.End)
		{
			if (minimalCount > result.Count)
			{
				result = null;
				return false;
			}
			return true;
		}
		result = null;
		return false;
	}

	public static bool TryParse(string input, int minimalCount, out List<string> result)
	{
		return TryParse(input, minimalCount, (ElementTryParser<string>)TryParseStringElement, out result);
	}

	public static bool TryParseRepetition(string input, int minimalCount, out List<string> result)
	{
		return TryParseRepetition(input, minimalCount, (ElementTryParser<string>)TryParseStringElement, out result);
	}

	private static bool TryParseStringElement(Lexer lexer, out string parsedValue, out Token t)
	{
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.Token)
		{
			parsedValue = lexer.GetStringValue(t);
			if (parsedValue.Length == 0)
			{
				parsedValue = null;
			}
			t = lexer.Scan();
		}
		else
		{
			parsedValue = null;
		}
		return true;
	}

	public static bool TryParseRepetition<T>(string input, int minimalCount, ElementTryParser<T> parser, out List<T> result) where T : class
	{
		Lexer lexer = new Lexer(input);
		result = new List<T>();
		Token token;
		do
		{
			if (!parser(lexer, out var parsedValue, out token))
			{
				return false;
			}
			if (parsedValue != null)
			{
				result.Add(parsedValue);
			}
		}
		while ((Token.Type)token != Token.Type.End);
		if (minimalCount > result.Count)
		{
			return false;
		}
		return true;
	}
}
